
#include "../../../stdafx.h"
#include "DVDInputStreamNavigator.h"
#include "..\..\..\util.h"
#include "..\..\..\LangCodeExpander.h"
#include "..\DVDDemuxSPU.h"
#include "..\DVDOverlay.h"

CDVDInputStreamNavigator::CDVDInputStreamNavigator(IDVDPlayer* player) : CDVDInputStream()
{
  m_streamType = DVDSTREAM_TYPE_DVD;
  m_dvdnav = 0;
  m_pDVDPlayer = player;
  InitializeCriticalSection(&m_critSection);
  m_bCheckButtons = false;
  m_bDiscardHop = false;
}

CDVDInputStreamNavigator::~CDVDInputStreamNavigator()
{
  Close();
  DeleteCriticalSection(&m_critSection);
}

bool CDVDInputStreamNavigator::Open(const char* strFile)
{
  char* strDVDFile;
  
  if (!CDVDInputStream::Open(strFile)) return false;
  
  // load libdvdnav.dll
  if (!m_dll.Load())
    return false;
  
  // load the dvd language codes
  // g_LangCodeExpander.LoadStandardCodes();

  // since libdvdnav automaticly play's the dvd if the directory contains VIDEO_TS.IFO
  // we strip it here.
  strDVDFile = strdup(strFile);
  if (strnicmp(strDVDFile + strlen(strDVDFile) - 12, "VIDEO_TS.IFO", 12) == 0)
  {
    strDVDFile[strlen(strDVDFile) - 13] = '\0';
  }

  // open up the DVD device
  if (m_dll.dvdnav_open(&m_dvdnav, strDVDFile) != DVDNAV_STATUS_OK)
  {
    free(strDVDFile);

    CLog::DebugLog("Error on dvdnav_open\n");
    Close();
    return false;
  }
  free(strDVDFile);

  vm_t* vm = m_dll.dvdnav_get_vm(m_dvdnav);
  if (vm && vm->vmgi && vm->vmgi->vmgi_mat)
  {
    // find out what region dvd reports itself to be from, and use that as mask if available
    int mask = ~(vm->vmgi->vmgi_mat->vmg_category >> 16);
    if( mask != 0 )
      m_dll.dvdnav_set_region_mask(m_dvdnav, mask);
    else
      m_dll.dvdnav_set_region_mask(m_dvdnav, 0xff);
  }
  else
  {
    // set region flag (0xff = all regions ?)
    m_dll.dvdnav_set_region_mask(m_dvdnav, 0xff);
  }

  // set defaults
  if (m_dll.dvdnav_menu_language_select(m_dvdnav, "en") != DVDNAV_STATUS_OK ||
      m_dll.dvdnav_audio_language_select(m_dvdnav, "en") != DVDNAV_STATUS_OK ||
      m_dll.dvdnav_spu_language_select(m_dvdnav, "en") != DVDNAV_STATUS_OK)
  {
    CLog::DebugLog("Error on setting languages: %s\n", m_dll.dvdnav_err_to_string(m_dvdnav));
    Close();
    return false;
  }

  // set read ahead cache usage
  if (m_dll.dvdnav_set_readahead_flag(m_dvdnav, 1) != DVDNAV_STATUS_OK)
  {
    CLog::DebugLog("Error on dvdnav_set_readahead_flag: %s\n", m_dll.dvdnav_err_to_string(m_dvdnav));
    Close();
    return false;
  }

  // set the PGC positioning flag to have position information relatively to the
  // whole feature instead of just relatively to the current chapter
  if (m_dll.dvdnav_set_PGC_positioning_flag(m_dvdnav, 1) != DVDNAV_STATUS_OK)
  {
    CLog::DebugLog("Error on dvdnav_set_PGC_positioning_flag: %s\n", m_dll.dvdnav_err_to_string(m_dvdnav));
    Close();
    return false;
  }

  m_bStopped = false;
  
  return true;
}

void CDVDInputStreamNavigator::Close()
{
  if (!m_dvdnav) return;

  // finish off by closing the dvdnav device
  if (m_dll.dvdnav_close(m_dvdnav) != DVDNAV_STATUS_OK)
  {
    CLog::DebugLog("Error on dvdnav_close: %s\n", m_dll.dvdnav_err_to_string(m_dvdnav));
    return ;
  }

  CDVDInputStream::Close();
  m_dvdnav = NULL;
}

