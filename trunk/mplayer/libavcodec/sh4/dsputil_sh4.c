/*
 *  sh4 dsputil
 *
 * Copyright (c) 2003 BERO <bero@geocities.co.jp>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "../avcodec.h"
#include "../dsputil.h"

static void memzero_align8(void *dst,size_t size)
{
#if defined(__SH4__) || defined(__SH4_SINGLE__) || defined(__SH4_SINGLE_ONLY__)
	(char*)dst+=size;
	size/=8*4;
	asm(
#if defined(__SH4__)
	" fschg\n"  //single float mode
#endif
	" fldi0 fr0\n"
	" fldi0 fr1\n"
	" fschg\n"  // double
	"1: \n" \
	" dt %1\n"
	" fmov	dr0,@-%0\n"
	" fmov	dr0,@-%0\n"
	" fmov	dr0,@-%0\n"
	" bf.s 1b\n"
	" fmov	dr0,@-%0\n"
#if defined(__SH4_SINGLE__) || defined(__SH4_SINGLE_ONLY__)
	" fschg" //back to single
#endif
	: : "r"(dst),"r"(size): "memory" );
#else
	double *d = dst;
	size/=8*4;
	do {
		d[0] = 0.0;
		d[1] = 0.0;
		d[2] = 0.0;
		d[3] = 0.0;
		d+=4;
	} while(--size);
#endif
}

static void clear_blocks_sh4(DCTELEM *blocks)
{
//	if (((int)blocks&7)==0) 
	memzero_align8(blocks,sizeof(DCTELEM)*6*64);
}

extern void idct_sh4(DCTELEM *block);
static void idct_put(uint8_t *dest, int line_size, DCTELEM *block)
{
	idct_sh4(block);
	int i;
	uint8_t *cm = cropTbl + MAX_NEG_CROP;
	for(i=0;i<8;i++) {
		dest[0] = cm[block[0]];
		dest[1] = cm[block[1]];
		dest[2] = cm[block[2]];
		dest[3] = cm[block[3]];
		dest[4] = cm[block[4]];
		dest[5] = cm[block[5]];
		dest[6] = cm[block[6]];
		dest[7] = cm[block[7]];
		dest+=line_size;
		block+=8;
	}
}
static void idct_add(uint8_t *dest, int line_size, DCTELEM *block)
{
	idct_sh4(block);
	int i;
	uint8_t *cm = cropTbl + MAX_NEG_CROP;
	for(i=0;i<8;i++) {
		dest[0] = cm[dest[0]+block[0]];
		dest[1] = cm[dest[1]+block[1]];
		dest[2] = cm[dest[2]+block[2]];
		dest[3] = cm[dest[3]+block[3]];
		dest[4] = cm[dest[4]+block[4]];
		dest[5] = cm[dest[5]+block[5]];
		dest[6] = cm[dest[6]+block[6]];
		dest[7] = cm[dest[7]+block[7]];
		dest+=line_size;
		block+=8;
	}
}

extern void dsputil_init_align(DSPContext* c, AVCodecContext *avctx);

void dsputil_init_sh4(DSPContext* c, AVCodecContext *avctx)
{
	const int idct_algo= avctx->idct_algo;
	dsputil_init_align(c,avctx);

	c->clear_blocks = clear_blocks_sh4;
	if(idct_algo==FF_IDCT_AUTO || idct_algo==FF_IDCT_SH4){        
		c->idct_put = idct_put;
		c->idct_add = idct_add;
               c->idct     = idct_sh4;
		c->idct_permutation_type= FF_NO_IDCT_PERM; //FF_SIMPLE_IDCT_PERM; //FF_LIBMPEG2_IDCT_PERM;
	}
}
