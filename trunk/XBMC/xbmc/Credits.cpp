#define _WCTYPE_INLINE_DEFINED
#include <list>
#include <map>
#include <process.h>
#include "GraphicContext.h"
#include "GUIFontManager.h"
#include "credits.h"
#include "Application.h"
#include "lib/mikxbox/mikmod.h"
#include "lib/mikxbox/mikxbox.h"
#include "SectionLoader.h"
#include "credits_res.h"
#include "utils/Log.h"

// Transition effects for text, must specific exactly one in and one out effect
enum CRED_EFFECTS
{
	EFF_IN_APPEAR  = 0x0,     // appear on the screen instantly
	EFF_IN_FADE    = 0x1,     // fade in over time
	EFF_IN_FLASH   = 0x2,     // flash the screen white over time and appear (short dur recommended)
	EFF_IN_ASCEND  = 0x3,     // ascend from the bottom of the screen
	EFF_IN_DESCEND = 0x4,     // descend from the top of the screen
	EFF_IN_LEFT    = 0x5,     // slide in from the left
	EFF_IN_RIGHT   = 0x6,     // slide in from the right

	EFF_OUT_APPEAR  = 0x00,     // disappear from the screen instantly
	EFF_OUT_FADE    = 0x10,     // fade out over time
	EFF_OUT_FLASH   = 0x20,     // flash the screen white over time and disappear (short dur recommended)
	EFF_OUT_ASCEND  = 0x30,     // ascend to the top of the screen
	EFF_OUT_DESCEND = 0x40,     // descend to the bottom of the screen
	EFF_OUT_LEFT    = 0x50,     // slide out to the left
	EFF_OUT_RIGHT   = 0x60,     // slide out to the right
};
#define EFF_IN_MASK (0xf)
#define EFF_OUT_MASK (0xf0)

// One line of the credits
struct CreditLine_t
{
	short x, y;           // Position
	DWORD Time;           // Time to start transition in (in ms)
	DWORD Duration;       // Duration of display (excluding transitions) (in ms)
	WORD InDuration;      // In transition duration (in ms)
	WORD OutDuration;     // Out transition duration (in ms)
	BYTE Effects;         // Effects flags
	BYTE Font;            // Font size
	const wchar_t* Text;  // The text to display
};

// Module sync notes
// one row is (125*sngspd)/(bpm*50) seconds
// the module used is 125 bpm, spd 6 (unless noted) so 6/50 = 0.12 seconds per row
// speed 7: 0.14s/row
// speed 8: 0.16s/row
// song starts at pattern 18!