int CDVDInputStreamNavigator::Read(BYTE* buf, int buf_size)
{
  if (!m_dvdnav || m_bStopped) return -1;
  if (buf_size < DVD_VIDEO_BLOCKSIZE)
  {
    CLog::Log(LOGERROR, "CDVDInputStreamNavigator: buffer size is to small, %d bytes, should be 2048 bytes", buf_size);
    return -1;
  }
  
  int navresult;
  int iBytesRead;

  do
  {
    navresult = ProcessBlock(buf, &iBytesRead);
    if (navresult == DVDNAV_STILL_FRAME) return 0; // return 0 bytes read;
    if (navresult == DVDNAV_STOP || navresult == -1) return -1;
  }
  while (navresult != DVDNAV_BLOCK_OK);

  return iBytesRead;
}

// not working yet, but it is the recommanded way for seeking
__int64 CDVDInputStreamNavigator::Seek(__int64 offset, int whence)
{
  if (!m_dvdnav) return -1;  
  uint32_t pos=0, len=1;
  if (m_dll.dvdnav_sector_search(m_dvdnav, (uint64_t)(offset / DVD_VIDEO_LB_LEN), (int32_t)whence) != DVDNAV_STATUS_ERR)
  {
    m_dll.dvdnav_get_position(m_dvdnav, &pos, &len);
  }
  else
  {
    CLog::Log(LOGDEBUG, "dvdnav: %s", m_dll.dvdnav_err_to_string(m_dvdnav));
    return -1;
  }
  
  return (int)(pos * DVD_VIDEO_LB_LEN);
}

