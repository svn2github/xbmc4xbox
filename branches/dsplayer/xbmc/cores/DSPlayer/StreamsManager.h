#pragma once

/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "dshowutil/dshowutil.h"
#include "DShowUtil/DshowCommon.h"
#include <initguid.h>
#include "moreuuids.h"
#include <dmodshow.h>
#include <D3d9.h>
#include "DShowUtil/MediaTypeEx.h"
#include "Filters/ffdshow_constants.h"

#include "DSGraph.h"
#include "log.h"
#include "CharsetConverter.h"
#include "RegExp.h"

enum SStreamType
{
  AUDIO, VIDEO, SUBTITLE, EXTERNAL_SUBTITLE
};

struct SStreamInfos
{
  unsigned int IAMStreamSelect_Index; ///< IAMStreamSelect index of the stream
  CStdString name; ///< Stream name
  CStdString codecname; ///< Stream codec name
  DWORD flags; ///< Stream flags. Set to AMSTREAMSELECTINFO_ENABLED if the stream if selected in the GUI, 0 otherwise
  Com::SmartPtr<IPin> pObj; ///< Output pin of the splitter
  Com::SmartPtr<IPin> pUnk; ///< Not used
  LCID  lcid; ///< Currently not used
  DWORD group; ///< Currently not used
  SStreamType type; ///< Stream type
  bool connected; ///< Is the stream connected

  /// Set all structure variable to their default values
  virtual void Clear()
  {
    IAMStreamSelect_Index = 0;
    flags = 0;
    pObj = 0;
    pUnk = 0;
    lcid = 0;
    group = 0;
    name = "";
    codecname = "";
    connected = false;
  }
};

/// Informations about an audio stream
struct SAudioStreamInfos: SStreamInfos
{
  unsigned int channels; ///< Number of audio channels
  unsigned int bitrate; ///< Audio bitrate
  unsigned int samplerate; ///< Audio samplerate

  virtual void Clear()
  {
    SStreamInfos::Clear();

    channels = 0;
    bitrate = 0;
    samplerate = 0;
  }

  SAudioStreamInfos()
  {
    Clear();
    type = AUDIO;
  }
};

/// Informations about a video stream
struct SVideoStreamInfos: SStreamInfos
{
  unsigned int width; ///< Current video width
  unsigned int height; ///< Current video height
  DWORD fourcc; ///< Current video fourcc

  virtual void Clear()
  {
    SStreamInfos::Clear();

    width = 0;
    height = 0;
    fourcc = 0;
  }

  SVideoStreamInfos()
  {
    Clear();
    type = VIDEO;
  }
};

/// Informations about a internal subtitle
struct SSubtitleStreamInfos: SStreamInfos
{
  CStdString encoding; ///< Subtitle encoding
  bool external; ///< If True, you can safely cast the structure to a SExternalSubtitleInfos

  unsigned long offset; ///< Not used
  CStdString isolang; ///< ISO Code of the subtitle language. Use CLang to get full language name

  GUID subtype; ///< Subtype GUID of the subtitle

  virtual void Clear()
  {
    SStreamInfos::Clear();

    encoding = "";
    external = false;
    offset = 0;
    isolang = "";
    subtype = GUID_NULL;
  }

  SSubtitleStreamInfos()
  {
    Clear();
    type = SUBTITLE;
  }
};

/// Informations about an external subtitle
struct SExternalSubtitleInfos: SSubtitleStreamInfos
{
  CStdString path; ///< Subtitle file path

  virtual void Clear()
  {
    SSubtitleStreamInfos::Clear();

    path = "";
  }

  SExternalSubtitleInfos()
  {
    Clear();
    type = EXTERNAL_SUBTITLE;
  }
};

/** @brief DSPlayer Streams Manager.
 *
 * Singleton class handling audio and subtitle streams.
 */
class CStreamsManager
{
public:
  /// Retrieve singleton instance
  static CStreamsManager *getSingleton();
  /// Destroy singleton instance
  static void Destroy();

  /// @return A std::vector of all audio streams found in the media file
  std::vector<SAudioStreamInfos *> GetAudios();

