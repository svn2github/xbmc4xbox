#include "musicinfotagloadermp3.h"
#include "stdstring.h"
#include "sectionloader.h"
#include "Util.h"

#define BYTES2INT(b1,b2,b3,b4) (((b1 & 0xFF) << (3*8)) | \
                                ((b2 & 0xFF) << (2*8)) | \
                                ((b3 & 0xFF) << (1*8)) | \
                                ((b4 & 0xFF) << (0*8)))

#define MPEG_VERSION2_5 0
#define MPEG_VERSION1   1
#define MPEG_VERSION2   2

/* Xing header information */
#define VBR_FRAMES_FLAG 0x01
#define VBR_BYTES_FLAG  0x02
#define VBR_TOC_FLAG    0x04

// mp3 header flags
#define SYNC_MASK (0x7ff << 21)
#define VERSION_MASK (3 << 19)
#define LAYER_MASK (3 << 17)
#define PROTECTION_MASK (1 << 16)
#define BITRATE_MASK (0xf << 12)
#define SAMPLERATE_MASK (3 << 10)
#define PADDING_MASK (1 << 9)
#define PRIVATE_MASK (1 << 8)
#define CHANNELMODE_MASK (3 << 6)
#define MODE_EXT_MASK (3 << 4)
#define COPYRIGHT_MASK (1 << 3)
#define ORIGINAL_MASK (1 << 2)
#define EMPHASIS_MASK 3

using namespace MUSIC_INFO;
using namespace XFILE;

CMusicInfoTagLoaderMP3::CMusicInfoTagLoaderMP3(void)
{
}

CMusicInfoTagLoaderMP3::~CMusicInfoTagLoaderMP3()
{
}

bool CMusicInfoTagLoaderMP3::ReadTag( ID3_Tag& id3tag, CMusicInfoTag& tag )
{
	bool bResult= false;

	SYSTEMTIME dateTime;
	auto_ptr<char>pYear  (ID3_GetYear( &id3tag  ));
	auto_ptr<char>pTitle (ID3_GetTitle( &id3tag ));
	auto_ptr<char>pArtist(ID3_GetArtist( &id3tag));
	auto_ptr<char>pAlbum (ID3_GetAlbum( &id3tag ));
	auto_ptr<char>pGenre (ID3_GetGenre( &id3tag ));
	int nTrackNum=ID3_GetTrackNum( &id3tag );

	tag.SetTrackNumber(nTrackNum);

	if (NULL != pGenre.get())
	{
		tag.SetGenre(pGenre.get());
	}
	if (NULL != pTitle.get())
	{
		bResult = true;
		tag.SetTitle(pTitle.get());
	}
	if (NULL != pArtist.get())
	{
		tag.SetArtist(pArtist.get());
	}
	if (NULL != pAlbum.get())
	{
		tag.SetAlbum(pAlbum.get());
	}
	if (NULL != pYear.get())
	{
		dateTime.wYear=atoi(pYear.get());
		tag.SetReleaseDate(dateTime);
	}

	//	extract Cover Art and save as album thumb
	if (ID3_HasPicture(&id3tag))
	{
		ID3_PictureType nPicTyp=ID3PT_COVERFRONT;
		bool bFound=false;
		auto_ptr<char>pMimeTyp (ID3_GetMimeTypeOfPicType(&id3tag, nPicTyp));
		if (pMimeTyp.get() == NULL)
		{
			nPicTyp=ID3PT_OTHER;
			auto_ptr<char>pMimeTyp (ID3_GetMimeTypeOfPicType(&id3tag, nPicTyp));
			if (pMimeTyp.get() != NULL)
				bFound=true;
		}
		else
			bFound=true;

		if (bFound)
		{
			CStdString strCoverArt;
			CUtil::GetAlbumThumb(tag.GetAlbum(), strCoverArt);
			if (!CUtil::FileExists(strCoverArt))
				ID3_GetPictureDataOfPicType(&id3tag, strCoverArt, nPicTyp);
		}
	}
	const Mp3_Headerinfo* mp3info = id3tag.GetMp3HeaderInfo();
	if ( mp3info != NULL )
		tag.SetDuration( mp3info->time );
		
	return bResult;
}