int CDVDInputStreamNavigator::ProcessBlock(BYTE* dest_buffer, int* read)
{
  if (!m_dvdnav) return -1;

  int result;
  int event;
  int len;
  int iNavresult;

  bool bFinished = false;
  uint8_t* buf = m_tempbuffer;
  iNavresult = -1;
  
  while (!bFinished)
  {
    // the main reading function
    result = m_dll.dvdnav_get_next_cache_block(m_dvdnav, &buf, &event, &len);

    if (result == DVDNAV_STATUS_ERR)
    {
      CLog::DebugLog("Error getting next block: %s\n", m_dll.dvdnav_err_to_string(m_dvdnav));
      iNavresult = DVDNAV_STATUS_ERR;
      bFinished = true;
    }

    switch (event)
    {
    case DVDNAV_BLOCK_OK:
      {
        // We have received a regular block of the currently playing MPEG stream.
        // buf contains the data and len its length (obviously!) (which is always 2048 bytes btw)
        fast_memcpy(dest_buffer, buf, len);
        *read = len;
        iNavresult = DVDNAV_BLOCK_OK;
        bFinished = true;
      }
      break;

    case DVDNAV_NOP:
      // Nothing to do here.
      break;

    case DVDNAV_STILL_FRAME:
      {
        // We have reached a still frame. A real player application would wait
        // the amount of time specified by the still's length while still handling
        // user input to make menus and other interactive stills work.
        // A length of 0xff means an indefinite still which has to be skipped
        // indirectly by some user interaction.
        int navres = m_pDVDPlayer->OnDVDNavResult(buf, DVDNAV_STILL_FRAME);
        iNavresult = DVDNAV_STILL_FRAME;
        if (navres == NAVRESULT_STILL_NOT_SKIPPED) bFinished = true;
      }
      break;

    case DVDNAV_WAIT:
      {
        // We have reached a point in DVD playback, where timing is critical.
        // Player application with internal fifos can introduce state
        // inconsistencies, because libdvdnav is always the fifo's length
        // ahead in the stream compared to what the application sees.
        // Such applications should wait until their fifos are empty
        // when they receive this type of event.

        // xbmc, we don't use any fifos at this place
        SkipWait();
      }
      break;

    case DVDNAV_SPU_CLUT_CHANGE:
      // Player applications should pass the new colour lookup table to their
      // SPU decoder. The CLUT is given as 16 uint32_t's in the buffer.
      {
        m_pDVDPlayer->OnDVDNavResult(buf, DVDNAV_SPU_CLUT_CHANGE);
      }
      break;

    case DVDNAV_SPU_STREAM_CHANGE:
      // Player applications should inform their SPU decoder to switch channels
      {
        dvdnav_spu_stream_change_event_t* event = (dvdnav_spu_stream_change_event_t*)buf;
        
        //libdvdnav never sets logical, why.. don't know..
        event->logical = GetActiveSubtitleStream();

        m_bCheckButtons = true;
        m_pDVDPlayer->OnDVDNavResult(buf, DVDNAV_SPU_STREAM_CHANGE);
      }
      break;

    case DVDNAV_AUDIO_STREAM_CHANGE:
      // Player applications should inform their audio decoder to switch channels
      {
        
        //dvdnav_get_audio_logical_stream actually does the oposite to the docs.. 
        //taking a audiostream as given on dvd, it gives the physical stream that
        //refers to in the mpeg file

        dvdnav_audio_stream_change_event_t* event = (dvdnav_audio_stream_change_event_t*)buf;
        
        //wroong... stupid docs..
        //event->logical = dvdnav_get_audio_logical_stream(m_dvdnav, event->physical);
        //logical should actually be set to the (vm->state).AST_REG

        event->logical = GetActiveAudioStream();
        
        m_pDVDPlayer->OnDVDNavResult(m_tempbuffer, DVDNAV_AUDIO_STREAM_CHANGE);
      }

      break;

    case DVDNAV_HIGHLIGHT:
      {
        m_pDVDPlayer->OnDVDNavResult(buf, DVDNAV_HIGHLIGHT);
      }
      break;

    case DVDNAV_VTS_CHANGE:
      // Some status information like video aspect and video scale permissions do
      // not change inside a VTS. Therefore this event can be used to query such
      // information only when necessary and update the decoding/displaying
      // accordingly.
      {
        m_pDVDPlayer->OnDVDNavResult(buf, DVDNAV_VTS_CHANGE);
        iNavresult = -1; // return read error
        bFinished = true;
      }
      break;

    case DVDNAV_CELL_CHANGE:
      {
        // Some status information like the current Title and Part numbers do not
        // change inside a cell. Therefore this event can be used to query such
        // information only when necessary and update the decoding/displaying
        // accordingly.
        int tt = 0, ptt = 0;
        uint32_t pos, len;
        char input = '\0';

        m_dll.dvdnav_current_title_info(m_dvdnav, &tt, &ptt);
        m_dll.dvdnav_get_position(m_dvdnav, &pos, &len);
        CLog::DebugLog("Cell change: Title %d, Chapter %d\n", tt, ptt);
        CLog::DebugLog("At position %.0f%% inside the feature\n", 100 * (double)pos / (double)len);

        //Get total segment time
        vm_t* vm = m_dll.dvdnav_get_vm(m_dvdnav);        
        if (vm)
        {
          m_iTotalTime = vm->state.pgc->playback_time.hour * 3600;
          m_iTotalTime += vm->state.pgc->playback_time.minute * 60;
          m_iTotalTime += vm->state.pgc->playback_time.second;
          m_iTotalTime *= 1000;
        }

        m_pDVDPlayer->OnDVDNavResult(buf, DVDNAV_CELL_CHANGE);
      }
      break;

    case DVDNAV_NAV_PACKET:
      {
        // A NAV packet provides PTS discontinuity information, angle linking information and
        // button definitions for DVD menus. Angles are handled completely inside libdvdnav.
        // For the menus to work, the NAV packet information has to be passed to the overlay
        // engine of the player so that it knows the dimensions of the button areas.

        // Applications with fifos should not use these functions to retrieve NAV packets,
        // they should implement their own NAV handling, because the packet you get from these
        // functions will already be ahead in the stream which can cause state inconsistencies.
        // Applications with fifos should therefore pass the NAV packet through the fifo
        // and decoding pipeline just like any other data.

        // Calculate current time
        unsigned int pos, len;
        m_dll.dvdnav_get_position(m_dvdnav, &pos, &len);
        m_iTime = (int)(((__int64)m_iTotalTime * pos) / len);

        if (m_bCheckButtons)
        {
          CheckButtons();
          m_bCheckButtons = false;
        }
        pci_t* pci = m_dll.dvdnav_get_current_nav_pci(m_dvdnav);
        
        m_pDVDPlayer->OnDVDNavResult((void*)pci, DVDNAV_NAV_PACKET);
      }
      break;

    case DVDNAV_HOP_CHANNEL:
      // This event is issued whenever a non-seamless operation has been executed.
      // Applications with fifos should drop the fifos content to speed up responsiveness.
      {
        if (!m_bDiscardHop)
        {
          m_pDVDPlayer->OnDVDNavResult(NULL, DVDNAV_HOP_CHANNEL);
        }
        //Reset skip flag.
        m_bDiscardHop = false;
        
        iNavresult = -1; // return read error
        bFinished = true;
      }
      break;

    case DVDNAV_STOP:
      {
        // Playback should end here.
        
        // don't read any further, it could be libdvdnav had some problems reading
        // the disc. reading further results in a crash
        m_bStopped = true;
        
        m_pDVDPlayer->OnDVDNavResult(NULL, DVDNAV_STOP);
        iNavresult = DVDNAV_STOP;
        bFinished = true;
      }
      break;

    default:
      {
        CLog::DebugLog("CDVDInputStreamNavigator: Unknown event (%i)\n", event);
      }
      break;

    }

    m_dll.dvdnav_free_cache_block(m_dvdnav, buf);
  }
  return iNavresult;
}

