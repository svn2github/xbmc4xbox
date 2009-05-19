/*
 *      Copyright (C) 2009 Team XBMC
 *
 *  This Program is free software you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "DSPFilter.h"

CDSPFilter::CDSPFilter()
{
  Create(1,1);
}

CDSPFilter::CDSPFilter(unsigned int inputBusses, unsigned int outputBusses)
{
  Create(inputBusses,outputBusses);
}

CDSPFilter::~CDSPFilter()
{
  Destroy();
}

void CDSPFilter::Create(unsigned int inputBusses, unsigned int outputBusses)
{
  m_InputBusses = inputBusses;
  m_OutputBusses = outputBusses;

  m_pInputDescriptor = (StreamAttributes*)calloc(inputBusses, sizeof(StreamAttributes));
  m_pOutputDescriptor = (StreamAttributes*)calloc(outputBusses, sizeof(StreamAttributes));
  m_pInput = (InputBus*)calloc(inputBusses, sizeof(InputBus));
}

void CDSPFilter::Destroy()
{
  free(m_pInputDescriptor);
  free(m_pOutputDescriptor);
  free(m_pInput);
}

// IAudioSink
MA_RESULT CDSPFilter::TestInputFormat(CStreamDescriptor* pDesc, unsigned int bus /* = 0*/)
{
  if (!pDesc)
    return MA_ERROR;

  if (bus >= m_InputBusses)
    return MA_INVALID_BUS;

  CStreamAttributeCollection *pAttribs = pDesc->GetAttributes();
  // Verify that the required attributes are present and of the correct type
  if (
    (pAttribs->GetFlag(MA_ATT_TYPE_STREAM_FLAGS , MA_STREAM_FLAG_LOCKED, NULL) != MA_SUCCESS) ||
    (pAttribs->GetInt(MA_ATT_TYPE_BYTES_PER_SEC , NULL) != MA_SUCCESS) ||
    (pAttribs->GetInt(MA_ATT_TYPE_BYTES_PER_FRAME, NULL) != MA_SUCCESS) ||
    (pAttribs->GetInt(MA_ATT_TYPE_STREAM_FORMAT , NULL) != MA_SUCCESS)) 
    return MA_MISSING_ATTRIBUTE;

  return MA_SUCCESS;
}

MA_RESULT CDSPFilter::SetInputFormat(CStreamDescriptor* pDesc, unsigned int bus /* = 0*/)
{
  if (bus >= m_InputBusses)
    return MA_INVALID_BUS;

  CStreamAttributeCollection *pAttribs = pDesc->GetAttributes();
  // Fetch and store the required stream attributes
  if (
    (pAttribs->GetFlag(MA_ATT_TYPE_STREAM_FLAGS, MA_STREAM_FLAG_LOCKED, &m_pInputDescriptor[bus].m_Locked) != MA_SUCCESS) ||
    (pAttribs->GetFlag(MA_ATT_TYPE_STREAM_FLAGS, MA_STREAM_FLAG_LOCKED, &m_pInputDescriptor[bus].m_VariableBitrate) != MA_SUCCESS) ||
    (pAttribs->GetInt(MA_ATT_TYPE_BYTES_PER_SEC, (int*)&m_pInputDescriptor[bus].m_BytesPerSecond) != MA_SUCCESS) ||
    (pAttribs->GetInt(MA_ATT_TYPE_BYTES_PER_FRAME, (int*)&m_pInputDescriptor[bus].m_FrameSize) != MA_SUCCESS) ||
    (pAttribs->GetInt(MA_ATT_TYPE_STREAM_FORMAT , &m_pInputDescriptor[bus].m_StreamFormat) != MA_SUCCESS)) 
  {
    ClearInputFormat(bus);
    return MA_MISSING_ATTRIBUTE;
  }

  return MA_SUCCESS;
}

MA_RESULT CDSPFilter::SetSource(IAudioSource* pSource, unsigned int sourceBus, unsigned int sinkBus /* = 0*/)
{
  if (sinkBus >= m_InputBusses)
    return MA_INVALID_BUS;

  m_pInput[sinkBus].source = pSource;
  m_pInput[sinkBus].bus = sourceBus;

  return MA_SUCCESS;
}

float CDSPFilter::GetMaxLatency()
{
  return 0.0f;
}

void CDSPFilter::Flush()
{

}