bool CMusicInfoTagLoaderMP3::Load(const CStdString& strFileName, CMusicInfoTag& tag)
{
	// retrieve the ID3 Tag info from strFileName
	// and put it in tag
	bool bResult=false;
//	CSectionLoader::Load("LIBID3");
	tag.SetURL(strFileName);
	tag.SetLoaded(true);
	CFile file;
	if ( file.Open( strFileName.c_str() ) ) 
	{
		//	Do not use ID3TT_ALL, because
		//	id3lib reads the ID3V1 tag first
		//	then ID3V2 tag is blocked.
		ID3_XIStreamReader reader( file );
		ID3_Tag myTag;
		if ( myTag.Link(reader, ID3TT_ID3V2) >= 0)
		{
			if ( !(bResult = ReadTag( myTag, tag )) ) 
			{
				myTag.Clear();
				if ( myTag.Link(reader, ID3TT_ID3V1 ) >= 0 ) 
				{
					bResult = ReadTag( myTag, tag );
				}
			}
			if (tag.GetDuration()<=0)
				tag.SetDuration(ReadDuration(file, myTag));
		}
		file.Close();
	}

	//	CSectionLoader::Unload("LIBID3");
	return bResult;
}

/* check if 'head' is a valid mp3 frame header */
bool CMusicInfoTagLoaderMP3::IsMp3FrameHeader(unsigned long head)
{
    if ((head & SYNC_MASK) != (unsigned long)SYNC_MASK) /* bad sync? */
        return false;
    if ((head & VERSION_MASK) == (1 << 19)) /* bad version? */
        return false;
    if (!(head & LAYER_MASK)) /* no layer? */
        return false;
    if ((head & BITRATE_MASK) == BITRATE_MASK) /* bad bitrate? */
        return false;
    if (!(head & BITRATE_MASK)) /* no bitrate? */
        return false;
    if ((head & SAMPLERATE_MASK) == SAMPLERATE_MASK) /* bad sample rate? */
        return false;
    if (((head >> 19) & 1) == 1 &&
        ((head >> 17) & 3) == 3 &&
        ((head >> 16) & 1) == 1)
        return false;
    if ((head & 0xffff0000) == 0xfffe0000)
        return false;
    
    return true;
}