// The credits - these should be sorted by Time
// x, y are percentage distance across the screen of the center point
// Time is delay since last credit
//    x,   y,   Time,    Dur,  InD, OutD,                        Effects, Font, Text
CreditLine_t Credits[] = 
{
  // Intro  fadein 32 rows, on 46 rows, crossfade 4 rows
  {  50,  25,      0,   5400, 3840,  480, EFF_IN_FADE   |EFF_OUT_FADE   ,   78, L"XBOX" },
  {  50,  45,      0,   5400, 3840,  480, EFF_IN_FADE   |EFF_OUT_FADE   ,   78, L"MEDIA" },
  {  50,  65,      0,   5400, 3840,  480, EFF_IN_FADE   |EFF_OUT_FADE   ,   78, L"CENTER" },

  // Lead dev  crossfade 4 rows, on 75 rows, fadeout 4 rows
  {  50,  22,   9240,   9000,  480,  480, EFF_IN_FADE   |EFF_OUT_FADE   ,   36, L"Project Founders" },
  {  50,  29,      0,   9000,  480,  480, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"and" },
  {  50,  34,      0,   9000,  480,  480, EFF_IN_FADE   |EFF_OUT_FADE   ,   36, L"Lead Developers" },
  {  50,  47,    240,   8760,  480,  480, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"Frodo" },
  {  50,  57,      0,   8760,  480,  480, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"RUNTiME" },
  {  50,  67,      0,   8760,  480,  480, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"d7o3g4q" },

  // Devs  flash 0.5 rows, on 62 rows, crossfade 3.5 rows
  {  50,  22,   9720,  15420,   60,  600, EFF_IN_FLASH  |EFF_OUT_FADE   ,   36, L"Developers" },
  {  50,  35,      0,   7440,   60,  420, EFF_IN_FLASH  |EFF_OUT_FADE   ,   20, L"Butcher" },
  {  50,  45,      0,   7440,   60,  420, EFF_IN_FLASH  |EFF_OUT_FADE   ,   20, L"Forza" },
  {  50,  55,      0,   7440,   60,  420, EFF_IN_FLASH  |EFF_OUT_FADE   ,   20, L"Bobbin007" },
  {  50,  65,      0,   7440,   60,  420, EFF_IN_FLASH  |EFF_OUT_FADE   ,   20, L"JCMarshall" },
  //  crossfade 3.5, on 62, crossfade 3
  {  50,  35,   7500,   7440,  420,  360, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"jwnmulder" },
  {  50,  45,      0,   7440,  420,  360, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"monkeyhappy" },
  {  50,  55,      0,   7440,  420,  360, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"Tslayer" },

  // Project management crossfade 3, on 61, crossfade 3
  {  50,  18,   7860,   7320,  360,  360, EFF_IN_FADE   |EFF_OUT_FADE   ,   36, L"Project Management" },
  {  50,  28,      0,   7320,  360,  360, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"Gamester17" },

  {  50,  40,      0,   7320,  360,  360, EFF_IN_FADE   |EFF_OUT_FADE   ,   36, L"Tech Support Mods" },
  {  50,  50,      0,   7320,  360,  360, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"Hullebulle" },
  {  50,  60,      0,   7320,  360,  360, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"Pike" },
  {  50,  70,      0,   7320,  360,  360, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"DDay" },
  {  50,  80,      0,   7320,  360,  360, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"Poing" },

  // Testers crossfade 3, on 30(6),16(7), fadeout 8(8), gap 8(8)
  {  50,  22,   7680,   5840,  360, 1280, EFF_IN_FADE   |EFF_OUT_FADE   ,   36, L"Testers" },
  {  50,  35,      0,   5840,  360, 1280, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"MrMario64" },
  {  50,  45,      0,   5840,  360, 1280, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"Obstler" },
  {  50,  55,      0,   5840,  360, 1280, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"Shadow_Mx" },
  {  50,  65,      0,   5840,  360, 1280, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"xAD/nIGHTFALL" },
  {  50,  75,      0,   5840,  360, 1280, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"[XC]D-Ice" },

  // Support  flash 0.5, on 46.5, crossfade 3.5
  {  50,  22,   8760,   5580,   60,  420, EFF_IN_FLASH  |EFF_OUT_FADE   ,   36, L"Exceptional Patches" },
  {  50,  35,      0,   5580,   60,  420, EFF_IN_FLASH  |EFF_OUT_FADE   ,   20, L"Q-Silver" },
  {  50,  45,      0,   5580,   60,  420, EFF_IN_FLASH  |EFF_OUT_FADE   ,   20, L"WiSo" },
  {  50,  55,      0,   5580,   60,  420, EFF_IN_FLASH  |EFF_OUT_FADE   ,   20, L"Tilmann" },

  // Stream server  crossfade 3.5, on 12.5+36(1)+22 fadeout 4
  {  50,  22,   5640,   4860,  420,  480, EFF_IN_FADE   |EFF_OUT_FADE   ,   36, L"Stream Server" },
  {  50,  35,      0,   4860,  420,  480, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"[XC]D-Ice" },
  {  50,  45,      0,   4860,  420,  480, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"PuhPuh" },
  {  50,  55,      0,   4860,  420,  480, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"Pope-X" },

  // Website Hosts  flash 0.5, on 62.5, crossfade 3.5
  {  50,  22,   5760,   7500,   60,  420, EFF_IN_FLASH  |EFF_OUT_FADE   ,   36, L"Website Hosting" },
  {  50,  35,      0,   7500,   60,  420, EFF_IN_FLASH  |EFF_OUT_FADE   ,   20, L"HulleBulle" },
  {  50,  45,      0,   7500,   60,  420, EFF_IN_FLASH  |EFF_OUT_FADE   ,   20, L"MrX" },
  {  50,  55,      0,   7500,   60,  420, EFF_IN_FLASH  |EFF_OUT_FADE   ,   20, L"Team-XBMC" },
  {  50,  65,      0,   7500,   60,  420, EFF_IN_FLASH  |EFF_OUT_FADE   ,   20, L"xAD/nIGHTFALL (ASP Interface Site Upload)" },
  {  50,  75,      0,   7500,   60,  420, EFF_IN_FLASH  |EFF_OUT_FADE   ,   20, L"Sourceforge.net" },

  // Sponsors crossfade 3.5, on 60, crossfade 4
  {  50,  22,   7560,   7200,  420,  480, EFF_IN_FADE   |EFF_OUT_FADE   ,   36, L"Sponsors" },
  {  50,  35,      0,   7200,  420,  480, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"Team-Xecuter" },
  {  50,  45,      0,   7200,  420,  480, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"Team-SmartXX" },
  {  50,  55,      0,   7200,  420,  480, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"Team-OzXodus" },

  // Current Skin crossfade 3.5, on 60, crossfade 4
  {  50,  22,   7620,   7200,  480,  480, EFF_IN_FADE   |EFF_OUT_FADE   ,   36, L"$SKINTITLE" },
  {  50,  35,      0,   7200,  480,  480, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, NULL }, // skin names go in these 5
  {  50,  45,      0,   7200,  480,  480, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, NULL },
  {  50,  55,      0,   7200,  480,  480, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, NULL },
  {  50,  65,      0,   7200,  480,  480, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, NULL },
  {  50,  75,      0,   7200,  480,  480, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, NULL },

  // Special Thanks crossfade 34, on 60, fadeout 4
  {  50,  22,   7680,   7200,  480,  480, EFF_IN_FADE   |EFF_OUT_FADE   ,   36, L"Special Thanks to" },
  {  50,  35,      0,   7200,  480,  480, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"The Joker of Team Avalaunch" },
  {  50,  45,      0,   7200,  480,  480, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"Team Complex" },
  {  50,  55,      0,   7200,  480,  480, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"Team EvoX" },
  {  50,  65,      0,   7200,  480,  480, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"Xbox-scene.com" },
  {  50,  75,      0,   7200,  480,  480, EFF_IN_FADE   |EFF_OUT_FADE   ,   20, L"bestplayer.de" },

  // All stuff after this just scrolls

  // Code credits
  {  50,  50,   8500,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   36, L"Code Credits" },
  {  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"MPlayer" },
  {  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"FFmpeg" },
  {  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"XVID" },
	{  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"libmpeg2" },
	{  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"libvorbis" },
	{  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"libmad" },
	{  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"xiph" },
	{  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"MikMod" },
	{  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"Matroska" },
	{  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"CxImage" },
	{  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"libCDIO" },
	{  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"LZO" },
	{  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"Sidplay2" },
	{  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"Goom" },
	{  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"MXM" },
	{  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"XBFileZilla" },
	{  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"GoAhead Webserver" },

  // JOKE section;-)

  // section Frodo
	{  50,  50,   4000,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   36, L"Frodo" },
	{  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"Special thanks to my dear friends" },
	{  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"Gandalf, Legolas, Gimli , Aragorn, Boromir" },
	{  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"Merry, Pippin and offcourse Samwise" },

  // section mario64
  {  50,  50,   4000,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   36, L"Mario64" },
  {  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"Special thanks to:" },
  {  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"The coyote for never catching the road runner" },
  {  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"The pizza delivery guy" },
  {  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"Amstel & Heineken" },
  {  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"Rob Hubbard for his great sid tunes" },

  // section dday
  {  50,  50,   4000,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   36, L"DDay" },
  {  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"Special thanks to:" },
  {  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"Frodo for not kicking in my skull" },
  {  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"God for pizza, beer and women" },
  {  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"Douglas Adams for making me smile each summer" },
  {  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"when reading Hitchhikers Guide To The Galaxy" },

  // section [XC]D-Ice
  {  50,  50,   4000,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   36, L"[XC]D-Ice" },
  {  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"Tnx to my friends:" },
  {  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"mira, galaxy, mumrik, Frodo," },
  {  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"my girlfriend, heineken, pepsi" },
  {  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"my family, niece and nephew" },
  {  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"M$ for Blue-screen error :)" },

  // section xAD
  {  50,  50,   4000,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   36, L"xAD/nIGHTFALL" },
  {  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"Special thanks to:" },
  {  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"Butcher for making me happy (Sid player)" },
  {  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"My wife for moral support" },
  {  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"My friend 'Fox/nIGHTFALL' for the ASP help" },
  {  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"My little computer room" },

  // section Obstler
  {  50,  50,   4000,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   36, L"Obstler" },
  {  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"Special thanks to runtime and duo for" },
  {  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"starting with xbmp -- which made me buy and" },
  {  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"mod the xbox the very day it was first announced" },

  // section gamestr17
  {  50,  50,   4000,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   36, L"Gamestr17" },
  {  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"Special thanks to everyone on the XBMC/XBMP-Team," },
  {  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"and users who don't complain" },
  {  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"Real men don't make backups, but they cry often" },

  // section pike
  {  50,  50,   4000,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   36, L"Pike" },
  {  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"Microsoft for the XBOX," },
	{  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"which we have turned into the best Mediaplayer!" },
  {  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"Trance & other uplifting music" },
	{  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"for making my days more enjoyable!" },
  {  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"And remember - best things in life are free" },
	{  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"(Sex) and ofcourse XBMC!" },

	// can duplicate the lines below as many times as required for more credits
//	{  50,  50,   4000,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   36, L"" },
//	{  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"" },

	// Leave this as is - it tells the music to fade
  {   0,   0,  10000,   5000,    0,    0, EFF_IN_APPEAR |EFF_OUT_APPEAR ,   20, NULL },
};

