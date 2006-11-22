#pragma once
#include "DirectoryNode.h"

namespace DIRECTORY
{
  namespace MUSICDATABASEDIRECTORY
  {
    class CQueryParams
    {
    public:
      CQueryParams();
      long GetArtistId() { return m_idArtist; }
      long GetAlbumId() { return m_idAlbum; }
      long GetGenreId() { return m_idGenre; }
      long GetSongId() { return m_idSong; }

    protected:
      void SetQueryParam(NODE_TYPE NodeType, const CStdString& strNodeName);

      friend CDirectoryNode;
    private:
      long m_idArtist;
      long m_idAlbum;
      long m_idGenre;
      long m_idSong;
    };
  };
};