// IAudioSource
MA_RESULT CDSPFilter::TestOutputFormat(CStreamDescriptor* pDesc, unsigned int bus /* = 0*/)
{
  if(!pDesc)
    return MA_ERROR;
  
  if (bus >= m_OutputBusses)
    return MA_INVALID_BUS;

  CStreamAttributeCollection *pAttribs = pDesc->GetAttributes();
  // Verify that the required attributes are present and of the correct type
  if (
    (pAttribs->GetFlag(MA_ATT_TYPE_STREAM_FLAGS , MA_STREAM_FLAG_LOCKED, NULL) != MA_SUCCESS) || // If one flag exists, then the bitfield exists
    (pAttribs->GetInt(MA_ATT_TYPE_BYTES_PER_SEC , NULL) != MA_SUCCESS) ||
    (pAttribs->GetInt(MA_ATT_TYPE_BYTES_PER_FRAME, NULL) != MA_SUCCESS) ||
    (pAttribs->GetInt(MA_ATT_TYPE_STREAM_FORMAT , NULL) != MA_SUCCESS)) 
    return MA_MISSING_ATTRIBUTE;

  return MA_SUCCESS;
}

MA_RESULT CDSPFilter::SetOutputFormat(CStreamDescriptor* pDesc, unsigned int bus /* = 0*/)
{
  if (bus >= m_OutputBusses)
    return MA_INVALID_BUS;

  if(!pDesc)
  {
    ClearOutputFormat(bus);
    return MA_SUCCESS;
  }

  CStreamAttributeCollection *pAttribs = pDesc->GetAttributes();
  // Fetch and store the required stream attributes
  if (
    (pAttribs->GetFlag(MA_ATT_TYPE_STREAM_FLAGS, MA_STREAM_FLAG_LOCKED, &m_pOutputDescriptor[bus].m_Locked) != MA_SUCCESS) ||
    (pAttribs->GetFlag(MA_ATT_TYPE_STREAM_FLAGS, MA_STREAM_FLAG_LOCKED, &m_pOutputDescriptor[bus].m_VariableBitrate) != MA_SUCCESS) ||
    (pAttribs->GetInt(MA_ATT_TYPE_BYTES_PER_SEC, (int*)&m_pOutputDescriptor[bus].m_BytesPerSecond) != MA_SUCCESS) ||
    (pAttribs->GetInt(MA_ATT_TYPE_BYTES_PER_FRAME, (int*)&m_pOutputDescriptor[bus].m_FrameSize) != MA_SUCCESS) ||
    (pAttribs->GetInt(MA_ATT_TYPE_STREAM_FORMAT , &m_pOutputDescriptor[bus].m_StreamFormat) != MA_SUCCESS)) 
  {
    ClearOutputFormat(bus);
    return MA_MISSING_ATTRIBUTE;
  }

  return MA_SUCCESS;
}

MA_RESULT CDSPFilter::Render(ma_audio_container* pOutput, unsigned int frameCount, ma_timestamp renderTime, unsigned int renderFlags, unsigned int bus /* = 0*/)
{
  return RenderOutput(pOutput, frameCount, renderTime, renderFlags, bus);
}

// IDSPFilter
void CDSPFilter::Close()
{

}

// Local Implementation

ma_audio_container* CDSPFilter::GetInputData(unsigned int frameCount, ma_timestamp renderTime, unsigned int renderFlags, unsigned int bus /* = 0*/)
{
  if (m_pInput[bus].source && bus < m_InputBusses)
  {
    // TODO: These containers don't exist yet
    ma_audio_container* pInput = m_pInputContainer[bus];
    if (pInput && MA_SUCCESS == m_pInput[bus].source->Render(pInput, frameCount, renderTime, renderFlags, m_pInput[bus].bus))
      return pInput;
  }
  return NULL;
}

MA_RESULT CDSPFilter::RenderOutput(ma_audio_container* pOutput, unsigned int frameCount, ma_timestamp renderTime, unsigned int renderFlags, unsigned int bus /* = 0*/)
{
  ma_audio_container* pInput = GetInputData(frameCount, renderTime, renderFlags, bus);
  // TODO: pull data and slice as necessary
  return MA_NOT_IMPLEMENTED;
}

void CDSPFilter::ClearInputFormat(unsigned int bus /* = 0 */)
{
  if (bus < m_InputBusses)
    memset(&m_pInputDescriptor[bus], 0, sizeof(StreamAttributes));
}

void CDSPFilter::ClearOutputFormat(unsigned int bus /* = 0 */)
{
  if (bus < m_OutputBusses)
    memset(&m_pOutputDescriptor[bus], 0, sizeof(StreamAttributes));
}