bool CDVDInputStreamNavigator::SetActiveAudioStream(int iId)
{
  if (m_dvdnav)
  {
    return (DVDNAV_STATUS_OK == m_dll.dvdnav_audio_change(m_dvdnav, iId));
  }
  return false; 
  /*
  vm_t* vm = m_dll.dvdnav_get_vm(m_dvdnav);

  if((vm->state).domain != VTS_DOMAIN)
    return false;


  int iStreams = vm->vtsi->vtsi_mat->nr_of_vts_audio_streams;
  for( int audioN = 0; audioN < iStreams; audioN++ )
  {
    if((vm->state).pgc->audio_control[audioN] & (1<<15))
    {
      int streamN = ((vm->state).pgc->audio_control[audioN] >> 8) & 0x07;
      if( streamN == iPhysicalId )
      {
        (vm->state).AST_REG = audioN;
        return true;
      }
    }
  }
  return false;*/
}

bool CDVDInputStreamNavigator::SetActiveSubtitleStream(int iId)
{
  if (m_dvdnav)
  {
    return (DVDNAV_STATUS_OK == m_dll.dvdnav_subpicture_change(m_dvdnav, iId));
  }
  return false; 
/*
  vm_t* vm = m_dll.dvdnav_get_vm(m_dvdnav);

  if((vm->state).domain != VTS_DOMAIN)
    return false;


  int iStreams = vm->vtsi->vtsi_mat->nr_of_vts_subp_streams;
  for( int subpN = 0; subpN < iStreams; subpN++ )
  {
    if((vm->state).pgc->subp_control[subpN] & (1<<31))
    {
      int streamN0 = ((vm->state).pgc->subp_control[subpN] >> 16) & 0x1f;
      int streamN1 = ((vm->state).pgc->subp_control[subpN] >> 8) & 0x1f;
      int streamN2 = (vm->state).pgc->subp_control[subpN] & 0x1f;
      if( streamN0 == iId || streamN1 == iId || streamN2 == iId )
      {
        (vm->state).SPST_REG = subpN;
        return true;
      }
    }
  }
  
  return false;*/
}

void CDVDInputStreamNavigator::ActivateButton()
{
  if (m_dvdnav)
  {
    //m_bDiscardHop = true;
    m_dll.dvdnav_button_activate(m_dvdnav, m_dll.dvdnav_get_current_nav_pci(m_dvdnav));
  }
}

void CDVDInputStreamNavigator::SelectButton(int iButton)
{
  if (!m_dvdnav) return;
  m_dll.dvdnav_button_select(m_dvdnav, m_dll.dvdnav_get_current_nav_pci(m_dvdnav), iButton);
}

int CDVDInputStreamNavigator::GetCurrentButton()
{
  int button;
  if (m_dvdnav)
  {
    m_dll.dvdnav_get_current_highlight(m_dvdnav, &button);
    return button;
  }
  return -1;
}