  /// @return Audio streams count
  int  GetAudioStreamCount();
  /// @return The index to the current audio stream
  int  GetAudioStream();
  /** Get the name of an audio stream
   * @param[in] iStream Index of the audio stream to get name of
   * @param[out] strStreamName Name of the iStream audio stream
   */
  void GetAudioStreamName(int iStream, CStdString &strStreamName);
  /** Change the current audio stream
   * @param[in] iStream Index of the audio stream
   * @remarks If the IAMStreamSelect interface wasn't found, the graph must be stopped and restarted in order to change the audio stream
   */
  void SetAudioStream(int iStream);
  /** Is the manager changing stream?
   * @return True if the manager is changing audio stream, false otherwise
   * @remarks The method is mainly used by CDSGraph when unloading the graph, to prevent object to be released when they're still used.
   */
  bool IsChangingStream();
  /// @return The number of audio channels of the current audio stream
  int GetChannels();
  /// @return The number of bits per sample of the current audio stream
  int GetBitsPerSample();
  /// @return The sample rate of the current audio stream
  int GetSampleRate();
  /// @return The name of the audio codec used in the media file (ie FLAC, MP3, DTS ...)
  CStdString GetAudioCodecName();
  /// @return An instance to the IAMStreamSelect interface if the splitter expose it, NULL otherwise
  IAMStreamSelect *GetStreamSelector() { return m_pIAMStreamSelect; }

  /// @return A std::vector of all subtitle streams (internal or external) found in the media file
  std::vector<SSubtitleStreamInfos *> GetSubtitles();
  /// @return Subtitles count
  int  GetSubtitleCount();
  /// @return The index of the current subtitle
  int  GetSubtitle();
  /** Get the name of a subtitle
   * @param[in] iStream Index of the subtitle to get name of
   * @param[out] strStreamName Name of the iStream subtitle
   */
  void GetSubtitleName(int iStream, CStdString &strStreamName);
  /** Change the current subtitle
   * @param[in] iStream Index of the subtitle
   * @remarks If the IAMStreamSelect interface wasn't found, the graph must be stopped and restarted in order to change the subtitle
   */
  void SetSubtitle(int iStream);
  /// @return True if subtitles are visible, false otherwise
  bool GetSubtitleVisible();
  /** Set subtitle visibility
   * @param[in] bVisible Subtitle visibility
   */
  void SetSubtitleVisible( bool bVisible );
  /** Set subtitle delay
   * @param in fValue Subtitle delay (in ms)
   */
  void SetSubtitleDelay(float fValue = 0.0f);
  /// @return Current subtitle delay
  float GetSubtitleDelay(void);
  /** Add a subtitle to the manager
   * @param[in] subFilePath Path of the subtitle
   * @return True if the subtitle has been added, false otherwise
   * @remarks The subtitle will be automatically flagged as external
  */
  bool AddSubtitle(const CStdString& subFilePath);
  
  /// Load streams from the current media file
  void LoadStreams();

  /// @return Current video width
  int GetPictureWidth();
  /// @return Current video height
  int GetPictureHeight();
  /// @return The name of the video codec used in the media file (XviD, DivX, h264, ...)
  CStdString GetVideoCodecName();
  
  /** Initialize the manager
   * @param[in] DSGraph Pointer to CDSGraph
   * @return True if the manager is initialized, false otherwise
   */
  bool InitManager(CDSGraph *DSGraph);
  /** Extract stream information from AM_MEDIA_TYPE
   * @param[in] mt Media type informations
   * @param[out] s A filled SStreamInfos structure
   */
  void GetStreamInfos(AM_MEDIA_TYPE *mt, SStreamInfos *s);

private:
  CStreamsManager(void);
  ~CStreamsManager(void);
  static CStreamsManager *m_pSingleton;

  CStdString ISOToLanguage(CStdString code);

  void DisconnectCurrentSubtitlePins(void);
  IPin *GetFirstSubtitlePin(void);

  std::vector<SAudioStreamInfos *> m_audioStreams;
  std::vector<SSubtitleStreamInfos *> m_subtitleStreams;

  Com::SmartPtr<IAMStreamSelect> m_pIAMStreamSelect;
  Com::SmartPtr<IFilterGraph2> m_pGraphBuilder;
  Com::SmartPtr<IBaseFilter> m_pSplitter;

  CDSGraph* m_pGraph;

  bool m_init;
  bool m_bChangingStream;
  bool m_bSubtitlesVisible;

  SVideoStreamInfos m_videoStream;
  bool m_bSubtitlesUnconnected;

  Com::SmartPtr<IPin> m_SubtitleInputPin;

  AM_MEDIA_TYPE m_subtitleMediaType;
};