#define NUM_CREDITS (sizeof(Credits) / sizeof(Credits[0]))

unsigned __stdcall CreditsMusicThread(void* pParam);
static BOOL s_bStopPlaying;
static BOOL s_bFadeMusic;
static HANDLE s_hMusicStarted;

// Logo rendering stuff
struct CUSTOMVERT
{
	D3DXVECTOR3 pos;
	D3DXVECTOR3 normal;
	float s, t;
	D3DXVECTOR3 tangent;
	D3DXVECTOR3 binormal;
};

static DWORD s_dwVertexDecl[] = 
{
	D3DVSD_STREAM( 0 ),
	D3DVSD_REG( 0, D3DVSDT_FLOAT3 ),    // position
	D3DVSD_REG( 1, D3DVSDT_FLOAT3 ),    // normal
	D3DVSD_REG( 2, D3DVSDT_FLOAT2 ),    // tex co-ords
	D3DVSD_REG( 3, D3DVSDT_FLOAT3 ),    // tangent
	D3DVSD_REG( 4, D3DVSDT_FLOAT3 ),    // binormal
	D3DVSD_END()
};

static IDirect3DVertexBuffer8*	pVBuffer;									// vertex and index buffers
static IDirect3DIndexBuffer8*		pIBuffer;
static DWORD										s_dwVShader, s_dwPShader;	// shaders
static IDirect3DCubeTexture8*		pSpecEnvMap;							// texture for specular environment map
static IDirect3DTexture8*				pNormalMap;								// the bump map
static DWORD										NumFaces, NumVerts;
static D3DXMATRIX								matWorld, matVP, matWVP;	// world, view-proj, world-view-proj matrices
static D3DXVECTOR3							vEye;											// camera
static D3DXVECTOR4							ambient(0.15f, 0.15f, 0.15f, 0);
static D3DXVECTOR4							colour(1.0f, 1.0f, 1.0f, 0);
static char*										ResourceHeader;
static void*										ResourceData;
static int											SkinOffset;