void CDVDInputStreamNavigator::CheckButtons()
{
  if (m_dvdnav)
  {
    pci_t* pci = m_dll.dvdnav_get_current_nav_pci(m_dvdnav);
    int iCurrentButton = GetCurrentButton();
    btni_t* button = &(pci->hli.btnit[iCurrentButton]);
    
    // menu buttons are always cropped overlays, so if there is no such information
    // we assume the button is invalid
    if (!(button->x_start || button->x_end || button->y_start || button->y_end))
    {
      for (int i = 0; i < 36; i++)
      {
        // select first valid button.
        if (pci->hli.btnit[i].x_start ||
            pci->hli.btnit[i].x_end ||
            pci->hli.btnit[i].y_start ||
            pci->hli.btnit[i].y_end)
        {
          CLog::Log(LOGWARNING, "CDVDInputStreamNavigator: found invalid button(%d)", iCurrentButton);
          CLog::Log(LOGWARNING, "CDVDInputStreamNavigator: switching to button(%d) instead", i + 1);
          m_dll.dvdnav_button_select(m_dvdnav, pci, i + 1);
          break;
        }
      }
    }
  }
}

int CDVDInputStreamNavigator::GetTotalButtons()
{
  if (!m_dvdnav) return 0;
  
  pci_t* pci = m_dll.dvdnav_get_current_nav_pci(m_dvdnav);
  
  int counter = 0;
  for (int i = 0; i < 36; i++)
  {
    if (pci->hli.btnit[i].x_start ||
        pci->hli.btnit[i].x_end ||
        pci->hli.btnit[i].y_start ||
        pci->hli.btnit[i].y_end)
    {
      counter++;
    }
  }
  return counter;
}

void CDVDInputStreamNavigator::OnUp()
{
  if (m_dvdnav) m_dll.dvdnav_upper_button_select(m_dvdnav, m_dll.dvdnav_get_current_nav_pci(m_dvdnav));    
}

void CDVDInputStreamNavigator::OnDown()
{
  if (m_dvdnav) m_dll.dvdnav_lower_button_select(m_dvdnav, m_dll.dvdnav_get_current_nav_pci(m_dvdnav));
}

void CDVDInputStreamNavigator::OnLeft()
{
  if (m_dvdnav) m_dll.dvdnav_left_button_select(m_dvdnav, m_dll.dvdnav_get_current_nav_pci(m_dvdnav));
}

void CDVDInputStreamNavigator::OnRight()
{
  if (m_dvdnav) m_dll.dvdnav_right_button_select(m_dvdnav, m_dll.dvdnav_get_current_nav_pci(m_dvdnav));
}

void CDVDInputStreamNavigator::OnMenu()
{
  if (m_dvdnav) m_dll.dvdnav_menu_call(m_dvdnav, DVD_MENU_Escape);
}

void CDVDInputStreamNavigator::OnBack()
{
  if (!m_dvdnav) m_dll.dvdnav_go_up(m_dvdnav);
}

// we don't allow skipping in menu's cause it will remove menu overlays
void CDVDInputStreamNavigator::OnNext()
{
  if (m_dvdnav && !IsInMenu())
  {
    //m_bDiscardHop = true;
    m_dll.dvdnav_next_pg_search(m_dvdnav);
  }
}

// we don't allow skipping in menu's cause it will remove menu overlays
void CDVDInputStreamNavigator::OnPrevious()
{
  if (m_dvdnav && !IsInMenu())
  {
    //m_bDiscardHop = true;
    m_dll.dvdnav_prev_pg_search(m_dvdnav);
  }
}

void CDVDInputStreamNavigator::SkipStill()
{
  if (!m_dvdnav) return ;
  m_dll.dvdnav_still_skip(m_dvdnav);
}

void CDVDInputStreamNavigator::SkipWait()
{
  if (!m_dvdnav) return ;
  m_dll.dvdnav_wait_skip(m_dvdnav);
}

bool CDVDInputStreamNavigator::IsInMenu()
{
  if (!m_dvdnav) return false;

  //unsigned int iButton = (m_dll.dvdnav_get_vm(m_dvdnav)->state.HL_BTNN_REG >> 10);
  pci_t* pci = m_dll.dvdnav_get_current_nav_pci(m_dvdnav);
  if (pci->hli.hl_gi.hli_ss > 0) return true;

  int iTitle, iPart;

  //if (DVDNAV_STATUS_OK != m_dll.dvdnav_current_title_info(m_dvdnav, &iTitle, &iPart))
  //return (iTitle == 0);
  m_dll.dvdnav_current_title_info(m_dvdnav, &iTitle, &iPart);

  // if we are not in a vts domain, we are probably in a menu
  return (0 == m_dll.dvdnav_is_domain_vts(m_dvdnav) || iTitle == 0);
  return false;
}

