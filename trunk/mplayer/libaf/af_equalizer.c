/*=============================================================================
//	
//  This software has been released under the terms of the GNU Public
//  license. See http://www.gnu.org/copyleft/gpl.html for details.
//
//  Copyright 2001 Anders Johansson ajh@atri.curtin.edu.au
//
//=============================================================================
*/

/* Equalizer filter, implementation of a 10 band time domain graphic
   equalizer using IIR filters. The IIR filters are implemented using a
   Direct Form II approach, but has been modified (b1 == 0 always) to
   save computation.
*/

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <inttypes.h>
#include <math.h>

#include "af.h"

#define L   	2      // Storage for filter taps
#define KM  	10     // Max number of bands 

#define Q   1.2247449 /* Q value for band-pass filters 1.2247=(3/2)^(1/2)
			 gives 4dB suppression @ Fc*2 and Fc/2 */

/* Center frequencies for band-pass filters
   The different frequency bands are:	
   nr.    	center frequency
   0  	31.25 Hz
   1 	62.50 Hz
   2	125.0 Hz
   3	250.0 Hz
   4	500.0 Hz
   5	1.000 kHz
   6	2.000 kHz
   7	4.000 kHz
   8	8.000 kHz
   9 	16.00 kHz
*/
#define CF  	{31.25,62.5,125,250,500,1000,2000,4000,8000,16000}

// Maximum and minimum gain for the bands
#define G_MAX	+12.0
#define G_MIN	-12.0	

// Data for specific instances of this filter
typedef struct af_equalizer_s
{
  float   a[KM][L];        	// A weights
  float   b[KM][L];	     	// B weights
  float   wq[AF_NCH][KM][L];  	// Circular buffer for W data
  float   g[AF_NCH][KM];      	// Gain factor for each channel and band
  int     K; 		   	// Number of used eq bands
  int     channels;        	// Number of channels
} af_equalizer_t;

// 2nd order Band-pass Filter design
static void bp2(float* a, float* b, float fc, float q){
  double th= 2.0 * M_PI * fc;
  double C = (1.0 - tan(th*q/2.0))/(1.0 + tan(th*q/2.0));

  a[0] = (1.0 + C) * cos(th);
  a[1] = -1 * C;
  
  b[0] = (1.0 - C)/2.0;
  b[1] = -1.0050;
}

// Initialization and runtime control
static int control(struct af_instance_s* af, int cmd, void* arg)
{
  af_equalizer_t* s   = (af_equalizer_t*)af->setup; 

  switch(cmd){
  case AF_CONTROL_REINIT:{
    int k =0;
    float F[KM] = CF;
    
    // Sanity check
    if(!arg) return AF_ERROR;
    
    af->data->rate   = ((af_data_t*)arg)->rate;
    af->data->nch    = ((af_data_t*)arg)->nch;
    af->data->format = AF_FORMAT_NE | AF_FORMAT_F;
    af->data->bps    = 4;
    
    // Calculate number of active filters
    s->K=KM;
    while(F[s->K-1] > (float)af->data->rate/2.2)
      s->K--;
    
    if(s->K != KM)
      af_msg(AF_MSG_INFO,"[equalizer] Limiting the number of filters to" 
	     " %i due to low sample rate.\n",s->K);

    // Generate filter taps
    for(k=0;k<s->K;k++)
      bp2(s->a[k],s->b[k],F[k]/((float)af->data->rate),Q);

    // Calculate how much this plugin adds to the overall time delay
    af->delay += 2000.0/((float)af->data->rate);

    return af_test_output(af,arg);
  }
  case AF_CONTROL_COMMAND_LINE:{
    float g[10]={0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0};
    int i,j;
    sscanf((char*)arg,"%f:%f:%f:%f:%f:%f:%f:%f:%f:%f", &g[0], &g[1], 
	   &g[2], &g[3], &g[4], &g[5], &g[6], &g[7], &g[8] ,&g[9]);
    for(i=0;i<AF_NCH;i++){
      for(j=0;j<KM;j++){
	((af_equalizer_t*)af->setup)->g[i][j] = 
	  pow(10.0,clamp(g[j],G_MIN,G_MAX)/20.0)-1.0;
      }
    }
    return AF_OK;
  }
  case AF_CONTROL_EQUALIZER_GAIN | AF_CONTROL_SET:{
    float* gain = ((af_control_ext_t*)arg)->arg;
    int    ch   = ((af_control_ext_t*)arg)->ch;
    int    k;
    if(ch > AF_NCH || ch < 0)
      return AF_ERROR;

    for(k = 0 ; k<KM ; k++)
      s->g[ch][k] = pow(10.0,clamp(gain[k],G_MIN,G_MAX)/20.0)-1.0;

    return AF_OK;
  }
  case AF_CONTROL_EQUALIZER_GAIN | AF_CONTROL_GET:{
    float* gain = ((af_control_ext_t*)arg)->arg;
    int    ch   = ((af_control_ext_t*)arg)->ch;
    int    k;
    if(ch > AF_NCH || ch < 0)
      return AF_ERROR;

    for(k = 0 ; k<KM ; k++)
      gain[k] = log10(s->g[ch][k]+1.0) * 20.0;

    return AF_OK;
  }
  }
  return AF_UNKNOWN;
}

// Deallocate memory 
static void uninit(struct af_instance_s* af)
{
  if(af->data)
    free(af->data);
  if(af->setup)
    free(af->setup);
}

// Filter data through filter
static af_data_t* play(struct af_instance_s* af, af_data_t* data)
{
  af_data_t*       c 	= data;			    	// Current working data
  af_equalizer_t*  s 	= (af_equalizer_t*)af->setup; 	// Setup 
  uint32_t  	   ci  	= af->data->nch; 	    	// Index for channels
  uint32_t	   nch 	= af->data->nch;   	    	// Number of channels

  while(ci--){
    float*	g   = s->g[ci];      // Gain factor 
    float*	in  = ((float*)c->audio)+ci;
    float*	out = ((float*)c->audio)+ci;
    float* 	end = in + c->len/4; // Block loop end

    while(in < end){
      register uint32_t	k  = 0;		// Frequency band index
      register float 	yt = *in; 	// Current input sample
      in+=nch;
      
      // Run the filters
      for(;k<s->K;k++){
 	// Pointer to circular buffer wq
 	register float* wq = s->wq[ci][k];
 	// Calculate output from AR part of current filter
 	register float w=yt*s->b[k][0] + wq[0]*s->a[k][0] + wq[1]*s->a[k][1];
 	// Calculate output form MA part of current filter
 	yt+=(w + wq[1]*s->b[k][1])*g[k];
 	// Update circular buffer
 	wq[1] = wq[0];
	wq[0] = w;
      }
      // Calculate output 
      *out=yt/(4.0*10.0);
      out+=nch;
    }
  }
  return c;
}

// Allocate memory and set function pointers
static int open(af_instance_t* af){
  af->control=control;
  af->uninit=uninit;
  af->play=play;
  af->mul.n=1;
  af->mul.d=1;
  af->data=calloc(1,sizeof(af_data_t));
  af->setup=calloc(1,sizeof(af_equalizer_t));
  if(af->data == NULL || af->setup == NULL)
    return AF_ERROR;
  return AF_OK;
}

// Description of this filter
af_info_t af_info_equalizer = {
  "Equalizer audio filter",
  "equalizer",
  "Anders",
  "",
  AF_FLAGS_NOT_REENTRANT,
  open
};