static HRESULT InitLogo(LPDIRECT3DDEVICE8 pD3DDevice)
{
	DWORD n;

	// Open XPR
	HANDLE hFile = CreateFile("q:\\credits\\credits.xpr", GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (hFile == INVALID_HANDLE_VALUE)
		return GetLastError();

	// Get header
	XPR_HEADER XPRHeader;
	if (!ReadFile(hFile, &XPRHeader, sizeof(XPR_HEADER), &n, 0) || n < sizeof(XPR_HEADER))
	{
		CloseHandle(hFile);
		return GetLastError() ? GetLastError() : E_FAIL;
	}

	if (XPRHeader.dwMagic != XPR_MAGIC_VALUE)
	{
		CloseHandle(hFile);
		return E_FAIL;
	}

	// Load header data (includes shaders)
	DWORD Size = XPRHeader.dwHeaderSize - sizeof(XPR_HEADER);
	ResourceHeader = (char*)malloc(Size);
	ZeroMemory(ResourceHeader, Size);
	if (!ReadFile(hFile, ResourceHeader, Size, &n, 0) || n < Size)
	{
		CloseHandle(hFile);
		return GetLastError() ? GetLastError() : E_FAIL;
	}

	// create shaders (8 bytes of header)
	pD3DDevice->CreateVertexShader(s_dwVertexDecl, (const DWORD*)(ResourceHeader + credits_VShader_OFFSET + 8), &s_dwVShader, 0);
	pD3DDevice->CreatePixelShader((const D3DPIXELSHADERDEF*)(ResourceHeader + credits_PShader_OFFSET + 12), &s_dwPShader);

	// Get info
	DWORD* MeshInfo = (DWORD*)(ResourceHeader + credits_MeshInfo_OFFSET + 8);
	NumFaces = MeshInfo[0];
	NumVerts = MeshInfo[1];

	// load resource data
	Size = XPRHeader.dwTotalSize - XPRHeader.dwHeaderSize;
	ResourceData = XPhysicalAlloc(Size, MAXULONG_PTR, 128, PAGE_READWRITE | PAGE_WRITECOMBINE);
	if (!ReadFile(hFile, ResourceData, Size, &n, 0) || n < Size)
	{
		CloseHandle(hFile);
		return GetLastError() ? GetLastError() : E_FAIL;
	}
	CloseHandle(hFile);

	// Register resources
	pVBuffer = (LPDIRECT3DVERTEXBUFFER8)(ResourceHeader + credits_XBMCVBuffer_OFFSET);
	pVBuffer->Register(ResourceData);

	pIBuffer = (LPDIRECT3DINDEXBUFFER8)(ResourceHeader + credits_XBMCIBuffer_OFFSET);
	WORD* IdxBuf = new WORD[NumFaces * 3]; // needs to be in cached memory
	memcpy(IdxBuf, (char*)ResourceData + pIBuffer->Data, NumFaces * 3 * sizeof(WORD));
	pIBuffer->Data = (DWORD)IdxBuf;

	pNormalMap = (LPDIRECT3DTEXTURE8)(ResourceHeader + credits_NormMap_OFFSET);
	pNormalMap->Register(ResourceData);

	pSpecEnvMap = (LPDIRECT3DCUBETEXTURE8)(ResourceHeader + credits_SpecEnvMap_OFFSET);
	pSpecEnvMap->Register(ResourceData);

	D3DXMATRIX matView, matProj;
	// Set view matrix
	vEye = D3DXVECTOR3(0.0f, 1.0f, -200.0f);
	D3DXVECTOR3 vAt(0.0f, 0.0f, 0.f);
	D3DXVECTOR3 vRight(1, 0, 0);
	D3DXVECTOR3 vUp(0.0f, 1.0f, 0.0f);

	D3DXVECTOR3 vLook = vAt - vEye;
	D3DXVec3Normalize(&vLook, &vLook);
	D3DXVec3Cross(&vUp, &vLook, &vRight);

	D3DXMatrixLookAtLH(&matView, &vEye, &vAt, &vUp);

	// setup projection matrix
	D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI/4, float(g_graphicsContext.GetWidth()) / g_graphicsContext.GetHeight(), 0.1f, 1000.f);

	D3DXMatrixMultiply(&matVP, &matView, &matProj);

	// setup world matrix
	D3DXMatrixIdentity(&matWorld);

	D3DXMATRIX mat;
	D3DXMatrixTranspose(&matWVP, D3DXMatrixMultiply(&mat, &matWorld, &matVP));

	// set render states
	pD3DDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
	pD3DDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	pD3DDevice->SetRenderState(D3DRS_SPECULARENABLE, FALSE);
	pD3DDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
	pD3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

	// normal map (no filter!)
	pD3DDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_POINT);
	pD3DDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_POINT);
	pD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
	pD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);

	// env map
	pD3DDevice->SetTextureStageState(3, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
	pD3DDevice->SetTextureStageState(3, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
	pD3DDevice->SetTextureStageState(3, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
	pD3DDevice->SetTextureStageState(3, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
	pD3DDevice->SetTextureStageState(3, D3DTSS_ADDRESSW, D3DTADDRESS_CLAMP);

	return S_OK;
}

static void RenderLogo(LPDIRECT3DDEVICE8 pD3DDevice, float fElapsedTime)
{
	D3DXMATRIX mat;

	// Rotate the mesh
	if (fElapsedTime > 0.0f)
	{
		if (fElapsedTime > 0.1f)
			fElapsedTime = 0.1f;
		D3DXMATRIX matRotate;
		FLOAT fXRotate = -fElapsedTime*D3DX_PI*0.08f;
		D3DXMatrixRotationX(&matRotate, fXRotate);
		D3DXMatrixMultiply(&matWorld, &matWorld, &matRotate);

		D3DXMatrixTranspose(&matWVP, D3DXMatrixMultiply(&mat, &matWorld, &matVP));
	}
	// Set vertex shader
	pD3DDevice->SetVertexShader(s_dwVShader);

	// Set WVP matrix as constants 0-3
	pD3DDevice->SetVertexShaderConstantFast(0, (float*)matWVP, 4);
	// Set world-view matrix as constants 4-7
	D3DXMatrixTranspose(&mat, &matWorld);
	pD3DDevice->SetVertexShaderConstantFast(4, (float*)mat, 4);
	// Set camera position as c8
	pD3DDevice->SetVertexShaderConstantFast(8, (float*)vEye, 1);

	// Set pixel shader
	pD3DDevice->SetPixelShader(s_dwPShader);

	// ambient in c0
	pD3DDevice->SetPixelShaderConstant(0, &ambient, 1);
	// colour in c1
	pD3DDevice->SetPixelShaderConstant(1, &colour, 1);

	pD3DDevice->SetStreamSource(0, pVBuffer, sizeof(CUSTOMVERT));
	pD3DDevice->SetIndices(pIBuffer, 0);

	pD3DDevice->SetTexture(0, pNormalMap);
	pD3DDevice->SetTexture(3, pSpecEnvMap);

	// Render geometry
	pD3DDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, NumVerts, 0, NumFaces);
}

static void CleanupLogo(LPDIRECT3DDEVICE8 pD3DDevice)
{
	pD3DDevice->SetTexture(0, NULL);
	pD3DDevice->SetTexture(3, NULL);
	pD3DDevice->SetPixelShader(0);
	pD3DDevice->SetVertexShader(0);
	pD3DDevice->SetStreamSource(0, NULL, 0);
	pD3DDevice->SetIndices(NULL, 0);

	if (s_dwVShader)
		pD3DDevice->DeleteVertexShader(s_dwVShader);
	s_dwVShader = 0;

	if (s_dwPShader)
		pD3DDevice->DeletePixelShader(s_dwPShader);
	s_dwPShader = 0;

	if (pIBuffer && pIBuffer->Data)
	{
		delete [] (WORD*)pIBuffer->Data;
		pIBuffer->Data = 0;
		pIBuffer = 0;
	}
	pVBuffer = 0;
	pNormalMap = 0;
	pSpecEnvMap = 0;

	if (ResourceHeader)
		free(ResourceHeader);
	ResourceHeader = 0;

	if (ResourceData)
		XPhysicalFree(ResourceData);
	ResourceData = 0;
}

void RunCredits()
{
	using std::map;
	using std::list;

	// Pause any media
	bool NeedUnpause = false;
	if (g_application.IsPlaying() && !g_application.m_pPlayer->IsPaused())
	{
		g_application.m_pPlayer->Pause();
		NeedUnpause = true;
	}

	g_graphicsContext.Lock(); // exclusive access
	LPDIRECT3DDEVICE8 pD3DDevice = g_graphicsContext.Get3DDevice();
	
	// Fade down display
	D3DGAMMARAMP StartRamp, Ramp;
	pD3DDevice->GetGammaRamp(&StartRamp);
	for (int i = 49; i; --i)
	{
		for (int j = 0; j < 256; ++j)
		{
			Ramp.blue[j] = StartRamp.blue[j] * i / 50;
			Ramp.green[j] = StartRamp.green[j] * i / 50;
			Ramp.red[j] = StartRamp.red[j] * i / 50;
		}
		pD3DDevice->BlockUntilVerticalBlank();
		pD3DDevice->SetGammaRamp(D3DSGR_IMMEDIATE, &Ramp);
	}
	pD3DDevice->Clear(0, 0, D3DCLEAR_STENCIL | D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
	pD3DDevice->Present(0, 0, 0, 0);

	static bool FixedCredits = false;

	DWORD Time = 0;
	map<int, CGUIFont*> Fonts;
	for (int i = 0; i < NUM_CREDITS; ++i)
	{
		// map fonts
		if (Fonts.find(Credits[i].Font) == Fonts.end())
		{
			CStdString strFont;
			strFont.Fmt("creditsfont%d", Credits[i].Font);
			// first try loading it
			CStdString strFilename;
			strFilename.Fmt("q:\\credits\\credits-font%d.xpr", Credits[i].Font);
			CGUIFont* pFont = g_fontManager.Load(strFont, strFilename);
			if (!pFont)
			{
				// Find closest skin font size
				for (int j = 0; j < Credits[i].Font; ++j)
				{
					strFont.Fmt("font%d", Credits[i].Font + j);
					pFont = g_fontManager.GetFont(strFont);
					if (pFont)
						break;
					if (j)
					{
						strFont.Fmt("font%d", Credits[i].Font - j);
						pFont = g_fontManager.GetFont(strFont);
						if (pFont)
							break;
					}
				}
				if (!pFont)
					pFont = g_fontManager.GetFont("font13"); // should have this!
			}
			Fonts.insert(std::pair<int, CGUIFont*>(Credits[i].Font, pFont));
		}

		// validate credits
		if (!FixedCredits)
		{
			if ((Credits[i].Effects & EFF_IN_MASK) == EFF_IN_APPEAR)
			{
				Credits[i].Duration += Credits[i].InDuration;
				Credits[i].InDuration = 0;
			}
			if ((Credits[i].Effects & EFF_OUT_MASK) == EFF_OUT_APPEAR)
			{
				Credits[i].Duration += Credits[i].OutDuration;
				Credits[i].OutDuration = 0;
			}

			Credits[i].x = Credits[i].x * g_graphicsContext.GetWidth() / 100;
			Credits[i].y = Credits[i].y * g_graphicsContext.GetHeight() / 100;

			Credits[i].Time += Time;
			Time = Credits[i].Time;

			if (Credits[i].Text && !wcsicmp(Credits[i].Text, L"$SKINTITLE"))
				SkinOffset = i;
		}
	}

	int SkinIdx = 0;
	CStdString SkinCreditsPath(g_graphicsContext.GetMediaDir());
	SkinCreditsPath += "\\credits.txt";
	FILE* fp = fopen(SkinCreditsPath.c_str(), "r");
	if (fp)
	{
		static wchar_t SkinNames[6][50];
		while (fgetws(SkinNames[SkinIdx], 45, fp) && SkinIdx < 6)
		{
			int n = wcslen(SkinNames[SkinIdx]) - 1;
			while (iswspace(SkinNames[SkinIdx][n]))
				SkinNames[SkinIdx][n--] = 0;
			Credits[SkinOffset + SkinIdx].Text = SkinNames[SkinIdx];
			++SkinIdx;
		}
		fclose(fp);
	}
	if (SkinIdx)
		wcscat((wchar_t*)Credits[SkinOffset].Text, L" Skin");
	for (; SkinIdx < 6; ++SkinIdx)
		Credits[SkinOffset + SkinIdx].Text = NULL;

	FixedCredits = true;

	bool NoLogo = false;
	HRESULT hr = InitLogo(pD3DDevice);
	if (FAILED(hr))
	{
		CLog::Log("Unable to load credits logo: %x", hr);
		CleanupLogo(pD3DDevice);
		NoLogo = true;
	}

	s_hMusicStarted = CreateEvent(0, TRUE, FALSE, 0);
	s_bStopPlaying = false;
	HANDLE hMusicThread = (HANDLE)_beginthreadex(0, 0, CreditsMusicThread, "q:\\credits\\credits.mod", 0, NULL);
	WaitForSingleObject(s_hMusicStarted, INFINITE);
	CloseHandle(s_hMusicStarted);

	// Start credits loop
	for (;;)
	{
		// restore gamma
		pD3DDevice->SetGammaRamp(D3DSGR_IMMEDIATE, &StartRamp);

		DWORD StartTime = timeGetTime();
		DWORD LastTime = StartTime;

		if (WaitForSingleObject(hMusicThread, 0) == WAIT_TIMEOUT)
			LastTime = 0;

		list<CreditLine_t*> ActiveList;
		int NextCredit = 0;

		// Do render loop
		while (NextCredit < NUM_CREDITS || !ActiveList.empty())
		{
			if (WaitForSingleObject(hMusicThread, 0) == WAIT_TIMEOUT)
				Time = (DWORD)mikxboxGetPTS();
			else
				Time = timeGetTime() - StartTime;

			if (Time < LastTime)
			{
				// Wrapped!
				NextCredit = 0;
				ActiveList.clear();
			}

			// fade out if at the end
			if (NextCredit == NUM_CREDITS && ActiveList.size() == 1)
				s_bFadeMusic = true;

			pD3DDevice->Clear(0, 0, D3DCLEAR_STENCIL | D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0, 1.0f, 0);

			// Background
			if (!NoLogo)
				RenderLogo(pD3DDevice, (float)(Time - LastTime) / 1000.f);

			LastTime = Time;

			// Set scissor region - 12% top / bottom
			D3DRECT rs = { 0, g_graphicsContext.GetHeight() * 12 / 100, g_graphicsContext.GetWidth(), g_graphicsContext.GetHeight() * 88 / 100	};
			pD3DDevice->SetScissors(1, FALSE, &rs);

			// Activate new credits
			while (NextCredit < NUM_CREDITS && Credits[NextCredit].Time <= Time)
			{
				ActiveList.push_back(&Credits[NextCredit]);
				++NextCredit;
			}

			DWORD Gamma = 0;

			// Render active credits
			for (list<CreditLine_t*>::iterator iCredit = ActiveList.begin(); iCredit != ActiveList.end(); ++iCredit)
			{
				CreditLine_t* pCredit = *iCredit;

				// check for retirement
				while (Time > pCredit->Time + pCredit->InDuration + pCredit->OutDuration + pCredit->Duration)
				{
					iCredit = ActiveList.erase(iCredit);
					if (iCredit == ActiveList.end())
						break;
					pCredit = *iCredit;
				}
				if (iCredit == ActiveList.end())
					break;

				if (pCredit->Text)
				{
					// Render
					float x;
					float y;
					DWORD alpha;

					CGUIFont* pFont = Fonts.find(pCredit->Font)->second;

					if (Time < pCredit->Time + pCredit->InDuration)
					{
						#define INPROPORTION (Time - pCredit->Time) / pCredit->InDuration
						switch (pCredit->Effects & EFF_IN_MASK)
						{
						case EFF_IN_FADE:
							x = pCredit->x;
							y = pCredit->y;
							alpha = 255 * INPROPORTION;
							break;

						case EFF_IN_FLASH:
							{
								x = pCredit->x;
								y = pCredit->y;
								alpha = 0;
								DWORD g = 255 * INPROPORTION;
								if (Time + 20 >= pCredit->Time + pCredit->InDuration)
									g = 255; // make sure last frame is 255 
								if (g > Gamma)
									Gamma = g;
							}
							break;

						case EFF_IN_ASCEND:
							{
								x = pCredit->x;
								float d = (float)(g_graphicsContext.GetHeight() - pCredit->y);
								y = pCredit->y + d - (d * INPROPORTION);
								alpha = 255;
							}
							break;

						case EFF_IN_DESCEND:
							{
								x = pCredit->x;
								y = (float)pCredit->y * INPROPORTION;
								alpha = 255;
							}
							break;

						case EFF_IN_LEFT:
							break;

						case EFF_IN_RIGHT:
							break;
						}
					}
					else if (Time > pCredit->Time + pCredit->InDuration + pCredit->Duration)
					{
						#define OUTPROPORTION (Time - (pCredit->Time + pCredit->InDuration + pCredit->Duration)) / pCredit->OutDuration
						switch (pCredit->Effects & EFF_OUT_MASK)
						{
						case EFF_OUT_FADE:
							x = pCredit->x;
							y = pCredit->y;
							alpha = 255 - (255 * OUTPROPORTION);
							break;

						case EFF_OUT_FLASH:
							{
								x = pCredit->x;
								y = pCredit->y;
								alpha = 0;
								DWORD g = 255 * OUTPROPORTION;
								if (Time + 20 >= pCredit->Time + pCredit->InDuration + pCredit->OutDuration + pCredit->Duration)
									g = 255; // make sure last frame is 255 
								if (g > Gamma)
									Gamma = g;
							}
							break;

						case EFF_OUT_ASCEND:
							{
								x = pCredit->x;
								y = (float)pCredit->y - ((float)pCredit->y * OUTPROPORTION);
								alpha = 255;
							}
							break;

						case EFF_OUT_DESCEND:
							{
								x = pCredit->x;
								float d = (float)(g_graphicsContext.GetHeight() - pCredit->y);
								y = pCredit->y + (d * OUTPROPORTION);
								alpha = 255;
							}
							break;

						case EFF_OUT_LEFT:
							break;

						case EFF_OUT_RIGHT:
							break;
						}
					}
					else // not transitioning
					{
						x = pCredit->x;
						y = pCredit->y;
						alpha = 0xff;
					}

					if (alpha)
						pFont->DrawText(x, y, 0xffffff | (alpha << 24), pCredit->Text, XBFONT_CENTER_X | XBFONT_CENTER_Y);
				}
			}

			if (Gamma)
			{
				for (int j = 0; j < 256; ++j)
				{
					#define bclamp(x) (x) > 255 ? 255 : (BYTE)((x) & 0xff)
					Ramp.blue[j] = bclamp(StartRamp.blue[j] + Gamma);
					Ramp.green[j] = bclamp(StartRamp.green[j] + Gamma);
					Ramp.red[j] = bclamp(StartRamp.red[j] + Gamma);
				}
				pD3DDevice->SetGammaRamp(0, &Ramp);
			}
			else
				pD3DDevice->SetGammaRamp(0, &StartRamp);

			// Reset scissor region
			pD3DDevice->SetScissors(0, FALSE, NULL);

			// check for keypress
			g_application.ReadInput();
			if (g_application.m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_B] ||
				g_application.m_DefaultGamepad.wButtons & XINPUT_GAMEPAD_BACK ||
				g_application.m_DefaultIR_Remote.wButtons == XINPUT_IR_REMOTE_BACK ||
				g_application.m_DefaultIR_Remote.wButtons == XINPUT_IR_REMOTE_MENU)
			{
				s_bStopPlaying = true;
				CloseHandle(hMusicThread);

				// Unload fonts
				for (map<int, CGUIFont*>::iterator iFont = Fonts.begin(); iFont != Fonts.end(); ++iFont)
				{
					CStdString strFont;
					strFont.Fmt("creditsfont%d", iFont->first);
					g_fontManager.Unload(strFont);
				}

				if (!NoLogo)
					CleanupLogo(pD3DDevice);

				// clear screen and exit to gui
				pD3DDevice->Clear(0, 0, D3DCLEAR_STENCIL | D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
				pD3DDevice->SetGammaRamp(0, &StartRamp);
				pD3DDevice->Present(0, 0, 0, 0);
				g_graphicsContext.Unlock();

				if (NeedUnpause)
					g_application.m_pPlayer->Pause();

				return;
			}

			// present scene
			pD3DDevice->BlockUntilVerticalBlank();
			pD3DDevice->Present(0, 0, 0, 0);
		}
	}
}

unsigned __stdcall CreditsMusicThread(void* pParam)
{
	CSectionLoader::Load("MOD_RX");
	CSectionLoader::Load("MOD_RW");

	if (!mikxboxInit())
	{
		SetEvent(s_hMusicStarted);
		return 1;
	}
	int MusicVol;
	mikxboxSetMusicVolume(MusicVol = 128);

	MODULE* pModule = Mod_Player_Load((char*)pParam, 127, 0);

	if (!pModule)
	{
		SetEvent(s_hMusicStarted);
		return 1;
	}

	// set to loop and jump to the end section
	pModule->loop = 1;
	pModule->sngpos = 18;
	s_bFadeMusic = false;

	Mod_Player_Start(pModule);

	SetEvent(s_hMusicStarted);

	do {
		while (!s_bStopPlaying && MusicVol > 0 && Mod_Player_Active())
		{
			MikMod_Update();
			if (s_bFadeMusic)
			{
				mikxboxSetMusicVolume(--MusicVol);
			}
		}

		if (!s_bStopPlaying)
		{
			Mod_Player_Stop();
			Mod_Player_Free(pModule);
			pModule = Mod_Player_Load((char*)pParam, 127, 0);
			pModule->loop = 1;
			pModule->sngpos = 18;
			s_bFadeMusic = false;
			mikxboxSetMusicVolume(MusicVol = 128);
			Mod_Player_Start(pModule);
		}
	} while (!s_bStopPlaying);

	Mod_Player_Free(pModule);

	mikxboxExit();

	CSectionLoader::Unload("MOD_RX");
	CSectionLoader::Unload("MOD_RW");

	return 0;
}