int CDVDInputStreamNavigator::GetActiveSubtitleStream()
{
  if (!m_dvdnav) return -1;

  vm_t* vm = m_dll.dvdnav_get_vm(m_dvdnav);
  if (vm)
  {
    if((vm->state).domain != VTS_DOMAIN)
      return 0;

    int subpN = (vm->state).SPST_REG & ~0x40;

    // If stream is larger than number of streams we have, use report first one
    if (vm->vtsi && vm->vtsi->vtsi_mat &&
        subpN >= vm->vtsi->vtsi_mat->nr_of_vts_subp_streams)
      return 0;
    // If it has info, it is an ok stream.. otherwise just use first as stream
    if ((vm->state).pgc->subp_control[subpN] & (1<<31))
      return subpN;
    else //If it was invalid, just set first as active
      return 0;
  }
  return -1;

  //dvdnav_get_active_spu_stream will return the stream counted as the physical streams in the mpeg
  //however all other subtitle functions count based on subpictures in the dvd, each subpicture can have multiple
  //mpeg streams associated.

  //iStream = (m_dll.dvdnav_get_active_spu_stream(m_dvdnav) & 0xff);
  //if (iStream == 0x80) return -1;
  //return (iStream & ~0x80);
}

// This will return the mpeg subtitle stream that a certain subpicture channel refers to
// It will give the 4:3 version if material is 4:3 and the Widescreen version if material is 16:9. (Letterbox, Pan&Scan is not available this way)
//GetMpegSubtitleStream(GetActiveSubtitleStream) will return the same as dvdnav_get_active_audio_stream(m_dvdnav)
int CDVDInputStreamNavigator::GetMpegSubtitleStream(int iSubtitle)
{
  return m_dll.dvdnav_get_spu_logical_stream(m_dvdnav, iSubtitle);
}

std::string CDVDInputStreamNavigator::GetSubtitleStreamLanguage(int iId)
{
  if (!m_dvdnav) return NULL;

  CStdString strLanguage;  

  subp_attr_t subp_attributes;
  if( m_dll.dvdnav_get_stitle_info(m_dvdnav, iId, &subp_attributes) == DVDNAV_STATUS_OK )
  {
    
    if( subp_attributes.type == DVD_SUBPICTURE_TYPE_Language )
    {
      if (!g_LangCodeExpander.LookupDVDLangCode(strLanguage, subp_attributes.lang_code)) strLanguage = "Unknown";

      switch( subp_attributes.lang_extension )
      {
        case DVD_SUBPICTURE_LANG_EXT_NotSpecified:
        case DVD_SUBPICTURE_LANG_EXT_NormalCaptions:
        case DVD_SUBPICTURE_LANG_EXT_BigCaptions:
        case DVD_SUBPICTURE_LANG_EXT_ChildrensCaptions:
          break;

        case DVD_SUBPICTURE_LANG_EXT_NormalCC:
        case DVD_SUBPICTURE_LANG_EXT_BigCC:
        case DVD_SUBPICTURE_LANG_EXT_ChildrensCC:
          strLanguage+= " (CC)";
          break;
        case DVD_SUBPICTURE_LANG_EXT_Forced:
          strLanguage+= " (Forced)";
          break;
        case DVD_SUBPICTURE_LANG_EXT_NormalDirectorsComments:
        case DVD_SUBPICTURE_LANG_EXT_BigDirectorsComments:
        case DVD_SUBPICTURE_LANG_EXT_ChildrensDirectorsComments:
          strLanguage+= " (Directors Comments)";
          break;
      }
    }
    else
    {
      strLanguage = "Unknown";
    }
  }

  return strLanguage;
}

int CDVDInputStreamNavigator::GetSubTitleStreamCount()
{
  if (!m_dvdnav) return 0;
  return m_dll.dvdnav_get_nr_of_subtitle_streams(m_dvdnav);
}

