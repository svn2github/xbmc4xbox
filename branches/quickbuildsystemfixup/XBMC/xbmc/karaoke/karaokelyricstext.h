//
// C++ Interface: karaokelyricstext
//
// Description:
//
//
// Author: Team XBMC <>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef KARAOKELYRICSTEXT_H
#define KARAOKELYRICSTEXT_H


#include "karaokelyrics.h"

class CGUITextLayout;

//! This class is a base abstract class for all lyric loaders which provide text-based lyrics.
//! Once the lyrics are properly transferred to this class, it will take care of rendering.
//! Therefore the Render() function in the derived class might be empty, but must still call
//! the parent function to function properly.
class CKaraokeLyricsText : public CKaraokeLyrics
{
  public:
    CKaraokeLyricsText();
    virtual ~CKaraokeLyricsText();

    //! Parses the lyrics or song file, and loads the lyrics into memory.
    //! Done in derived classes, this class only takes care of rendering.
    virtual bool Load() = 0;

    //! Most of text lyrics do not have any backgrounds
    virtual bool HasBackground();

  protected:
    enum
    {
      LYRICS_NEW_LINE = 0x0001,
      LYRICS_NEW_PARAGRAPH = 0x0002,

      LYRICS_END = 0xFFFFFFFF
    };

    //! Render functionality from the parent class is handled here
    virtual void Render();

    //! This function is called when the karoke visualisation window created. It may
    //! be called after Start(), but is guaranteed to be called before Render()
    //! Default implementation does nothing and returns true.
    virtual bool InitGraphics();

    //! This function is called when the karoke visualisation window is destroyed.
    //! Default implementation does nothing.
    virtual void Shutdown();

    //! The loader should call this function to add each separate lyrics syllable.
    //! timing is in 1/10 seconds; if you have time in milliseconds, multiple by 100.
    //! flags could be 0 (regular), LYRICS_NEW_LINE (this syllable should start on a new line),
    //! and LYRICS_NEW_PARAGRAPH (this syllable should start on a new paragraph)
    void  addLyrics( const CStdString& text, unsigned int timing, unsigned int flags = 0 );

    //! This function clears the lyrics array and resets song information
    void  clearLyrics();

    //! This function calculares next paragraph of lyrics which will be shown. Returns true if indexes change
    void  nextParagraph();

    //! Rescan lyrics, fix typical issues
    void  rescanLyrics();

    //! Returns string width if rendered using current font
    float  getStringWidth( const CStdString& text );

    //! Saves parsed lyrics into a temporary file for debugging
    void  saveLyrics();

    //! Those variables keep the song information if available, parsed from the lyrics file.
    //! It should not be based on filename, as this case will be handled internally.
    //! Should be set to "" if no information available.
    CStdString    m_songName;
    CStdString    m_artist;

  private:

    //! Lyrics render state machine
    enum
    {
      //! the next paragraph lyrics are not shown yet. Screen is clear.
      //! All indexes are set, m_index points to the first element.
      //! m_index timing - m_delayBefore shows when the state changes to STATE_DRAW_SYLLABLE
      STATE_WAITING,

      //! the next paragraph lyrics are shown, but the paragraph hasn't start yet.
      //! Using m_preambleTime, we redraw the marker each second.
      STATE_PREAMBLE,

      //! the lyrics are played, the end of paragraph is not reached.
      STATE_PLAYING_PARAGRAPH,

      //! the whole paragraph is colored, but still shown, waiting until it's time to clear the lyrics.
      //! m_index still points to the last entry, and m_indexNextPara points to the first entry of next
      //! paragraph, or to LYRICS_END. When the next paragraph is about to start (which is
      //! m_indexNextPara timing - m_delayBefore), the state switches to STATE_START_PARAGRAPH. When time
      //! goes after m_index timing + m_delayAfter, the state switches to STATE_WAITING,
      STATE_END_PARAGRAPH,

      //!< the song is completed, there are no more lyrics to show. This state is finita la comedia.
      STATE_END_SONG,
    };

    typedef struct
    {
      CStdString    text;
      unsigned int   timing;
      unsigned int   flags;

    } Lyric;

    std::vector<Lyric>  m_lyrics;

    //! Text layout for lyrics
    CGUITextLayout  *  m_karaokeLayout;

    //! Text layout for preamble
    CGUITextLayout  *  m_preambleLayout;

    //! Fond for lyrics
    CGUIFont     *  m_karaokeFont;

    //! Lyrics colors
    unsigned int    m_colorLyrics;
    unsigned int    m_colorLyricsOutline;
    CStdString       m_colorSinging;

    //! This is index in m_lyrics pointing to current paragraph first, last and current elements
    unsigned int    m_indexEndPara;
    unsigned int    m_indexStartPara;
    unsigned int    m_index;

    //! This is preamble timing, used to update preamble each second
    unsigned int    m_lastPreambleUpdate;

    //! This is index in m_lyrics pointing to next paragraph.
    //! If LYRICS_END - there is no next paragraph
    unsigned int    m_indexNextPara;

    //! Current lyrics rendering state
    unsigned int    m_lyricsState;

    //! Lyrics text on screen
    CStdString      m_currentLyrics;

    //! Preamble text on screen
    CStdString      m_currentPreamble;

    //
    // Configuration settings
    //
    //! Number of 1/10 seconds between the lyrics are shown and start singing. 50 means 5 seconds
    unsigned int    m_showLyricsBeforeStart;
    unsigned int    m_showPreambleBeforeStart;
    bool            m_mergeLines;

    //! Autosplitter uses this value to split paragraphs. If a new line starts in more than
    //! m_paragraphBreakTime after current line ends, it's a new paragraph.
    unsigned int    m_paragraphBreakTime;

    //! Number of 1/10 seconds after the lyrics are sung. 50 means 5 seconds
    unsigned int    m_delayAfter;
};

#endif
