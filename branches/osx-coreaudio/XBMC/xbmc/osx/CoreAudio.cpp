#ifdef __APPLE__
/*
 *      Copyright (C) 2005-2009 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "CoreAudio.h"
#include <PlatformDefs.h>
#include <Log.h>
#include <math.h>

char* UInt32ToFourCC(UInt32* pVal) // NOT NULL TERMINATED! Modifies input value.
{
  UInt32 inVal = *pVal;
  char* pIn = (char*)&inVal;
  char* fourCC = (char*)pVal;
  fourCC[3] = pIn[0];
  fourCC[2] = pIn[1];
  fourCC[1] = pIn[2];
  fourCC[0] = pIn[3];
  return fourCC;
}

const char* StreamDescriptionToString(AudioStreamBasicDescription desc, CStdString& str)
{
  UInt32 formatId = desc.mFormatID;
  char* fourCC = UInt32ToFourCC(&formatId);
  
  switch (desc.mFormatID)
  {
    case kAudioFormatLinearPCM:
      str.Format("[%4.4s] %s%u Channel %u-bit %s (%uHz)", 
                 fourCC,
                 (desc.mFormatFlags & kAudioFormatFlagIsNonMixable) ? "" : "Mixable ",
                 desc.mChannelsPerFrame,
                 desc.mBitsPerChannel,
                 (desc.mFormatFlags & kAudioFormatFlagIsFloat) ? "Floating Point" : "Signed Integer",
                 (UInt32)desc.mSampleRate);
      break;
    case kAudioFormatAC3:
      str.Format("[%s] AC-3/DTS", fourCC);
      break;
    case kAudioFormat60958AC3:
      str.Format("[%s] AC-3/DTS for S/PDIF", fourCC);
      break;
    default:
      str.Format("[%s]", fourCC);
      break;
  }
  return str.c_str();
}

#define CONVERT_OSSTATUS(x) UInt32ToFourCC((UInt32*)&ret)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CCoreAudioHardware
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

AudioDeviceID CCoreAudioHardware::FindAudioDevice(CStdString searchName)
{
  if (!searchName.length())
    return 0;
  
  UInt32 size = 0;
  AudioDeviceID deviceId = 0;
  OSStatus ret;
  
  CLog::Log(LOGDEBUG, "CCoreAudioHardware::FindAudioDevice: Searching for device - %s.", searchName.c_str());
  
  // Obtain a list of all available audio devices
  AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices, &size, NULL);
  UInt32 deviceCount = size / sizeof(AudioDeviceID);
  AudioDeviceID* pDevices = new AudioDeviceID[deviceCount];
  ret = AudioHardwareGetProperty(kAudioHardwarePropertyDevices, &size, pDevices);
  if (ret)
  { 
    CLog::Log(LOGERROR, "CCoreAudioHardware::FindAudioDevice: Unable to retrieve the list of available devices. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
    delete[] pDevices;
    return 0; 
  }
  
  // Attempt to locate the requested device
  CStdString deviceName;
  for (UInt32 dev = 0; dev < deviceCount; dev++)
  {
    CCoreAudioDevice device;
    device.Open((pDevices[dev]));
    device.GetName(deviceName);
    UInt32 totalChannels = device.GetTotalOutputChannels();
    CLog::Log(LOGDEBUG, "CCoreAudioHardware::FindAudioDevice:   Device[0x%04x] - Name: '%s', Total Ouput Channels: %u. ", pDevices[dev], deviceName.c_str(), totalChannels);
    if (searchName.Equals(deviceName))
      deviceId = pDevices[dev];
    if (deviceId)
      break;
  }
  delete[] pDevices;  
  
  return deviceId;
}

AudioDeviceID CCoreAudioHardware::GetDefaultOutputDevice()
{
  UInt32 size = sizeof(AudioDeviceID);
  AudioDeviceID deviceId = 0;
  OSStatus ret = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultOutputDevice, &size, &deviceId);
  if (ret || !deviceId) // outputDevice is set to 0 if there is no audio device available, or if the default device is set to an encoded format
  {
    CLog::Log(LOGERROR, "CCoreAudioHardware::GetDefaultOutputDevice: Unable to identify default output device. Error = 0x%08x (%4.4s).", ret, CONVERT_OSSTATUS(ret));
    return 0;
  }
  return deviceId;
}

UInt32 CCoreAudioHardware::GetOutputDevices(CoreAudioDeviceList* pList)
{
  if (!pList)
    return 0;
  
  // Obtain a list of all available audio devices
  UInt32 found = 0;
  UInt32 size = 0;
  AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices, &size, NULL);
  UInt32 deviceCount = size / sizeof(AudioDeviceID);
  AudioDeviceID* pDevices = new AudioDeviceID[deviceCount];
  OSStatus ret = AudioHardwareGetProperty(kAudioHardwarePropertyDevices, &size, pDevices);
  if (ret)
    CLog::Log(LOGERROR, "CCoreAudioHardware::GetOutputDevices: Unable to retrieve the list of available devices. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
  else
  {
    for (UInt32 dev = 0; dev < deviceCount; dev++)
    {
      CCoreAudioDevice device(pDevices[dev]);
      if (device.GetTotalOutputChannels() == 0)
        continue;
      found++;
      pList->push_back(pDevices[dev]);
    }
  }
  delete[] pDevices;
  return found;
}

bool CCoreAudioHardware::GetAutoHogMode()
{
  UInt32 val = 0;
  UInt32 size = sizeof(val);
  OSStatus ret = AudioHardwareGetProperty(kAudioHardwarePropertyHogModeIsAllowed, &size, &val);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioHardware::GetAutoHogMode: Unable to get auto 'hog' mode. Error = 0x%08x (%4.4s).", ret, CONVERT_OSSTATUS(ret));
    return false;
  }
  return (val == 1);
}

void CCoreAudioHardware::SetAutoHogMode(bool enable)
{
  UInt32 val = enable ? 1 : 0;
  OSStatus ret = AudioHardwareSetProperty(kAudioHardwarePropertyHogModeIsAllowed, sizeof(val), &val);
  if (ret)
    CLog::Log(LOGERROR, "CCoreAudioHardware::SetAutoHogMode: Unable to set auto 'hog' mode. Error = 0x%08x (%4.4s).", ret, CONVERT_OSSTATUS(ret));
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CCoreAudioDevice
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CCoreAudioDevice::CCoreAudioDevice()  : 
  m_DeviceId(0),
  m_Hog(-1),
  m_MixerRestore(-1),
  m_IoProc(NULL),
  m_SampleRateRestore(0.0f)
{
  
}

CCoreAudioDevice::~CCoreAudioDevice()
{
  Close();
}

bool CCoreAudioDevice::Open(AudioDeviceID deviceId)
{
  m_DeviceId = deviceId;
  return true;
  CLog::Log(LOGDEBUG, "CCoreAudioDevice::Open: Opened device 0x%04x", m_DeviceId);
}

void CCoreAudioDevice::Close()
{
  if (!m_DeviceId)
    return;
  
  Stop(); // Stop the device if it was started
  
  RemoveIOProc(); // Unregister the IOProc if we have one
  
  SetHogStatus(false);
  if (m_MixerRestore > -1) // We changed the mixer status
    SetMixingSupport((m_MixerRestore ? true : false));
  m_MixerRestore = -1;
  
  if (m_SampleRateRestore != 0.0f)
  {
    CLog::Log(LOGDEBUG,  "CCoreAudioUnit::Close: Restoring original nominal samplerate.");    
    SetNominalSampleRate(m_SampleRateRestore);
  }
  
  CLog::Log(LOGDEBUG, "CCoreAudioDevice::Close: Closed device 0x%04x", m_DeviceId);
  m_DeviceId = 0;
  m_IoProc = NULL;
  
}

void CCoreAudioDevice::Start()
{
  if (!m_DeviceId) 
    return;
  
  OSStatus ret = AudioDeviceStart(m_DeviceId, m_IoProc);
  if (ret)
    CLog::Log(LOGERROR, "CCoreAudioDevice::Start: Unable to start device. Error = 0x%08x (%4.4s).", ret, CONVERT_OSSTATUS(ret));
}

void CCoreAudioDevice::Stop()
{
  if (!m_DeviceId)
    return;
  
  OSStatus ret = AudioDeviceStop(m_DeviceId, m_IoProc);
  if (ret)
    CLog::Log(LOGERROR, "CCoreAudioDevice::Stop: Unable to stop device. Error = 0x%08x (%4.4s).", ret, CONVERT_OSSTATUS(ret));
}

bool CCoreAudioDevice::AddIOProc(AudioDeviceIOProc ioProc, void* pCallbackData)
{
  if (!m_DeviceId || m_IoProc) // Only one IOProc at a time
    return false;
  
  OSStatus ret = AudioDeviceAddIOProc(m_DeviceId, ioProc, pCallbackData);  
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioDevice::Stop: Unable to add IOProc. Error = 0x%08x (%4.4s).", ret, CONVERT_OSSTATUS(ret));
    return false;
  }
  m_IoProc = ioProc;
  CLog::Log(LOGDEBUG, "CCoreAudioDevice::AddIOProc: IOProc set for device 0x%04x", m_DeviceId);
  return true;
}

void CCoreAudioDevice::RemoveIOProc()
{
  if (!m_DeviceId || !m_IoProc)
    return;
  
  Stop();
  OSStatus ret = AudioDeviceRemoveIOProc(m_DeviceId, m_IoProc);  
  if (ret)
    CLog::Log(LOGERROR, "CCoreAudioDevice::RemoveIOProc: Unable to remove IOProc. Error = 0x%08x (%4.4s).", ret, CONVERT_OSSTATUS(ret));
  else
    CLog::Log(LOGDEBUG, "CCoreAudioDevice::AddIOProc: IOProc removed for device 0x%04x", m_DeviceId);
  m_IoProc = NULL; // Clear the reference no matter what
}

const char* CCoreAudioDevice::GetName(CStdString& name)
{
  if (!m_DeviceId)
    return NULL;

  UInt32 size = 0;
  AudioDeviceGetPropertyInfo(m_DeviceId,0, false, kAudioDevicePropertyDeviceName, &size, NULL); // TODO: Change to kAudioObjectPropertyName
  OSStatus ret = AudioDeviceGetProperty(m_DeviceId, 0, false, kAudioDevicePropertyDeviceName, &size, name.GetBufferSetLength(size));  
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioDevice::SetHogStatus: Unable to get device name - id: 0x%04x Error = 0x%08x (%4.4s)", m_DeviceId, ret, CONVERT_OSSTATUS(ret));
    return NULL;
  }
  return name.c_str();
}

UInt32 CCoreAudioDevice::GetTotalOutputChannels()
{
  if (!m_DeviceId)
    return 0;
  UInt32 channels = 0;
	UInt32 size = 0;
  AudioDeviceGetPropertyInfo(m_DeviceId, 0, false, kAudioDevicePropertyStreamConfiguration, &size, NULL);
  AudioBufferList* pList = (AudioBufferList*)malloc(size);
  OSStatus ret = AudioDeviceGetProperty(m_DeviceId, 0, false, kAudioDevicePropertyStreamConfiguration, &size, pList); 
  if (!ret)
    for(UInt32 buffer = 0; buffer < pList->mNumberBuffers; ++buffer)
      channels += pList->mBuffers[buffer].mNumberChannels;
  else
    CLog::Log(LOGERROR, "CCoreAudioDevice::GetTotalChannels: Unable to get total device output channels - id: 0x%04x Error = 0x%08x (%4.4s)", m_DeviceId, ret, CONVERT_OSSTATUS(ret));
  CLog::Log(LOGDEBUG, "CCoreAudioDevice::GetTotalChannels: Found %u channels in %u buffers", channels, pList->mNumberBuffers);
  free(pList);
	return channels;  
}

                               
bool CCoreAudioDevice::GetStreams(AudioStreamIdList* pList)
{
  if (!pList || !m_DeviceId)
    return false;
  
  UInt32 propertySize = 0;
  Boolean writable = false;
  OSStatus ret = AudioDeviceGetPropertyInfo(m_DeviceId, 0, false, kAudioDevicePropertyStreams, &propertySize, &writable);
  if (ret)
    return false;
  UInt32 streamCount = propertySize / sizeof(AudioStreamID);
  AudioStreamID* pStreamList = new AudioStreamID[streamCount];
  ret = AudioDeviceGetProperty(m_DeviceId, 0, false, kAudioDevicePropertyStreams, &propertySize, pStreamList);
  if (!ret)
  {
    for (UInt32 stream = 0; stream < streamCount; stream++)
      pList->push_back(pStreamList[stream]);
  }
  delete[] pStreamList;
  return (ret == noErr);  
}

bool CCoreAudioDevice::SetHogStatus(bool hog)
{
  if (!m_DeviceId)
    return false;
  
  if (hog)
  {
    if (m_Hog == -1) // Not already set
    {
      CLog::Log(LOGDEBUG, "CCoreAudioDevice::SetHogStatus: Setting 'hog' status on device 0x%04x", m_DeviceId);
      OSStatus ret = AudioDeviceSetProperty(m_DeviceId, NULL, 0, false, kAudioDevicePropertyHogMode, sizeof(pid_t), &m_Hog);
      if (ret)
      {
        CLog::Log(LOGERROR, "CCoreAudioDevice::SetHogStatus: Unable to set 'hog' status. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
        return false;
      }
      CLog::Log(LOGDEBUG, "CCoreAudioDevice::SetHogStatus: Successfully set 'hog' status on device 0x%04x", m_DeviceId);
    }
  }
  else
  {
    if (m_Hog > -1) // Currently Set
    {
      CLog::Log(LOGDEBUG, "CCoreAudioDevice::SetHogStatus: Releasing 'hog' status on device 0x%04x", m_DeviceId);
      pid_t hogPid = -1;
      OSStatus ret = AudioDeviceSetProperty(m_DeviceId, NULL, 0, false, kAudioDevicePropertyHogMode, sizeof(pid_t), &hogPid);
      if (ret)
      {
        CLog::Log(LOGERROR, "CCoreAudioDevice::SetHogStatus: Unable to release 'hog' status. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
        return false;
      }
      m_Hog = hogPid; // Reset internal state
    }
  }
  return true;
}

bool CCoreAudioDevice::GetHogStatus()
{
  // TODO: Should we confirm with CoreAudio?
  return (m_Hog > -1);
}

bool CCoreAudioDevice::SetMixingSupport(bool mix)
{
  if (!m_DeviceId)
    return false;
  int restore = -1;
  if (m_MixerRestore == -1) // This is our first change to this setting. Store the original setting for restore
    restore = (GetMixingSupport() ? 1 : 0);
  UInt32 mixEnable = mix ? 1 : 0;
  CLog::Log(LOGDEBUG, "CCoreAudioDevice::SetMixingSupport: %sabling mixing for device 0x%04x",mix ? "En" : "Dis",  m_DeviceId);
  OSStatus ret = AudioDeviceSetProperty(m_DeviceId, NULL, 0, false, kAudioDevicePropertySupportsMixing, sizeof(UInt32), &mixEnable);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioDevice::SetMixingSupport: Unable to set MixingSupport to %s. Error = 0x%08x (%4.4s)", mix ? "'On'" : "'Off'", ret, CONVERT_OSSTATUS(ret));
    return false;
  }
  if (m_MixerRestore == -1) 
    m_MixerRestore = restore;
  return true;
}

bool CCoreAudioDevice::GetMixingSupport()
{
  if (!m_DeviceId)
    return false;
  UInt32 size = sizeof(UInt32);
  UInt32 val = 0;
  OSStatus ret = AudioDeviceGetProperty(m_DeviceId, 0, false, kAudioDevicePropertySupportsMixing, &size, &val);
  if (ret)
    return false;
  return (val > 0);
}

bool CCoreAudioDevice::GetPreferredChannelLayout(CoreAudioChannelList* pChannelMap)
{
  if (!pChannelMap || !m_DeviceId)
    return false;
  
  UInt32 propertySize = 0;
  Boolean writable = false;
  OSStatus ret = AudioDeviceGetPropertyInfo(m_DeviceId, 0, false, kAudioDevicePropertyPreferredChannelLayout, &propertySize, &writable);
  if (ret)
    return false;
  UInt32 channels = propertySize / sizeof(SInt32);
  SInt32* pMap = new SInt32[channels];
  ret = AudioDeviceGetProperty(m_DeviceId, 0, false, kAudioDevicePropertyPreferredChannelLayout, &propertySize, pMap);
  if (!ret)
    for (UInt32 i = 0; i < channels; i++)
      pChannelMap->push_back(pMap[i]);;
  delete[] pMap;
  return (!ret);  
}

bool CCoreAudioDevice::GetDataSources(CoreAudioDataSourceList* pList)
{
  if (!pList || !m_DeviceId)
    return false;
  
  UInt32 propertySize = 0;
  Boolean writable = false;
  OSStatus ret = AudioDeviceGetPropertyInfo(m_DeviceId, 0, false, kAudioDevicePropertyDataSources, &propertySize, &writable);
  if (ret)
    return false;
  UInt32 sources = propertySize / sizeof(UInt32);
  UInt32* pSources = new UInt32[sources];
  ret = AudioDeviceGetProperty(m_DeviceId, 0, false, kAudioDevicePropertyDataSources, &propertySize, pSources);
  if (!ret)
    for (UInt32 i = 0; i < sources; i++)
      pList->push_back(pSources[i]);;
  delete[] pSources;
  return (!ret);    
}

Float64 CCoreAudioDevice::GetNominalSampleRate()
{
  if (!m_DeviceId)
    return 0.0f;
  
  Float64 sampleRate = 0.0f;
  UInt32 size = sizeof(Float64);
  OSStatus ret = AudioDeviceGetProperty(m_DeviceId, 0, false, kAudioDevicePropertyNominalSampleRate, &size, &sampleRate);
  if (ret)
  { 
    CLog::Log(LOGERROR, "CCoreAudioUnit::GetNominalSampleRate: Unable to retrieve current device sample rate. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
    return 0.0f;
  }
  return sampleRate;
}

bool CCoreAudioDevice::SetNominalSampleRate(Float64 sampleRate)
{
  if (!m_DeviceId || sampleRate == 0.0f)
    return false;
  
  Float64 currentRate = GetNominalSampleRate();
  if (currentRate == sampleRate)
    return true; //No need to change
    
  UInt32 size = sizeof(Float64);
  OSStatus ret = AudioDeviceSetProperty(m_DeviceId, NULL, 0, false, kAudioDevicePropertyNominalSampleRate, size, &sampleRate);
  if (ret)
  { 
    CLog::Log(LOGERROR, "CCoreAudioUnit::SetNominalSampleRate: Unable to set current device sample rate to %0.0f. Error = 0x%08x (%4.4s)", (float)sampleRate, ret, CONVERT_OSSTATUS(ret));
    return false;
  }
  CLog::Log(LOGDEBUG,  "CCoreAudioUnit::SetNominalSampleRate: Changed device sample rate from %0.0f to %0.0f.", (float)currentRate, (float)sampleRate);
  if (m_SampleRateRestore == 0.0f)
    m_SampleRateRestore = currentRate;
  return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CCoreAudioStream
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CCoreAudioStream::CCoreAudioStream() :
m_StreamId(0)
{
  m_OriginalVirtualFormat.mFormatID = 0;
  m_OriginalPhysicalFormat.mFormatID = 0;
}

CCoreAudioStream::~CCoreAudioStream()
{
  Close();
}

bool CCoreAudioStream::Open(AudioStreamID streamId)
{
  m_StreamId = streamId;
  CLog::Log(LOGDEBUG, "CCoreAudioStream::Open: Opened stream 0x%04x.", m_StreamId);
  return true;
}

// TODO: Should it even be possible to change both the physical and virtual formats, since the devices do it themselves?
void CCoreAudioStream::Close()
{
  if (!m_StreamId)
    return;
  
  // Revert any format changes we made
  if (m_OriginalVirtualFormat.mFormatID && m_StreamId)
  {
    CLog::Log(LOGDEBUG, "CCoreAudioStream::Close: Restoring original virtual format for stream 0x%04x.", m_StreamId);
    SetVirtualFormat(&m_OriginalVirtualFormat);
  }
  if (m_OriginalPhysicalFormat.mFormatID && m_StreamId)
  {
    CLog::Log(LOGDEBUG, "CCoreAudioStream::Close: Restoring original physical format for stream 0x%04x.", m_StreamId);
    SetPhysicalFormat(&m_OriginalPhysicalFormat);
  }
  
  m_OriginalPhysicalFormat.mFormatID = 0;
  m_OriginalVirtualFormat.mFormatID = 0;
  CLog::Log(LOGDEBUG, "CCoreAudioStream::Close: Closed stream 0x%04x.", m_StreamId);
  m_StreamId = 0;
}

UInt32 CCoreAudioStream::GetDirection()
{
  if (!m_StreamId)
    return 0;
  UInt32 size = sizeof(UInt32);
  UInt32 val = 0;
  OSStatus ret = AudioStreamGetProperty(m_StreamId, 0, kAudioStreamPropertyDirection, &size, &val);
  if (ret)
    return 0;
  return val;
}

UInt32 CCoreAudioStream::GetTerminalType()
{
  if (!m_StreamId)
    return 0;
  UInt32 size = sizeof(UInt32);
  UInt32 val = 0;
  OSStatus ret = AudioStreamGetProperty(m_StreamId, 0, kAudioStreamPropertyTerminalType, &size, &val);
  if (ret)
    return 0;
  return val;  
}

UInt32 CCoreAudioStream::GetLatency()
{
  if (!m_StreamId)
    return 0;  
  UInt32 size = sizeof(UInt32);
  UInt32 val = 0;
  OSStatus ret = AudioStreamGetProperty(m_StreamId, 0, kAudioStreamPropertyLatency, &size, &val);
  if (ret)
    return 0;
  return val;  
}

bool CCoreAudioStream::GetVirtualFormat(AudioStreamBasicDescription* pDesc)
{
  if (!pDesc || !m_StreamId)
    return false;
  UInt32 size = sizeof(AudioStreamBasicDescription);
  OSStatus ret = AudioStreamGetProperty(m_StreamId, 0, kAudioStreamPropertyVirtualFormat, &size, pDesc);
  if (ret)
    return false;
  return true;
}

bool CCoreAudioStream::SetVirtualFormat(AudioStreamBasicDescription* pDesc)
{
  if (!pDesc || !m_StreamId)
    return false;
  if (!m_OriginalVirtualFormat.mFormatID)
  {
    if (!GetVirtualFormat(&m_OriginalVirtualFormat)) // Store the original format (as we found it) so that it can be restored later
    {
      CLog::Log(LOGERROR, "CCoreAudioStream::SetVirtualFormat: Unable to retrieve current virtual format for stream 0x%04x.", m_StreamId);
      return false;
    }
  }
  OSStatus ret = AudioStreamSetProperty(m_StreamId, NULL, 0, kAudioStreamPropertyVirtualFormat, sizeof(AudioStreamBasicDescription), pDesc);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioStream::SetVirtualFormat: Unable to set virtual format for stream 0x%04x. Error = 0x%08x (%4.4s)", m_StreamId, ret, CONVERT_OSSTATUS(ret));
    return false;
  }
  return true;   
}

bool CCoreAudioStream::GetPhysicalFormat(AudioStreamBasicDescription* pDesc)
{
  if (!pDesc || !m_StreamId)
    return false;
  UInt32 size = sizeof(AudioStreamBasicDescription);
  OSStatus ret = AudioStreamGetProperty(m_StreamId, 0, kAudioStreamPropertyPhysicalFormat, &size, pDesc);
  if (ret)
    return false;
  return true;   
}

bool CCoreAudioStream::SetPhysicalFormat(AudioStreamBasicDescription* pDesc)
{
  if (!pDesc || !m_StreamId)
    return false;
  if (!m_OriginalPhysicalFormat.mFormatID)
  {
    if (!GetPhysicalFormat(&m_OriginalPhysicalFormat)) // Store the original format (as we found it) so that it can be restored later
    {
      CLog::Log(LOGERROR, "CCoreAudioStream::SetPhysicalFormat: Unable to retrieve current physical format for stream 0x%04x.", m_StreamId);
      return false;
    }
  }  
  OSStatus ret = AudioStreamSetProperty(m_StreamId, NULL, 0, kAudioStreamPropertyPhysicalFormat, sizeof(AudioStreamBasicDescription), pDesc);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioStream::SetVirtualFormat: Unable to set physical format for stream 0x%04x. Error = 0x%08x (%4.4s)", m_StreamId, ret, CONVERT_OSSTATUS(ret));
    return false;
  }
  return true;   
}

bool CCoreAudioStream::GetAvailableVirtualFormats(StreamFormatList* pList)
{
  if (!pList || !m_StreamId)
    return false;
  
  UInt32 propertySize = 0;
  Boolean writable = false;
  OSStatus ret = AudioStreamGetPropertyInfo(m_StreamId, 0, kAudioStreamPropertyAvailableVirtualFormats, &propertySize, &writable);
  if (ret)
    return false;
  UInt32 formatCount = propertySize / sizeof(AudioStreamRangedDescription);
  AudioStreamRangedDescription* pFormatList = new AudioStreamRangedDescription[formatCount];
  ret = AudioStreamGetProperty(m_StreamId, 0, kAudioStreamPropertyAvailableVirtualFormats, &propertySize, pFormatList);
  if (!ret)
  {
    for (UInt32 format = 0; format < formatCount; format++)
      pList->push_back(pFormatList[format]);
  }
  delete[] pFormatList;
  return (ret == noErr);
}

bool CCoreAudioStream::GetAvailablePhysicalFormats(StreamFormatList* pList)
{
  if (!pList || !m_StreamId)
    return false;
  
  UInt32 propertySize = 0;
  Boolean writable = false;
  OSStatus ret = AudioStreamGetPropertyInfo(m_StreamId, 0, kAudioStreamPropertyAvailablePhysicalFormats, &propertySize, &writable);
  if (ret)
    return false;
  UInt32 formatCount = propertySize / sizeof(AudioStreamRangedDescription);
  AudioStreamRangedDescription* pFormatList = new AudioStreamRangedDescription[formatCount];
  ret = AudioStreamGetProperty(m_StreamId, 0, kAudioStreamPropertyAvailablePhysicalFormats, &propertySize, pFormatList);
  if (!ret)
  {
    for (UInt32 format = 0; format < formatCount; format++)
      pList->push_back(pFormatList[format]);
  }
  delete[] pFormatList;
  return (ret == noErr);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CCoreAudioUnit
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CCoreAudioUnit::CCoreAudioUnit() :
  m_Initialized(false),
  m_Component(NULL)
{
  
}

CCoreAudioUnit::~CCoreAudioUnit() 
{
  Close();
}

bool CCoreAudioUnit::Open(ComponentDescription desc)
{
  if (m_Component)
    Close();
  
  // Find the required Component
	Component outputComp = FindNextComponent(NULL, &desc);
	if (outputComp == NULL)  // Unable to find the AudioUnit we requested
  { 
    CLog::Log(LOGERROR, "CCoreAudioUnit::Open: Unable to locate AudioUnit Component.");
    return false;
  }
	
  // Create an instance of the AudioUnit Component
  OSStatus ret = OpenAComponent(outputComp, &m_Component);
	if (ret) // Unable to open AudioUnit
  { 
    CLog::Log(LOGERROR, "CCoreAudioUnit::Open: Unable to open AudioUnit Component. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
    return false; 
  }
  return true;
}

void CCoreAudioUnit::Close()
{
  if (m_Initialized)
    AudioUnitUninitialize(m_Component);
  if (m_Component)
    CloseComponent(m_Component);
  m_Initialized = false;
  m_Component = 0;
}

bool CCoreAudioUnit::Initialize()
{
  if (!m_Component)
    return false;
  
  OSStatus ret = AudioUnitInitialize(m_Component);
  if (ret)
  { 
    CLog::Log(LOGERROR, "CCoreAudioUnit::Initialize: Unable to Initialize AudioUnit. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
    return false; 
  } 
  m_Initialized = true;
  return true;
}


bool CCoreAudioUnit::GetInputFormat(AudioStreamBasicDescription* pDesc)
{
  if (!m_Component || !pDesc)
    return false;
  
  UInt32 size = sizeof(AudioStreamBasicDescription);
  OSStatus ret = AudioUnitGetProperty(m_Component, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, pDesc, &size);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::GetInputFormat: Unable to get AudioUnit input format. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
    return false;
  }
  return true;
}

bool CCoreAudioUnit::GetOutputFormat(AudioStreamBasicDescription* pDesc)
{
  if (!m_Component || !pDesc)
    return false;
  
  UInt32 size = sizeof(AudioStreamBasicDescription);
  OSStatus ret = AudioUnitGetProperty(m_Component, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, pDesc, &size);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::GetInputFormat: Unable to get AudioUnit output format. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
    return false;
  }
  return true;
}

bool CCoreAudioUnit::SetInputFormat(AudioStreamBasicDescription* pDesc)
{
  if (!m_Component || !pDesc)
    return false;
  
  OSStatus ret = AudioUnitSetProperty(m_Component, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, pDesc, sizeof(AudioStreamBasicDescription));
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::SetInputFormat: Unable to set AudioUnit input format. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
    return false;
  }
  return true;  
}

bool CCoreAudioUnit::SetOutputFormat(AudioStreamBasicDescription* pDesc)
{
  if (!m_Component || !pDesc)
    return false;
  
  OSStatus ret = AudioUnitSetProperty(m_Component, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, pDesc, sizeof(AudioStreamBasicDescription));
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::SetInputFormat: Unable to set AudioUnit output format. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
    return false;
  }
  return true;  
}

bool CCoreAudioUnit::SetRenderProc(AURenderCallback callback, void* pClientData)
{
  if (!m_Component)
    return false;
  
  AURenderCallbackStruct callbackInfo;
	callbackInfo.inputProc = callback; // Function to be called each time the AudioUnit needs data
	callbackInfo.inputProcRefCon = pClientData; // Pointer to be returned in the callback proc
	OSStatus ret = AudioUnitSetProperty(m_Component, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &callbackInfo, sizeof(AURenderCallbackStruct));
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::SetRenderProc: Unable to set AudioUnit render callback. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
    return false;
  }
  return true;
}

UInt32 CCoreAudioUnit::GetBufferFrameSize()
{
  if (!m_Component)
    return 0;
  
  UInt32 size = sizeof(UInt32);
  UInt32 bufferSize = 0;
  OSStatus ret = AudioUnitGetProperty(m_Component, kAudioDevicePropertyBufferFrameSize, kAudioUnitScope_Input, 0, &bufferSize, &size);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::GetBufferFrameSize: Unable to get current device's buffer size. ErrCode = Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
    return 0;
  }
  return bufferSize;
}

bool CCoreAudioUnit::SetMaxFramesPerSlice(UInt32 maxFrames)
{
  if (!m_Component)
    return false;
  
	OSStatus ret = AudioUnitSetProperty(m_Component, kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Global, 0, &maxFrames, sizeof(UInt32));
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::SetMaxFramesPerSlice: Unable to set AudioUnit max frames per slice. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
    return false;
  }
  return true;  
}

// TODO: These are not true AudioUnit methods. Move to OutputDeviceAU class
// TODO: The channel map setter/getter are inefficient

bool CCoreAudioUnit::SetCurrentDevice(AudioDeviceID deviceId)
{
  if (!m_Component)
    return false;
  
  OSStatus ret = AudioUnitSetProperty(m_Component, kAudioOutputUnitProperty_CurrentDevice, kAudioUnitScope_Global, 0, &deviceId, sizeof(AudioDeviceID));
  if (ret)
  { 
    CLog::Log(LOGERROR, "CCoreAudioUnit::SetCurrentDevice: Unable to set current device. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
    return false; 
  }  
  return true;
}

bool CCoreAudioUnit::GetInputChannelMap(CoreAudioChannelList* pChannelMap)
{
  if (!m_Component)
    return false;
  
  UInt32 size = 0;
  Boolean writable = false;
  AudioUnitGetPropertyInfo(m_Component, kAudioOutputUnitProperty_ChannelMap, kAudioUnitScope_Input, 0, &size, &writable);
  UInt32 channels = size/sizeof(SInt32);
  SInt32* pMap = new SInt32[channels];
  OSStatus ret = AudioUnitGetProperty(m_Component, kAudioOutputUnitProperty_ChannelMap, kAudioUnitScope_Input, 0, pMap, &size);
  if (ret)
    CLog::Log(LOGERROR, "CCoreAudioUnit::GetInputChannelMap: Unable to retrieve AudioUnit input channel map. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
  else
    for (UInt32 i = 0; i < channels; i++)
      pChannelMap->push_back(pMap[i]);  
  delete[] pMap;
  return (!ret);
}

bool CCoreAudioUnit::SetInputChannelMap(CoreAudioChannelList* pChannelMap)
{
  if (!m_Component || !pChannelMap)
    return false;
  UInt32 channels = pChannelMap->size();
  UInt32 size = sizeof(SInt32) * channels;
  SInt32* pMap = new SInt32[channels];
  for (UInt32 i = 0; i < channels; i++)
    pMap[i] = (*pChannelMap)[i];
  OSStatus ret = AudioUnitGetProperty(m_Component, kAudioOutputUnitProperty_ChannelMap, kAudioUnitScope_Input, 0, pMap, &size);
  if (ret)
    CLog::Log(LOGERROR, "CCoreAudioUnit::GetBufferFrameSize: Unable to get current device's buffer size. ErrCode = Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
  delete[] pMap;
  return (!ret);
}

void CCoreAudioUnit::Start()
{
  // TODO: Check component status
  if (m_Component)
    AudioOutputUnitStart(m_Component);  
}

void CCoreAudioUnit::Stop()
{
  // TODO: Check component status
  if (m_Component)
    AudioOutputUnitStop(m_Component);    
}

Float32 CCoreAudioUnit::GetCurrentVolume()
{
  if (!m_Component)
    return 0.0f;
  
  Float32 volPct = 0.0f;
  OSStatus ret = AudioUnitGetParameter(m_Component,  kHALOutputParam_Volume, kAudioUnitScope_Global, 0, &volPct);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::GetCurrentVolume: Unable to get AudioUnit volume. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
    return 0.0f;
  }
  return volPct;
}

bool CCoreAudioUnit::SetCurrentVolume(Float32 vol)
{
  if (!m_Component)
    return false;
  
  OSStatus ret = AudioUnitSetParameter(m_Component, kHALOutputParam_Volume, kAudioUnitScope_Global, 0, vol, 0);
  if (ret)
  {
    CLog::Log(LOGERROR, "CCoreAudioUnit::SetCurrentVolume: Unable to set AudioUnit volume. Error = 0x%08x (%4.4s)", ret, CONVERT_OSSTATUS(ret));
    return false;
  }
  return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CCoreAudioSound
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CCoreAudioSound::CCoreAudioSound() :
  m_pBufferList(NULL)
{

}

CCoreAudioSound::~CCoreAudioSound()
{
  Unload();
}

void WriteInterleavedSinWave(int channels, int samples, AudioBufferList** ppBufferList)
{
  AudioBufferList* pBufferList = *ppBufferList = (AudioBufferList*)calloc(1, sizeof(AudioBufferList));
  pBufferList->mNumberBuffers = 1;
  short* pBuffer = (short*)malloc(sizeof(short) * samples * channels);
  pBufferList->mBuffers[0].mNumberChannels = channels;
  pBufferList->mBuffers[0].mData = pBuffer;
  pBufferList->mBuffers[0].mDataByteSize = sizeof(short) * samples * channels;

  float pi = 3.1415926535897;
  float scale = (2.0f * pi) / 100.0f;
  for (int s = 0; s < samples; s++)
  {
    int smp = s % 100;
    float pos = (float)smp * scale;
    short val = (short)(sin(pos) * 32767.0f);
    for (int c = 0; c < channels; c++)
      pBuffer[s*channels+c] = val;
  }  
}

void WriteDeInterleavedSinWave(int channelCount, int samples, AudioBufferList** ppBufferList)
{
  AudioBufferList* pBufferList = *ppBufferList = (AudioBufferList*)calloc(1, sizeof(AudioBufferList) + sizeof(AudioBuffer) * (channelCount - kVariableLengthArray) );
  pBufferList->mNumberBuffers = channelCount;
  float* buffers = (float*)malloc(sizeof(float) * samples * channelCount);
  for(int i = 0; i < channelCount; i++)
  {
    pBufferList->mBuffers[i].mNumberChannels = 1;
    pBufferList->mBuffers[i].mData = buffers + (samples * i);
    pBufferList->mBuffers[i].mDataByteSize = samples * sizeof(float);
    
    float* pSamples = (float*)pBufferList->mBuffers[i].mData;
    float pi = 3.1415926535897;
    float scale = (2.0f * pi) / 100.0f;
    for (int s = 0; s < samples; s++)
    {
      int smp = s % 100;
      float pos = (float)smp * scale;
      pSamples[s] = sin(pos);      
    }
  }
}

bool CCoreAudioSound::LoadFile(CStdString fileName)
{
  //m_TotalFrames = 4000;
  //WriteInterleavedSinWave(2, m_TotalFrames, &m_pBufferList);
  //WriteDeInterleavedSinWave(2, m_TotalFrames, &m_pBufferList);
  //return true;
  
  // Validate the provided file path and open the audio file
  FSRef fileRef;
  UInt32 size = 0;
  ExtAudioFileRef audioFile;
  OSStatus ret = FSPathMakeRef((const UInt8*) fileName.c_str(), &fileRef, false);
  ret = ExtAudioFileOpen(&fileRef, &audioFile);
  
  // Retrieve the format of the source file
  AudioStreamBasicDescription inputFormat;
  size = sizeof(inputFormat);
  ExtAudioFileGetProperty(audioFile, kExtAudioFileProperty_FileDataFormat, &size, &inputFormat);
  
  // Set up format conversion
  // Our output stream is fixed at 32-bit native float, deinterlaced x 2-channels.
  // Fix output sample-rate at 44100, since multiple slices can be scheduled sequesntially 
  AudioStreamBasicDescription outputFormat;
  outputFormat.mSampleRate = 44100;
  outputFormat.mChannelsPerFrame  = 2;
  outputFormat.mFormatID = kAudioFormatLinearPCM;
  outputFormat.mFormatFlags = kAudioFormatFlagsNativeFloatPacked | kAudioFormatFlagIsNonInterleaved; // Format required by scheduled sound player
  outputFormat.mBitsPerChannel = 8 * sizeof(float);
  outputFormat.mBytesPerFrame =  sizeof(float);
  outputFormat.mFramesPerPacket = 1;
  outputFormat.mBytesPerPacket = outputFormat.mBytesPerFrame * outputFormat.mFramesPerPacket;
  ExtAudioFileSetProperty(audioFile, kExtAudioFileProperty_ClientDataFormat, sizeof(AudioStreamBasicDescription), &outputFormat);

  // Determine file size (in terms of the file's sample-rate, not the output sample-rate)
  size = sizeof(m_TotalFrames);
  ExtAudioFileGetProperty(audioFile, kExtAudioFileProperty_FileLengthFrames, &size, &m_TotalFrames);
  
  // Calculate the total number of converted frames to be read
  m_TotalFrames *= (float)outputFormat.mSampleRate / (float)inputFormat.mSampleRate; // TODO: Verify the accuracy of this
  
  // Allocate AudioBuffers
  UInt32 channelCount = outputFormat.mChannelsPerFrame;
  m_pBufferList = (AudioBufferList*)calloc(1, sizeof(AudioBufferList) + sizeof(AudioBuffer) * (channelCount - kVariableLengthArray));
  m_pBufferList->mNumberBuffers = channelCount; // One buffer per channel for deinterlaced pcm
  UInt32 framesPerBuffer = (UInt32)m_TotalFrames;
  float* buffers = (float*)calloc(1, sizeof(float) * framesPerBuffer * channelCount);
  for(int i = 0; i < channelCount; i++)
  {
    m_pBufferList->mBuffers[i].mNumberChannels = 1; // One channel per buffer for deinterlaced pcm
    m_pBufferList->mBuffers[i].mData = buffers + (framesPerBuffer * i);
    m_pBufferList->mBuffers[i].mDataByteSize = framesPerBuffer * sizeof(float);  
  }
  
  // Read the entire file
  // TODO: Limit the total file length?
  ExtAudioFileRead(audioFile, &framesPerBuffer, m_pBufferList);
  if (framesPerBuffer != m_TotalFrames)
  {
    CLog::Log(LOGERROR, "Unable to read entire file");
    m_TotalFrames = framesPerBuffer;
  }
  
  if (inputFormat.mChannelsPerFrame == 1) // Copy Left channel into Right if the source file is Mono
    memcpy(m_pBufferList->mBuffers[1].mData, m_pBufferList->mBuffers[1].mData, m_pBufferList->mBuffers[1].mDataByteSize);
  ExtAudioFileDispose(audioFile);

  return true; 
}

void CCoreAudioSound::Unload()
{
  if (m_pBufferList)
  {
    free(m_pBufferList->mBuffers[0].mData); // This will free all the buffers, since they were allocated together
    free(m_pBufferList);
  }
  m_pBufferList = NULL;
}

void CCoreAudioSound::SetVolume(float volume)
{
  
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CCoreAudioSoundManager
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CCoreAudioSoundManager::CCoreAudioSoundManager()
{
  m_pCurrentSound = NULL;
}

CCoreAudioSoundManager::~CCoreAudioSoundManager()
{
  
}

bool CCoreAudioSoundManager::Initialize(CStdString deviceName)
{
  // Attempt to find the configured output device
  AudioDeviceID outputDevice = CCoreAudioHardware::FindAudioDevice(deviceName);
  if (!outputDevice) // Fall back to the default device if no match is found
  {
    CLog::Log(LOGWARNING, "CCoreAudioSoundManager::Initialize: Unable to locate configured device, falling-back to the system default.");
    outputDevice = CCoreAudioHardware::GetDefaultOutputDevice();
    if (!outputDevice) // Not a lot to be done with no device.
      return false;
  }
  
  // Attach our output object to the device
  m_OutputDevice.Open(outputDevice);
  
  // Create the Output AudioUnit Component
  ComponentDescription outputCompDesc;
  outputCompDesc.componentType = kAudioUnitType_Output;
  outputCompDesc.componentSubType = kAudioUnitSubType_HALOutput;
  outputCompDesc.componentManufacturer = kAudioUnitManufacturer_Apple;
  outputCompDesc.componentFlags = 0;
  outputCompDesc.componentFlagsMask = 0;
  
  if (!m_OutputUnit.Open(outputCompDesc))
    return false;
  
  // Hook the Ouput AudioUnit to the selected device
  if (!m_OutputUnit.SetCurrentDevice(outputDevice))
    return false;

  // Set up output format (32-bit float, 2-channel, non-interleaved)
  m_OutputFormat.mSampleRate = 44100.0;
  m_OutputFormat.mChannelsPerFrame  = 2;
  m_OutputFormat.mFormatID = kAudioFormatLinearPCM;
  m_OutputFormat.mFormatFlags = kAudioFormatFlagsNativeFloatPacked | kAudioFormatFlagIsNonInterleaved;
  m_OutputFormat.mBitsPerChannel = 8 * sizeof(float);
  m_OutputFormat.mBytesPerFrame =  sizeof(float);
  m_OutputFormat.mFramesPerPacket = 1;
  m_OutputFormat.mBytesPerPacket = m_OutputFormat.mBytesPerFrame * m_OutputFormat.mFramesPerPacket;  
  if (!m_OutputUnit.SetInputFormat(&m_OutputFormat))
    return false;

  // Configure the maximum number of frames that the AudioUnit will ask to process at one time.
  // If this is not called, there is no guarantee that the callback will ever be called.
  UInt32 bufferFrames = m_OutputUnit.GetBufferFrameSize(); // Size of the output buffer, in Frames
  if (!m_OutputUnit.SetMaxFramesPerSlice(bufferFrames))
    return false;
  
  // Setup the callback function that the AudioUnit will use to request data	
  if (!m_OutputUnit.SetRenderProc(RenderCallback, this))
    return false;
  
  // Initialize the Output AudioUnit
  if (!m_OutputUnit.Initialize())
    return false;  
  
  return true;
}

void CCoreAudioSoundManager::Run()
{
  m_OutputUnit.Start();
}

void CCoreAudioSoundManager::Stop()
{
  // TODO: Empty event queue and reset state
  m_OutputUnit.Stop();
}

CoreAudioSoundRef CCoreAudioSoundManager::RegisterSound(CStdString fileName)
{  
  CCoreAudioSound* pSound = new CCoreAudioSound();
  if (!pSound->LoadFile(fileName))
  {
    delete pSound;
    return NULL;
  }
  return pSound;
}

void CCoreAudioSoundManager::UnregisterSound(CoreAudioSoundRef soundRef)
{
  delete soundRef;
}

void CCoreAudioSoundManager::PlaySound(CoreAudioSoundRef soundRef)
{
  if (!m_pCurrentSound)
  {
    m_OutputUnit.Start();
    m_pCurrentSound = soundRef;
    m_CurrentOffset = 0;
  }
  // TODO: Add to queue
}

OSStatus CCoreAudioSoundManager::OnRender(AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData)
{
  if (m_pCurrentSound)
  {
    if (m_CurrentOffset < m_pCurrentSound->GetTotalFrames())
    {
      if (m_pCurrentSound->GetTotalFrames() - m_CurrentOffset < inNumberFrames)
        inNumberFrames = m_pCurrentSound->GetTotalFrames() - m_CurrentOffset;
      for (int i = 0; i < ioData->mNumberBuffers; i++)
      {
        UInt32 frameLen = m_OutputFormat.mBytesPerFrame;
        unsigned char* pIn = (unsigned char*)m_pCurrentSound->GetBufferList()->mBuffers[i].mData;
        memcpy(ioData->mBuffers[i].mData, &pIn[m_CurrentOffset * frameLen], inNumberFrames * frameLen); // Copy out the requested number of frames
      }
      m_CurrentOffset += inNumberFrames;
    }
    else
    {
      m_pCurrentSound = NULL;
      m_CurrentOffset = 0;
    }
  }
  else
  {
    ioData->mBuffers[0].mDataByteSize = ioData->mBuffers[0].mDataByteSize = 0;
  }
  return noErr;
}

OSStatus CCoreAudioSoundManager::RenderCallback(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData)
{
  // Hand over to instance memeber
  return ((CCoreAudioSoundManager*)inRefCon)->OnRender(ioActionFlags, inTimeStamp, inBusNumber, inNumberFrames, ioData);
}

#endif