int CDVDInputStreamNavigator::GetActiveAudioStream()
{
  if (!m_dvdnav) return -1;

  vm_t* vm = m_dll.dvdnav_get_vm(m_dvdnav);
  if( vm )
  {
    if((vm->state).domain != VTS_DOMAIN)
      return -1;

    int audioN = (vm->state).AST_REG;

    // If stream is larger than number of streams we have, use report first one
    if (vm->vtsi && vm->vtsi->vtsi_mat &&
        audioN >= vm->vtsi->vtsi_mat->nr_of_vts_audio_streams )
      return 0;
    // If it has info, it is an ok stream.. otherwise just use first as stream
    else if((vm->state).pgc->audio_control[audioN] & (1<<15))
      return audioN;
    else
      return 0;
  }
  
  return -1;
}

//GetMpegAudioStream(GetActiveAudioStream) will return the same as dvdnav_get_active_audio_stream(m_dvdnav)
int CDVDInputStreamNavigator::GetMpegAudioStream(int iId)
{
  return m_dll.dvdnav_get_audio_logical_stream(m_dvdnav, iId);
}


std::string CDVDInputStreamNavigator::GetAudioStreamLanguage(int iId)
{
  if (!m_dvdnav) return NULL;
  
  CStdString strLanguage;

  audio_attr_t audio_attributes;
  if( m_dll.dvdnav_get_audio_info(m_dvdnav, iId, &audio_attributes) == DVDNAV_STATUS_OK )
  {
    if (!g_LangCodeExpander.LookupDVDLangCode(strLanguage, audio_attributes.lang_code)) strLanguage = "Unknown";

    switch( audio_attributes.lang_extension )
    {
      case DVD_AUDIO_LANG_EXT_VisuallyImpaired:
        strLanguage+= " (Visually Impaired)";
        break;
      case DVD_AUDIO_LANG_EXT_DirectorsComments1:
        strLanguage+= " (Directors Comments)";
        break;
      case DVD_AUDIO_LANG_EXT_DirectorsComments2:
        strLanguage+= " (Directors Comments 2)";
        break;
      case DVD_AUDIO_LANG_EXT_NotSpecified:
      case DVD_AUDIO_LANG_EXT_NormalCaptions:
      default:
        break;
    }
  }

  return strLanguage;
}

int CDVDInputStreamNavigator::GetAudioStreamCount()
{
  if (!m_dvdnav) return 0;
  return m_dll.dvdnav_get_nr_of_audio_streams(m_dvdnav);
}

bool CDVDInputStreamNavigator::GetCurrentButtonInfo(CDVDOverlaySpu* pOverlayPicture, CDVDDemuxSPU* pSPU, int iButtonType)
{
  int alpha[2][4];
  int color[2][4];
  dvdnav_highlight_area_t hl;
  
  if (!m_dvdnav) return false;
  
  int iButton = GetCurrentButton();

  if (m_dll.dvdnav_get_button_info(m_dvdnav, alpha, color) == 0)
  {
  /*
    int* a = pOverlayPicture->alpha;
    
    // we could also check the alpha from dvdnav_get_button_info
    // by checking if one of those values are valid (!0) and set them
    if (!a[0] && !a[1] && !a[2] && !a[3])*/
    {
      pOverlayPicture->alpha[0] = alpha[iButtonType][0];
      pOverlayPicture->alpha[1] = alpha[iButtonType][1];
      pOverlayPicture->alpha[2] = alpha[iButtonType][2];
      pOverlayPicture->alpha[3] = alpha[iButtonType][3];
    }
    
    int i;
    for (i = 0; i < 3; i++) pOverlayPicture->color[0][i] = pSPU->m_clut[color[iButtonType][0]][i];
    for (i = 0; i < 3; i++) pOverlayPicture->color[1][i] = pSPU->m_clut[color[iButtonType][1]][i];
    for (i = 0; i < 3; i++) pOverlayPicture->color[2][i] = pSPU->m_clut[color[iButtonType][2]][i];
    for (i = 0; i < 3; i++) pOverlayPicture->color[3][i] = pSPU->m_clut[color[iButtonType][3]][i];
  }
  
  if (DVDNAV_STATUS_OK == m_dll.dvdnav_get_highlight_area(m_dll.dvdnav_get_current_nav_pci(m_dvdnav), iButton, iButtonType, &hl))
  {
    // button cropping information
    pOverlayPicture->crop_i_x_start = hl.sx;
    pOverlayPicture->crop_i_x_end = hl.ex;
    pOverlayPicture->crop_i_y_start = hl.sy;
    pOverlayPicture->crop_i_y_end = hl.ey;
  }

  return true;
}