//	Inspired by http://rockbox.haxx.se/ and http://www.xs4all.nl/~rwvtveer/scilla 
int CMusicInfoTagLoaderMP3::ReadDuration(CFile& file, const ID3_Tag& id3tag)
{
	int nDuration=0;

	//raw mp3Data = FileSize - ID3v1 tag - ID3v2 tag
	int nMp3DataSize=id3tag.GetFileSize()-id3tag.GetAppendedBytes()-id3tag.GetPrependedBytes();
	int nPrependedBytes=id3tag.GetPrependedBytes();

	const int freqtab[][4] =
	{
			{11025, 12000, 8000, 0},  /* MPEG version 2.5 */
			{44100, 48000, 32000, 0}, /* MPEG Version 1 */
			{22050, 24000, 16000, 0}, /* MPEG version 2 */
	};

	unsigned char buffer[65501];
	file.Seek(0, SEEK_SET);
	file.Read(buffer, 65500);

	int frequency=0, bitrate=0, bittable=0;
	double tpf=0.0;
	for (int i=nPrependedBytes; i<65500; i++)
	{
		unsigned long mpegheader=(unsigned long)(
																	( (buffer[i] & 255) << 24) |
																	( (buffer[i+1] & 255) << 16) |
																	( (buffer[i+2] & 255) <<  8) |
																	( (buffer[i+3] & 255)      )
																); 

		if (IsMp3FrameHeader(mpegheader))
		{
			int version=0;
			/* MPEG Audio Version */
			switch(mpegheader & VERSION_MASK) {
			case 0:
					/* MPEG version 2.5 is not an official standard */
					version = MPEG_VERSION2_5;
					bittable = MPEG_VERSION2 - 1; /* use the V2 bit rate table */
					break;
		      
			case (1 << 19):
					return 0;
		      
			case (2 << 19):
					/* MPEG version 2 (ISO/IEC 13818-3) */
					version = MPEG_VERSION2;
					bittable = MPEG_VERSION2 - 1;
					break;
		      
			case (3 << 19):
					/* MPEG version 1 (ISO/IEC 11172-3) */
					version = MPEG_VERSION1;
					bittable = MPEG_VERSION1 - 1;
					break;
			}

			int layer=0;
			switch(mpegheader & LAYER_MASK)
			{
			case (3 << 17):	//	LAYER_I
				layer=1;
				break;
			case (2 << 17):	//	LAYER_II
				layer=2;
				break;
			case (1 << 17):	//	LAYER_III
				layer=3;
				break;
			}

			/* Table of bitrates for MP3 files, all values in kilo.
			* Indexed by version, layer and value of bit 15-12 in header.
			*/
			const int bitrate_table[2][4][16] =
			{
					{
							{0},
							{0,32,64,96,128,160,192,224,256,288,320,352,384,416,448,0},
							{0,32,48,56, 64,80, 96, 112,128,160,192,224,256,320,384,0},
							{0,32,40,48, 56,64, 80, 96, 112,128,160,192,224,256,320,0}
					},
					{
							{0},
							{0,32,48,56,64,80,96,112,128,144,160,176,192,224,256,0},
							{0, 8,16,24,32,40,48, 56, 64, 80, 96,112,128,144,160,0},
							{0, 8,16,24,32,40,48, 56, 64, 80, 96,112,128,144,160,0}
					}
			};

			/* Bitrate */
			int bitindex = (mpegheader & 0xf000) >> 12;
			bitrate = bitrate_table[bittable][layer][bitindex];
  
			double tpfbs[] = { 0, 384.0f, 1152.0f, 1152.0f };
			int freqindex = (mpegheader & 0x0C00) >> 10;
			frequency = freqtab[version][freqindex];
			tpf=tpfbs[layer] / (double) frequency;
			if (version==MPEG_VERSION2_5 && version==MPEG_VERSION2)
				tpf/=2;

			if(frequency == 0)
				return 0;
			break;
		}
	}

	//	Check for Xing VBR mp3 file
	int frame_count=0;
	for (i=nPrependedBytes; i<65500; i++)
	{
		if (buffer[i]=='X' &&
			buffer[i+1]=='i' &&
			buffer[i+2]=='n' &&
			buffer[i+3]=='g')
		{
			if(buffer[i+7] & VBR_FRAMES_FLAG) /* Is the frame count there? */
			{
					frame_count = BYTES2INT(buffer[i+8], buffer[i+8+1],
																	buffer[i+8+2], buffer[i+8+3]);
					break;
			}
		}

		unsigned long mpegheader=(unsigned long)(
																	( (buffer[i] & 255) << 24) |
																	( (buffer[i+1] & 255) << 16) |
																	( (buffer[i+2] & 255) <<  8) |
																	( (buffer[i+3] & 255)      )
																); 

		if (IsMp3FrameHeader(mpegheader))
			break;
	}

	if (frame_count > 0)
	{
		double d=tpf * frame_count;
		return (int)d;
	}


	//	Check for VBRI mp3 file
	//	Untested due to lack of VBRI files
	//for (i=nPrependedBytes; i<65500; i++)
	//{
	//	if (buffer[i]=='V' &&
	//		buffer[i+1]=='B' &&
	//		buffer[i+2]=='R' &&
	//		buffer[i+3]=='I')
	//	{
	//		for ( ; i<65500;i++)
	//		{
	//			unsigned long mpegheader=(unsigned long)(
	//																		( (buffer[i] & 255) << 24) |
	//																		( (buffer[i+1] & 255) << 16) |
	//																		( (buffer[i+2] & 255) <<  8) |
	//																		( (buffer[i+3] & 255)      )
	//																	); 

	//			if (is_mp3frameheader(mpegheader))
	//			{
	//				frame_count = BYTES2INT(buffer[i+14], buffer[i+14+1],
	//																buffer[i+14+2], buffer[i+14+3]);
	//			}
	//		}
	//	}
	//}

	//if (frame_count > 0)
	//{
	//	double duration=tpf * frame_count;
	//	return (int)duration;
	//}


	//	Normal mp3 with constant bitrate duration
	double d=(double)nMp3DataSize / ((double)bitrate * 125.0f);
	return (int)d;

}