int CDVDInputStreamNavigator::GetTotalTime()
{
  //We use buffers of this as they can get called from multiple threads, and could block if we are currently reading data
  return m_iTotalTime;
}

int CDVDInputStreamNavigator::GetTime()
{
  //We use buffers of this as they can get called from multiple threads, and could block if we are currently reading data
  return m_iTime;
}

bool CDVDInputStreamNavigator::Seek(int iTimeInMsec)
{
  uint32_t pos=0, len=0;
  uint64_t newpos=0;
  m_dll.dvdnav_get_position(m_dvdnav, &pos, &len);
  
  float fDesiredPrecentage = (iTimeInMsec / (float)GetTotalTime());
  
  // would be much easier if we knew how much blocks one second is
  // should actually look trough the vm tables to find the closest block the the requested time.
  // kinda think it's abit of overkill thou.

  //newpos = (uint64_t)iTimeInMsec / 1000 * 2048;
  newpos = (uint64_t)( fDesiredPrecentage * len );
  CLog::Log(LOGDEBUG, "dvdnav_sector_search pos: %d, len: %d, time %d, totaltime %d, percentage: %d",
   pos, len, iTimeInMsec, GetTotalTime(), (int)fDesiredPrecentage);
  if (m_dll.dvdnav_sector_search(m_dvdnav, newpos, SEEK_SET) == DVDNAV_STATUS_ERR)
  {
    CLog::Log(LOGDEBUG, "dvdnav: %s", m_dll.dvdnav_err_to_string(m_dvdnav));
    return false;
  }
  m_bDiscardHop = true;
  return true;
}

float CDVDInputStreamNavigator::GetVideoAspectRatio()
{
  int iAspect = m_dll.dvdnav_get_video_aspect(m_dvdnav);
  int iPerm = m_dll.dvdnav_get_video_scale_permission(m_dvdnav);

  //The video scale permissions should give if the source is letterboxed
  //and such. should be able to give us info that we can zoom in automatically
  //not sure what to do with it currently

  printf("DVD - Aspect wanted: %d, Scale permissions: %d", iAspect, iPerm);
  switch(iAspect)
  {
    case 2: //4:3
      return 4.0f / 3.0f;
    case 3: //16:9
      return 16.0f / 9.0f;
    case 4: 
      return 2.11f / 1.0f;
    default: //Unknown, use libmpeg2
      return 0.0f;
  }    
}

int CDVDInputStreamNavigator::GetNrOfTitles()
{
  int iTitles = 0;
  
  if (m_dvdnav)
  {
    m_dll.dvdnav_get_number_of_titles(m_dvdnav, &iTitles);
  }
  
  return iTitles;
}

int CDVDInputStreamNavigator::GetNrOfParts(int iTitle)
{
  int iParts = 0;
  
  if (m_dvdnav)
  {
    m_dll.dvdnav_get_number_of_parts(m_dvdnav, iTitle, &iParts);
  }
  
  return iParts;
}

bool CDVDInputStreamNavigator::PlayTitle(int iTitle)
{
  if (m_dvdnav)
  {
    return (DVDNAV_STATUS_OK == m_dll.dvdnav_title_play(m_dvdnav, iTitle));
  }
  
  return false;
}

bool CDVDInputStreamNavigator::PlayPart(int iTitle, int iPart)
{
  if (m_dvdnav)
  {
    return (DVDNAV_STATUS_OK == m_dll.dvdnav_part_play(m_dvdnav, iTitle, iPart));
  }
  
  return false;
}

void CDVDInputStreamNavigator::EnableSubtitleStream(bool bEnable)
{
  int iCurrentStream = GetActiveSubtitleStream();
  if (bEnable)
  {
    SetActiveSubtitleStream(iCurrentStream);
  }
  else
  {
    // hide subtitles
    SetActiveSubtitleStream(iCurrentStream | 0x80);
  }
}
