/**
 * @file mpegaudiodectab.h
 * mpeg audio layer decoder tables. 
 */

const uint16_t mpa_bitrate_tab[2][3][15] = {
    { {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448 },
      {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384 },
      {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320 } },
    { {0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256},
      {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160},
      {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}
    }
};

const uint16_t mpa_freq_tab[3] = { 44100, 48000, 32000 };

/*******************************************************/
/* half mpeg encoding window (full precision) */
const int32_t mpa_enwindow[257] = {
     0,    -1,    -1,    -1,    -1,    -1,    -1,    -2,
    -2,    -2,    -2,    -3,    -3,    -4,    -4,    -5,
    -5,    -6,    -7,    -7,    -8,    -9,   -10,   -11,
   -13,   -14,   -16,   -17,   -19,   -21,   -24,   -26,
   -29,   -31,   -35,   -38,   -41,   -45,   -49,   -53,
   -58,   -63,   -68,   -73,   -79,   -85,   -91,   -97,
  -104,  -111,  -117,  -125,  -132,  -139,  -147,  -154,
  -161,  -169,  -176,  -183,  -190,  -196,  -202,  -208,
   213,   218,   222,   225,   227,   228,   228,   227,
   224,   221,   215,   208,   200,   189,   177,   163,
   146,   127,   106,    83,    57,    29,    -2,   -36,
   -72,  -111,  -153,  -197,  -244,  -294,  -347,  -401,
  -459,  -519,  -581,  -645,  -711,  -779,  -848,  -919,
  -991, -1064, -1137, -1210, -1283, -1356, -1428, -1498,
 -1567, -1634, -1698, -1759, -1817, -1870, -1919, -1962,
 -2001, -2032, -2057, -2075, -2085, -2087, -2080, -2063,
  2037,  2000,  1952,  1893,  1822,  1739,  1644,  1535,
  1414,  1280,  1131,   970,   794,   605,   402,   185,
   -45,  -288,  -545,  -814, -1095, -1388, -1692, -2006,
 -2330, -2663, -3004, -3351, -3705, -4063, -4425, -4788,
 -5153, -5517, -5879, -6237, -6589, -6935, -7271, -7597,
 -7910, -8209, -8491, -8755, -8998, -9219, -9416, -9585,
 -9727, -9838, -9916, -9959, -9966, -9935, -9863, -9750,
 -9592, -9389, -9139, -8840, -8492, -8092, -7640, -7134,
  6574,  5959,  5288,  4561,  3776,  2935,  2037,  1082,
    70,  -998, -2122, -3300, -4533, -5818, -7154, -8540,
 -9975,-11455,-12980,-14548,-16155,-17799,-19478,-21189,
-22929,-24694,-26482,-28289,-30112,-31947,-33791,-35640,
-37489,-39336,-41176,-43006,-44821,-46617,-48390,-50137,
-51853,-53534,-55178,-56778,-58333,-59838,-61289,-62684,
-64019,-65290,-66494,-67629,-68692,-69679,-70590,-71420,
-72169,-72835,-73415,-73908,-74313,-74630,-74856,-74992,
 75038,
};

/*******************************************************/
/* layer 2 tables */

const int sblimit_table[5] = { 27 , 30 , 8, 12 , 30 };

const int quant_steps[17] = {
    3,     5,    7,    9,    15,
    31,    63,  127,  255,   511,
    1023,  2047, 4095, 8191, 16383,
    32767, 65535
};

/* we use a negative value if grouped */
const int quant_bits[17] = {
    -5,  -7,  3, -10, 4, 
     5,  6,  7,  8,  9,
    10, 11, 12, 13, 14,
    15, 16 
};

/* encoding tables which give the quantization index. Note how it is
   possible to store them efficiently ! */
static const unsigned char alloc_table_0[] = {
 4,  0,  2,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 
 4,  0,  2,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 
 4,  0,  2,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 
 4,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 16, 
 4,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 16, 
 4,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 16, 
 4,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 16, 
 4,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 16, 
 4,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 16, 
 4,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 16, 
 4,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 16, 
 3,  0,  1,  2,  3,  4,  5, 16, 
 3,  0,  1,  2,  3,  4,  5, 16, 
 3,  0,  1,  2,  3,  4,  5, 16, 
 3,  0,  1,  2,  3,  4,  5, 16, 
 3,  0,  1,  2,  3,  4,  5, 16, 
 3,  0,  1,  2,  3,  4,  5, 16, 
 3,  0,  1,  2,  3,  4,  5, 16, 
 3,  0,  1,  2,  3,  4,  5, 16, 
 3,  0,  1,  2,  3,  4,  5, 16, 
 3,  0,  1,  2,  3,  4,  5, 16, 
 3,  0,  1,  2,  3,  4,  5, 16, 
 3,  0,  1,  2,  3,  4,  5, 16, 
 2,  0,  1, 16, 
 2,  0,  1, 16, 
 2,  0,  1, 16, 
 2,  0,  1, 16, 
};

static const unsigned char alloc_table_1[] = {
 4,  0,  2,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 
 4,  0,  2,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 
 4,  0,  2,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 
 4,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 16, 
 4,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 16, 
 4,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 16, 
 4,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 16, 
 4,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 16, 
 4,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 16, 
 4,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 16, 
 4,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 16, 
 3,  0,  1,  2,  3,  4,  5, 16, 
 3,  0,  1,  2,  3,  4,  5, 16, 
 3,  0,  1,  2,  3,  4,  5, 16, 
 3,  0,  1,  2,  3,  4,  5, 16, 
 3,  0,  1,  2,  3,  4,  5, 16, 
 3,  0,  1,  2,  3,  4,  5, 16, 
 3,  0,  1,  2,  3,  4,  5, 16, 
 3,  0,  1,  2,  3,  4,  5, 16, 
 3,  0,  1,  2,  3,  4,  5, 16, 
 3,  0,  1,  2,  3,  4,  5, 16, 
 3,  0,  1,  2,  3,  4,  5, 16, 
 3,  0,  1,  2,  3,  4,  5, 16, 
 2,  0,  1, 16, 
 2,  0,  1, 16, 
 2,  0,  1, 16, 
 2,  0,  1, 16, 
 2,  0,  1, 16, 
 2,  0,  1, 16, 
 2,  0,  1, 16, 
};

static const unsigned char alloc_table_2[] = {
 4,  0,  1,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 
 4,  0,  1,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 
 3,  0,  1,  3,  4,  5,  6,  7, 
 3,  0,  1,  3,  4,  5,  6,  7, 
 3,  0,  1,  3,  4,  5,  6,  7, 
 3,  0,  1,  3,  4,  5,  6,  7, 
 3,  0,  1,  3,  4,  5,  6,  7, 
 3,  0,  1,  3,  4,  5,  6,  7, 
};

static const unsigned char alloc_table_3[] = {
 4,  0,  1,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 
 4,  0,  1,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 
 3,  0,  1,  3,  4,  5,  6,  7, 
 3,  0,  1,  3,  4,  5,  6,  7, 
 3,  0,  1,  3,  4,  5,  6,  7, 
 3,  0,  1,  3,  4,  5,  6,  7, 
 3,  0,  1,  3,  4,  5,  6,  7, 
 3,  0,  1,  3,  4,  5,  6,  7, 
 3,  0,  1,  3,  4,  5,  6,  7, 
 3,  0,  1,  3,  4,  5,  6,  7, 
 3,  0,  1,  3,  4,  5,  6,  7, 
 3,  0,  1,  3,  4,  5,  6,  7, 
};

static const unsigned char alloc_table_4[] = {
 4,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
 4,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 
 4,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 
 4,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 
 3,  0,  1,  3,  4,  5,  6,  7, 
 3,  0,  1,  3,  4,  5,  6,  7, 
 3,  0,  1,  3,  4,  5,  6,  7, 
 3,  0,  1,  3,  4,  5,  6,  7, 
 3,  0,  1,  3,  4,  5,  6,  7, 
 3,  0,  1,  3,  4,  5,  6,  7, 
 3,  0,  1,  3,  4,  5,  6,  7, 
 2,  0,  1,  3, 
 2,  0,  1,  3, 
 2,  0,  1,  3, 
 2,  0,  1,  3, 
 2,  0,  1,  3, 
 2,  0,  1,  3, 
 2,  0,  1,  3, 
 2,  0,  1,  3, 
 2,  0,  1,  3, 
 2,  0,  1,  3, 
 2,  0,  1,  3, 
 2,  0,  1,  3, 
 2,  0,  1,  3, 
 2,  0,  1,  3, 
 2,  0,  1,  3, 
 2,  0,  1,  3, 
 2,  0,  1,  3, 
 2,  0,  1,  3, 
 2,  0,  1,  3, 
};

const unsigned char *alloc_tables[5] = 
{ alloc_table_0, alloc_table_1, alloc_table_2, alloc_table_3, alloc_table_4, };

/*******************************************************/
/* layer 3 tables */

/* layer3 scale factor size */
static const uint8_t slen_table[2][16] = {
    { 0, 0, 0, 0, 3, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4 },
    { 0, 1, 2, 3, 0, 1, 2, 3, 1, 2, 3, 1, 2, 3, 2, 3 },
};

/* number of lsf scale factors for a given size */
static const uint8_t lsf_nsf_table[6][3][4] = {
    { {  6,  5,  5, 5 }, {  9,  9,  9, 9 }, {  6,  9,  9, 9 } },
    { {  6,  5,  7, 3 }, {  9,  9, 12, 6 }, {  6,  9, 12, 6 } },
    { { 11, 10,  0, 0 }, { 18, 18,  0, 0 }, { 15, 18,  0, 0 } },
    { {  7,  7,  7, 0 }, { 12, 12, 12, 0 }, {  6, 15, 12, 0 } }, 
    { {  6,  6,  6, 3 }, { 12,  9,  9, 6 }, {  6, 12,  9, 6 } },
    { {  8,  8,  5, 0 }, { 15, 12,  9, 0 }, {  6, 18,  9, 0 } },
};

/* mpegaudio layer 3 huffman tables */

const uint16_t mpa_huffcodes_1[4] = {
 0x0001, 0x0001, 0x0001, 0x0000,
};

const uint8_t mpa_huffbits_1[4] = {
  1,  3,  2,  3,
};

const uint16_t mpa_huffcodes_2[9] = {
 0x0001, 0x0002, 0x0001, 0x0003, 0x0001, 0x0001, 0x0003, 0x0002,
 0x0000,
};

const uint8_t mpa_huffbits_2[9] = {
  1,  3,  6,  3,  3,  5,  5,  5,
  6,
};

const uint16_t mpa_huffcodes_3[9] = {
 0x0003, 0x0002, 0x0001, 0x0001, 0x0001, 0x0001, 0x0003, 0x0002,
 0x0000,
};

const uint8_t mpa_huffbits_3[9] = {
  2,  2,  6,  3,  2,  5,  5,  5,
  6,
};

const uint16_t mpa_huffcodes_5[16] = {
 0x0001, 0x0002, 0x0006, 0x0005, 0x0003, 0x0001, 0x0004, 0x0004,
 0x0007, 0x0005, 0x0007, 0x0001, 0x0006, 0x0001, 0x0001, 0x0000,
};

const uint8_t mpa_huffbits_5[16] = {
  1,  3,  6,  7,  3,  3,  6,  7,
  6,  6,  7,  8,  7,  6,  7,  8,
};

const uint16_t mpa_huffcodes_6[16] = {
 0x0007, 0x0003, 0x0005, 0x0001, 0x0006, 0x0002, 0x0003, 0x0002,
 0x0005, 0x0004, 0x0004, 0x0001, 0x0003, 0x0003, 0x0002, 0x0000,
};

const uint8_t mpa_huffbits_6[16] = {
  3,  3,  5,  7,  3,  2,  4,  5,
  4,  4,  5,  6,  6,  5,  6,  7,
};

const uint16_t mpa_huffcodes_7[36] = {
 0x0001, 0x0002, 0x000a, 0x0013, 0x0010, 0x000a, 0x0003, 0x0003,
 0x0007, 0x000a, 0x0005, 0x0003, 0x000b, 0x0004, 0x000d, 0x0011,
 0x0008, 0x0004, 0x000c, 0x000b, 0x0012, 0x000f, 0x000b, 0x0002,
 0x0007, 0x0006, 0x0009, 0x000e, 0x0003, 0x0001, 0x0006, 0x0004,
 0x0005, 0x0003, 0x0002, 0x0000,
};

const uint8_t mpa_huffbits_7[36] = {
  1,  3,  6,  8,  8,  9,  3,  4,
  6,  7,  7,  8,  6,  5,  7,  8,
  8,  9,  7,  7,  8,  9,  9,  9,
  7,  7,  8,  9,  9, 10,  8,  8,
  9, 10, 10, 10,
};

const uint16_t mpa_huffcodes_8[36] = {
 0x0003, 0x0004, 0x0006, 0x0012, 0x000c, 0x0005, 0x0005, 0x0001,
 0x0002, 0x0010, 0x0009, 0x0003, 0x0007, 0x0003, 0x0005, 0x000e,
 0x0007, 0x0003, 0x0013, 0x0011, 0x000f, 0x000d, 0x000a, 0x0004,
 0x000d, 0x0005, 0x0008, 0x000b, 0x0005, 0x0001, 0x000c, 0x0004,
 0x0004, 0x0001, 0x0001, 0x0000,
};

const uint8_t mpa_huffbits_8[36] = {
  2,  3,  6,  8,  8,  9,  3,  2,
  4,  8,  8,  8,  6,  4,  6,  8,
  8,  9,  8,  8,  8,  9,  9, 10,
  8,  7,  8,  9, 10, 10,  9,  8,
  9,  9, 11, 11,
};

const uint16_t mpa_huffcodes_9[36] = {
 0x0007, 0x0005, 0x0009, 0x000e, 0x000f, 0x0007, 0x0006, 0x0004,
 0x0005, 0x0005, 0x0006, 0x0007, 0x0007, 0x0006, 0x0008, 0x0008,
 0x0008, 0x0005, 0x000f, 0x0006, 0x0009, 0x000a, 0x0005, 0x0001,
 0x000b, 0x0007, 0x0009, 0x0006, 0x0004, 0x0001, 0x000e, 0x0004,
 0x0006, 0x0002, 0x0006, 0x0000,
};

const uint8_t mpa_huffbits_9[36] = {
  3,  3,  5,  6,  8,  9,  3,  3,
  4,  5,  6,  8,  4,  4,  5,  6,
  7,  8,  6,  5,  6,  7,  7,  8,
  7,  6,  7,  7,  8,  9,  8,  7,
  8,  8,  9,  9,
};

const uint16_t mpa_huffcodes_10[64] = {
 0x0001, 0x0002, 0x000a, 0x0017, 0x0023, 0x001e, 0x000c, 0x0011,
 0x0003, 0x0003, 0x0008, 0x000c, 0x0012, 0x0015, 0x000c, 0x0007,
 0x000b, 0x0009, 0x000f, 0x0015, 0x0020, 0x0028, 0x0013, 0x0006,
 0x000e, 0x000d, 0x0016, 0x0022, 0x002e, 0x0017, 0x0012, 0x0007,
 0x0014, 0x0013, 0x0021, 0x002f, 0x001b, 0x0016, 0x0009, 0x0003,
 0x001f, 0x0016, 0x0029, 0x001a, 0x0015, 0x0014, 0x0005, 0x0003,
 0x000e, 0x000d, 0x000a, 0x000b, 0x0010, 0x0006, 0x0005, 0x0001,
 0x0009, 0x0008, 0x0007, 0x0008, 0x0004, 0x0004, 0x0002, 0x0000,
};

const uint8_t mpa_huffbits_10[64] = {
  1,  3,  6,  8,  9,  9,  9, 10,
  3,  4,  6,  7,  8,  9,  8,  8,
  6,  6,  7,  8,  9, 10,  9,  9,
  7,  7,  8,  9, 10, 10,  9, 10,
  8,  8,  9, 10, 10, 10, 10, 10,
  9,  9, 10, 10, 11, 11, 10, 11,
  8,  8,  9, 10, 10, 10, 11, 11,
  9,  8,  9, 10, 10, 11, 11, 11,
};

const uint16_t mpa_huffcodes_11[64] = {
 0x0003, 0x0004, 0x000a, 0x0018, 0x0022, 0x0021, 0x0015, 0x000f,
 0x0005, 0x0003, 0x0004, 0x000a, 0x0020, 0x0011, 0x000b, 0x000a,
 0x000b, 0x0007, 0x000d, 0x0012, 0x001e, 0x001f, 0x0014, 0x0005,
 0x0019, 0x000b, 0x0013, 0x003b, 0x001b, 0x0012, 0x000c, 0x0005,
 0x0023, 0x0021, 0x001f, 0x003a, 0x001e, 0x0010, 0x0007, 0x0005,
 0x001c, 0x001a, 0x0020, 0x0013, 0x0011, 0x000f, 0x0008, 0x000e,
 0x000e, 0x000c, 0x0009, 0x000d, 0x000e, 0x0009, 0x0004, 0x0001,
 0x000b, 0x0004, 0x0006, 0x0006, 0x0006, 0x0003, 0x0002, 0x0000,
};

const uint8_t mpa_huffbits_11[64] = {
  2,  3,  5,  7,  8,  9,  8,  9,
  3,  3,  4,  6,  8,  8,  7,  8,
  5,  5,  6,  7,  8,  9,  8,  8,
  7,  6,  7,  9,  8, 10,  8,  9,
  8,  8,  8,  9,  9, 10,  9, 10,
  8,  8,  9, 10, 10, 11, 10, 11,
  8,  7,  7,  8,  9, 10, 10, 10,
  8,  7,  8,  9, 10, 10, 10, 10,
};

const uint16_t mpa_huffcodes_12[64] = {
 0x0009, 0x0006, 0x0010, 0x0021, 0x0029, 0x0027, 0x0026, 0x001a,
 0x0007, 0x0005, 0x0006, 0x0009, 0x0017, 0x0010, 0x001a, 0x000b,
 0x0011, 0x0007, 0x000b, 0x000e, 0x0015, 0x001e, 0x000a, 0x0007,
 0x0011, 0x000a, 0x000f, 0x000c, 0x0012, 0x001c, 0x000e, 0x0005,
 0x0020, 0x000d, 0x0016, 0x0013, 0x0012, 0x0010, 0x0009, 0x0005,
 0x0028, 0x0011, 0x001f, 0x001d, 0x0011, 0x000d, 0x0004, 0x0002,
 0x001b, 0x000c, 0x000b, 0x000f, 0x000a, 0x0007, 0x0004, 0x0001,
 0x001b, 0x000c, 0x0008, 0x000c, 0x0006, 0x0003, 0x0001, 0x0000,
};

const uint8_t mpa_huffbits_12[64] = {
  4,  3,  5,  7,  8,  9,  9,  9,
  3,  3,  4,  5,  7,  7,  8,  8,
  5,  4,  5,  6,  7,  8,  7,  8,
  6,  5,  6,  6,  7,  8,  8,  8,
  7,  6,  7,  7,  8,  8,  8,  9,
  8,  7,  8,  8,  8,  9,  8,  9,
  8,  7,  7,  8,  8,  9,  9, 10,
  9,  8,  8,  9,  9,  9,  9, 10,
};

const uint16_t mpa_huffcodes_13[256] = {
 0x0001, 0x0005, 0x000e, 0x0015, 0x0022, 0x0033, 0x002e, 0x0047,
 0x002a, 0x0034, 0x0044, 0x0034, 0x0043, 0x002c, 0x002b, 0x0013,
 0x0003, 0x0004, 0x000c, 0x0013, 0x001f, 0x001a, 0x002c, 0x0021,
 0x001f, 0x0018, 0x0020, 0x0018, 0x001f, 0x0023, 0x0016, 0x000e,
 0x000f, 0x000d, 0x0017, 0x0024, 0x003b, 0x0031, 0x004d, 0x0041,
 0x001d, 0x0028, 0x001e, 0x0028, 0x001b, 0x0021, 0x002a, 0x0010,
 0x0016, 0x0014, 0x0025, 0x003d, 0x0038, 0x004f, 0x0049, 0x0040,
 0x002b, 0x004c, 0x0038, 0x0025, 0x001a, 0x001f, 0x0019, 0x000e,
 0x0023, 0x0010, 0x003c, 0x0039, 0x0061, 0x004b, 0x0072, 0x005b,
 0x0036, 0x0049, 0x0037, 0x0029, 0x0030, 0x0035, 0x0017, 0x0018,
 0x003a, 0x001b, 0x0032, 0x0060, 0x004c, 0x0046, 0x005d, 0x0054,
 0x004d, 0x003a, 0x004f, 0x001d, 0x004a, 0x0031, 0x0029, 0x0011,
 0x002f, 0x002d, 0x004e, 0x004a, 0x0073, 0x005e, 0x005a, 0x004f,
 0x0045, 0x0053, 0x0047, 0x0032, 0x003b, 0x0026, 0x0024, 0x000f,
 0x0048, 0x0022, 0x0038, 0x005f, 0x005c, 0x0055, 0x005b, 0x005a,
 0x0056, 0x0049, 0x004d, 0x0041, 0x0033, 0x002c, 0x002b, 0x002a,
 0x002b, 0x0014, 0x001e, 0x002c, 0x0037, 0x004e, 0x0048, 0x0057,
 0x004e, 0x003d, 0x002e, 0x0036, 0x0025, 0x001e, 0x0014, 0x0010,
 0x0035, 0x0019, 0x0029, 0x0025, 0x002c, 0x003b, 0x0036, 0x0051,
 0x0042, 0x004c, 0x0039, 0x0036, 0x0025, 0x0012, 0x0027, 0x000b,
 0x0023, 0x0021, 0x001f, 0x0039, 0x002a, 0x0052, 0x0048, 0x0050,
 0x002f, 0x003a, 0x0037, 0x0015, 0x0016, 0x001a, 0x0026, 0x0016,
 0x0035, 0x0019, 0x0017, 0x0026, 0x0046, 0x003c, 0x0033, 0x0024,
 0x0037, 0x001a, 0x0022, 0x0017, 0x001b, 0x000e, 0x0009, 0x0007,
 0x0022, 0x0020, 0x001c, 0x0027, 0x0031, 0x004b, 0x001e, 0x0034,
 0x0030, 0x0028, 0x0034, 0x001c, 0x0012, 0x0011, 0x0009, 0x0005,
 0x002d, 0x0015, 0x0022, 0x0040, 0x0038, 0x0032, 0x0031, 0x002d,
 0x001f, 0x0013, 0x000c, 0x000f, 0x000a, 0x0007, 0x0006, 0x0003,
 0x0030, 0x0017, 0x0014, 0x0027, 0x0024, 0x0023, 0x0035, 0x0015,
 0x0010, 0x0017, 0x000d, 0x000a, 0x0006, 0x0001, 0x0004, 0x0002,
 0x0010, 0x000f, 0x0011, 0x001b, 0x0019, 0x0014, 0x001d, 0x000b,
 0x0011, 0x000c, 0x0010, 0x0008, 0x0001, 0x0001, 0x0000, 0x0001,
};

const uint8_t mpa_huffbits_13[256] = {
  1,  4,  6,  7,  8,  9,  9, 10,
  9, 10, 11, 11, 12, 12, 13, 13,
  3,  4,  6,  7,  8,  8,  9,  9,
  9,  9, 10, 10, 11, 12, 12, 12,
  6,  6,  7,  8,  9,  9, 10, 10,
  9, 10, 10, 11, 11, 12, 13, 13,
  7,  7,  8,  9,  9, 10, 10, 10,
 10, 11, 11, 11, 11, 12, 13, 13,
  8,  7,  9,  9, 10, 10, 11, 11,
 10, 11, 11, 12, 12, 13, 13, 14,
  9,  8,  9, 10, 10, 10, 11, 11,
 11, 11, 12, 11, 13, 13, 14, 14,
  9,  9, 10, 10, 11, 11, 11, 11,
 11, 12, 12, 12, 13, 13, 14, 14,
 10,  9, 10, 11, 11, 11, 12, 12,
 12, 12, 13, 13, 13, 14, 16, 16,
  9,  8,  9, 10, 10, 11, 11, 12,
 12, 12, 12, 13, 13, 14, 15, 15,
 10,  9, 10, 10, 11, 11, 11, 13,
 12, 13, 13, 14, 14, 14, 16, 15,
 10, 10, 10, 11, 11, 12, 12, 13,
 12, 13, 14, 13, 14, 15, 16, 17,
 11, 10, 10, 11, 12, 12, 12, 12,
 13, 13, 13, 14, 15, 15, 15, 16,
 11, 11, 11, 12, 12, 13, 12, 13,
 14, 14, 15, 15, 15, 16, 16, 16,
 12, 11, 12, 13, 13, 13, 14, 14,
 14, 14, 14, 15, 16, 15, 16, 16,
 13, 12, 12, 13, 13, 13, 15, 14,
 14, 17, 15, 15, 15, 17, 16, 16,
 12, 12, 13, 14, 14, 14, 15, 14,
 15, 15, 16, 16, 19, 18, 19, 16,
};

const uint16_t mpa_huffcodes_15[256] = {
 0x0007, 0x000c, 0x0012, 0x0035, 0x002f, 0x004c, 0x007c, 0x006c,
 0x0059, 0x007b, 0x006c, 0x0077, 0x006b, 0x0051, 0x007a, 0x003f,
 0x000d, 0x0005, 0x0010, 0x001b, 0x002e, 0x0024, 0x003d, 0x0033,
 0x002a, 0x0046, 0x0034, 0x0053, 0x0041, 0x0029, 0x003b, 0x0024,
 0x0013, 0x0011, 0x000f, 0x0018, 0x0029, 0x0022, 0x003b, 0x0030,
 0x0028, 0x0040, 0x0032, 0x004e, 0x003e, 0x0050, 0x0038, 0x0021,
 0x001d, 0x001c, 0x0019, 0x002b, 0x0027, 0x003f, 0x0037, 0x005d,
 0x004c, 0x003b, 0x005d, 0x0048, 0x0036, 0x004b, 0x0032, 0x001d,
 0x0034, 0x0016, 0x002a, 0x0028, 0x0043, 0x0039, 0x005f, 0x004f,
 0x0048, 0x0039, 0x0059, 0x0045, 0x0031, 0x0042, 0x002e, 0x001b,
 0x004d, 0x0025, 0x0023, 0x0042, 0x003a, 0x0034, 0x005b, 0x004a,
 0x003e, 0x0030, 0x004f, 0x003f, 0x005a, 0x003e, 0x0028, 0x0026,
 0x007d, 0x0020, 0x003c, 0x0038, 0x0032, 0x005c, 0x004e, 0x0041,
 0x0037, 0x0057, 0x0047, 0x0033, 0x0049, 0x0033, 0x0046, 0x001e,
 0x006d, 0x0035, 0x0031, 0x005e, 0x0058, 0x004b, 0x0042, 0x007a,
 0x005b, 0x0049, 0x0038, 0x002a, 0x0040, 0x002c, 0x0015, 0x0019,
 0x005a, 0x002b, 0x0029, 0x004d, 0x0049, 0x003f, 0x0038, 0x005c,
 0x004d, 0x0042, 0x002f, 0x0043, 0x0030, 0x0035, 0x0024, 0x0014,
 0x0047, 0x0022, 0x0043, 0x003c, 0x003a, 0x0031, 0x0058, 0x004c,
 0x0043, 0x006a, 0x0047, 0x0036, 0x0026, 0x0027, 0x0017, 0x000f,
 0x006d, 0x0035, 0x0033, 0x002f, 0x005a, 0x0052, 0x003a, 0x0039,
 0x0030, 0x0048, 0x0039, 0x0029, 0x0017, 0x001b, 0x003e, 0x0009,
 0x0056, 0x002a, 0x0028, 0x0025, 0x0046, 0x0040, 0x0034, 0x002b,
 0x0046, 0x0037, 0x002a, 0x0019, 0x001d, 0x0012, 0x000b, 0x000b,
 0x0076, 0x0044, 0x001e, 0x0037, 0x0032, 0x002e, 0x004a, 0x0041,
 0x0031, 0x0027, 0x0018, 0x0010, 0x0016, 0x000d, 0x000e, 0x0007,
 0x005b, 0x002c, 0x0027, 0x0026, 0x0022, 0x003f, 0x0034, 0x002d,
 0x001f, 0x0034, 0x001c, 0x0013, 0x000e, 0x0008, 0x0009, 0x0003,
 0x007b, 0x003c, 0x003a, 0x0035, 0x002f, 0x002b, 0x0020, 0x0016,
 0x0025, 0x0018, 0x0011, 0x000c, 0x000f, 0x000a, 0x0002, 0x0001,
 0x0047, 0x0025, 0x0022, 0x001e, 0x001c, 0x0014, 0x0011, 0x001a,
 0x0015, 0x0010, 0x000a, 0x0006, 0x0008, 0x0006, 0x0002, 0x0000,
};

const uint8_t mpa_huffbits_15[256] = {
  3,  4,  5,  7,  7,  8,  9,  9,
  9, 10, 10, 11, 11, 11, 12, 13,
  4,  3,  5,  6,  7,  7,  8,  8,
  8,  9,  9, 10, 10, 10, 11, 11,
  5,  5,  5,  6,  7,  7,  8,  8,
  8,  9,  9, 10, 10, 11, 11, 11,
  6,  6,  6,  7,  7,  8,  8,  9,
  9,  9, 10, 10, 10, 11, 11, 11,
  7,  6,  7,  7,  8,  8,  9,  9,
  9,  9, 10, 10, 10, 11, 11, 11,
  8,  7,  7,  8,  8,  8,  9,  9,
  9,  9, 10, 10, 11, 11, 11, 12,
  9,  7,  8,  8,  8,  9,  9,  9,
  9, 10, 10, 10, 11, 11, 12, 12,
  9,  8,  8,  9,  9,  9,  9, 10,
 10, 10, 10, 10, 11, 11, 11, 12,
  9,  8,  8,  9,  9,  9,  9, 10,
 10, 10, 10, 11, 11, 12, 12, 12,
  9,  8,  9,  9,  9,  9, 10, 10,
 10, 11, 11, 11, 11, 12, 12, 12,
 10,  9,  9,  9, 10, 10, 10, 10,
 10, 11, 11, 11, 11, 12, 13, 12,
 10,  9,  9,  9, 10, 10, 10, 10,
 11, 11, 11, 11, 12, 12, 12, 13,
 11, 10,  9, 10, 10, 10, 11, 11,
 11, 11, 11, 11, 12, 12, 13, 13,
 11, 10, 10, 10, 10, 11, 11, 11,
 11, 12, 12, 12, 12, 12, 13, 13,
 12, 11, 11, 11, 11, 11, 11, 11,
 12, 12, 12, 12, 13, 13, 12, 13,
 12, 11, 11, 11, 11, 11, 11, 12,
 12, 12, 12, 12, 13, 13, 13, 13,
};

const uint16_t mpa_huffcodes_16[256] = {
 0x0001, 0x0005, 0x000e, 0x002c, 0x004a, 0x003f, 0x006e, 0x005d,
 0x00ac, 0x0095, 0x008a, 0x00f2, 0x00e1, 0x00c3, 0x0178, 0x0011,
 0x0003, 0x0004, 0x000c, 0x0014, 0x0023, 0x003e, 0x0035, 0x002f,
 0x0053, 0x004b, 0x0044, 0x0077, 0x00c9, 0x006b, 0x00cf, 0x0009,
 0x000f, 0x000d, 0x0017, 0x0026, 0x0043, 0x003a, 0x0067, 0x005a,
 0x00a1, 0x0048, 0x007f, 0x0075, 0x006e, 0x00d1, 0x00ce, 0x0010,
 0x002d, 0x0015, 0x0027, 0x0045, 0x0040, 0x0072, 0x0063, 0x0057,
 0x009e, 0x008c, 0x00fc, 0x00d4, 0x00c7, 0x0183, 0x016d, 0x001a,
 0x004b, 0x0024, 0x0044, 0x0041, 0x0073, 0x0065, 0x00b3, 0x00a4,
 0x009b, 0x0108, 0x00f6, 0x00e2, 0x018b, 0x017e, 0x016a, 0x0009,
 0x0042, 0x001e, 0x003b, 0x0038, 0x0066, 0x00b9, 0x00ad, 0x0109,
 0x008e, 0x00fd, 0x00e8, 0x0190, 0x0184, 0x017a, 0x01bd, 0x0010,
 0x006f, 0x0036, 0x0034, 0x0064, 0x00b8, 0x00b2, 0x00a0, 0x0085,
 0x0101, 0x00f4, 0x00e4, 0x00d9, 0x0181, 0x016e, 0x02cb, 0x000a,
 0x0062, 0x0030, 0x005b, 0x0058, 0x00a5, 0x009d, 0x0094, 0x0105,
 0x00f8, 0x0197, 0x018d, 0x0174, 0x017c, 0x0379, 0x0374, 0x0008,
 0x0055, 0x0054, 0x0051, 0x009f, 0x009c, 0x008f, 0x0104, 0x00f9,
 0x01ab, 0x0191, 0x0188, 0x017f, 0x02d7, 0x02c9, 0x02c4, 0x0007,
 0x009a, 0x004c, 0x0049, 0x008d, 0x0083, 0x0100, 0x00f5, 0x01aa,
 0x0196, 0x018a, 0x0180, 0x02df, 0x0167, 0x02c6, 0x0160, 0x000b,
 0x008b, 0x0081, 0x0043, 0x007d, 0x00f7, 0x00e9, 0x00e5, 0x00db,
 0x0189, 0x02e7, 0x02e1, 0x02d0, 0x0375, 0x0372, 0x01b7, 0x0004,
 0x00f3, 0x0078, 0x0076, 0x0073, 0x00e3, 0x00df, 0x018c, 0x02ea,
 0x02e6, 0x02e0, 0x02d1, 0x02c8, 0x02c2, 0x00df, 0x01b4, 0x0006,
 0x00ca, 0x00e0, 0x00de, 0x00da, 0x00d8, 0x0185, 0x0182, 0x017d,
 0x016c, 0x0378, 0x01bb, 0x02c3, 0x01b8, 0x01b5, 0x06c0, 0x0004,
 0x02eb, 0x00d3, 0x00d2, 0x00d0, 0x0172, 0x017b, 0x02de, 0x02d3,
 0x02ca, 0x06c7, 0x0373, 0x036d, 0x036c, 0x0d83, 0x0361, 0x0002,
 0x0179, 0x0171, 0x0066, 0x00bb, 0x02d6, 0x02d2, 0x0166, 0x02c7,
 0x02c5, 0x0362, 0x06c6, 0x0367, 0x0d82, 0x0366, 0x01b2, 0x0000,
 0x000c, 0x000a, 0x0007, 0x000b, 0x000a, 0x0011, 0x000b, 0x0009,
 0x000d, 0x000c, 0x000a, 0x0007, 0x0005, 0x0003, 0x0001, 0x0003,
};

const uint8_t mpa_huffbits_16[256] = {
  1,  4,  6,  8,  9,  9, 10, 10,
 11, 11, 11, 12, 12, 12, 13,  9,
  3,  4,  6,  7,  8,  9,  9,  9,
 10, 10, 10, 11, 12, 11, 12,  8,
  6,  6,  7,  8,  9,  9, 10, 10,
 11, 10, 11, 11, 11, 12, 12,  9,
  8,  7,  8,  9,  9, 10, 10, 10,
 11, 11, 12, 12, 12, 13, 13, 10,
  9,  8,  9,  9, 10, 10, 11, 11,
 11, 12, 12, 12, 13, 13, 13,  9,
  9,  8,  9,  9, 10, 11, 11, 12,
 11, 12, 12, 13, 13, 13, 14, 10,
 10,  9,  9, 10, 11, 11, 11, 11,
 12, 12, 12, 12, 13, 13, 14, 10,
 10,  9, 10, 10, 11, 11, 11, 12,
 12, 13, 13, 13, 13, 15, 15, 10,
 10, 10, 10, 11, 11, 11, 12, 12,
 13, 13, 13, 13, 14, 14, 14, 10,
 11, 10, 10, 11, 11, 12, 12, 13,
 13, 13, 13, 14, 13, 14, 13, 11,
 11, 11, 10, 11, 12, 12, 12, 12,
 13, 14, 14, 14, 15, 15, 14, 10,
 12, 11, 11, 11, 12, 12, 13, 14,
 14, 14, 14, 14, 14, 13, 14, 11,
 12, 12, 12, 12, 12, 13, 13, 13,
 13, 15, 14, 14, 14, 14, 16, 11,
 14, 12, 12, 12, 13, 13, 14, 14,
 14, 16, 15, 15, 15, 17, 15, 11,
 13, 13, 11, 12, 14, 14, 13, 14,
 14, 15, 16, 15, 17, 15, 14, 11,
  9,  8,  8,  9,  9, 10, 10, 10,
 11, 11, 11, 11, 11, 11, 11,  8,
};

const uint16_t mpa_huffcodes_24[256] = {
 0x000f, 0x000d, 0x002e, 0x0050, 0x0092, 0x0106, 0x00f8, 0x01b2,
 0x01aa, 0x029d, 0x028d, 0x0289, 0x026d, 0x0205, 0x0408, 0x0058,
 0x000e, 0x000c, 0x0015, 0x0026, 0x0047, 0x0082, 0x007a, 0x00d8,
 0x00d1, 0x00c6, 0x0147, 0x0159, 0x013f, 0x0129, 0x0117, 0x002a,
 0x002f, 0x0016, 0x0029, 0x004a, 0x0044, 0x0080, 0x0078, 0x00dd,
 0x00cf, 0x00c2, 0x00b6, 0x0154, 0x013b, 0x0127, 0x021d, 0x0012,
 0x0051, 0x0027, 0x004b, 0x0046, 0x0086, 0x007d, 0x0074, 0x00dc,
 0x00cc, 0x00be, 0x00b2, 0x0145, 0x0137, 0x0125, 0x010f, 0x0010,
 0x0093, 0x0048, 0x0045, 0x0087, 0x007f, 0x0076, 0x0070, 0x00d2,
 0x00c8, 0x00bc, 0x0160, 0x0143, 0x0132, 0x011d, 0x021c, 0x000e,
 0x0107, 0x0042, 0x0081, 0x007e, 0x0077, 0x0072, 0x00d6, 0x00ca,
 0x00c0, 0x00b4, 0x0155, 0x013d, 0x012d, 0x0119, 0x0106, 0x000c,
 0x00f9, 0x007b, 0x0079, 0x0075, 0x0071, 0x00d7, 0x00ce, 0x00c3,
 0x00b9, 0x015b, 0x014a, 0x0134, 0x0123, 0x0110, 0x0208, 0x000a,
 0x01b3, 0x0073, 0x006f, 0x006d, 0x00d3, 0x00cb, 0x00c4, 0x00bb,
 0x0161, 0x014c, 0x0139, 0x012a, 0x011b, 0x0213, 0x017d, 0x0011,
 0x01ab, 0x00d4, 0x00d0, 0x00cd, 0x00c9, 0x00c1, 0x00ba, 0x00b1,
 0x00a9, 0x0140, 0x012f, 0x011e, 0x010c, 0x0202, 0x0179, 0x0010,
 0x014f, 0x00c7, 0x00c5, 0x00bf, 0x00bd, 0x00b5, 0x00ae, 0x014d,
 0x0141, 0x0131, 0x0121, 0x0113, 0x0209, 0x017b, 0x0173, 0x000b,
 0x029c, 0x00b8, 0x00b7, 0x00b3, 0x00af, 0x0158, 0x014b, 0x013a,
 0x0130, 0x0122, 0x0115, 0x0212, 0x017f, 0x0175, 0x016e, 0x000a,
 0x028c, 0x015a, 0x00ab, 0x00a8, 0x00a4, 0x013e, 0x0135, 0x012b,
 0x011f, 0x0114, 0x0107, 0x0201, 0x0177, 0x0170, 0x016a, 0x0006,
 0x0288, 0x0142, 0x013c, 0x0138, 0x0133, 0x012e, 0x0124, 0x011c,
 0x010d, 0x0105, 0x0200, 0x0178, 0x0172, 0x016c, 0x0167, 0x0004,
 0x026c, 0x012c, 0x0128, 0x0126, 0x0120, 0x011a, 0x0111, 0x010a,
 0x0203, 0x017c, 0x0176, 0x0171, 0x016d, 0x0169, 0x0165, 0x0002,
 0x0409, 0x0118, 0x0116, 0x0112, 0x010b, 0x0108, 0x0103, 0x017e,
 0x017a, 0x0174, 0x016f, 0x016b, 0x0168, 0x0166, 0x0164, 0x0000,
 0x002b, 0x0014, 0x0013, 0x0011, 0x000f, 0x000d, 0x000b, 0x0009,
 0x0007, 0x0006, 0x0004, 0x0007, 0x0005, 0x0003, 0x0001, 0x0003,
};

const uint8_t mpa_huffbits_24[256] = {
  4,  4,  6,  7,  8,  9,  9, 10,
 10, 11, 11, 11, 11, 11, 12,  9,
  4,  4,  5,  6,  7,  8,  8,  9,
  9,  9, 10, 10, 10, 10, 10,  8,
  6,  5,  6,  7,  7,  8,  8,  9,
  9,  9,  9, 10, 10, 10, 11,  7,
  7,  6,  7,  7,  8,  8,  8,  9,
  9,  9,  9, 10, 10, 10, 10,  7,
  8,  7,  7,  8,  8,  8,  8,  9,
  9,  9, 10, 10, 10, 10, 11,  7,
  9,  7,  8,  8,  8,  8,  9,  9,
  9,  9, 10, 10, 10, 10, 10,  7,
  9,  8,  8,  8,  8,  9,  9,  9,
  9, 10, 10, 10, 10, 10, 11,  7,
 10,  8,  8,  8,  9,  9,  9,  9,
 10, 10, 10, 10, 10, 11, 11,  8,
 10,  9,  9,  9,  9,  9,  9,  9,
  9, 10, 10, 10, 10, 11, 11,  8,
 10,  9,  9,  9,  9,  9,  9, 10,
 10, 10, 10, 10, 11, 11, 11,  8,
 11,  9,  9,  9,  9, 10, 10, 10,
 10, 10, 10, 11, 11, 11, 11,  8,
 11, 10,  9,  9,  9, 10, 10, 10,
 10, 10, 10, 11, 11, 11, 11,  8,
 11, 10, 10, 10, 10, 10, 10, 10,
 10, 10, 11, 11, 11, 11, 11,  8,
 11, 10, 10, 10, 10, 10, 10, 10,
 11, 11, 11, 11, 11, 11, 11,  8,
 12, 10, 10, 10, 10, 10, 10, 11,
 11, 11, 11, 11, 11, 11, 11,  8,
  8,  7,  7,  7,  7,  7,  7,  7,
  7,  7,  7,  8,  8,  8,  8,  4,
};

const HuffTable mpa_huff_tables[16] = {
{ 1, NULL, NULL },
{ 2, mpa_huffbits_1, mpa_huffcodes_1 },
{ 3, mpa_huffbits_2, mpa_huffcodes_2 },
{ 3, mpa_huffbits_3, mpa_huffcodes_3 },
{ 4, mpa_huffbits_5, mpa_huffcodes_5 },
{ 4, mpa_huffbits_6, mpa_huffcodes_6 },
{ 6, mpa_huffbits_7, mpa_huffcodes_7 },
{ 6, mpa_huffbits_8, mpa_huffcodes_8 },
{ 6, mpa_huffbits_9, mpa_huffcodes_9 },
{ 8, mpa_huffbits_10, mpa_huffcodes_10 },
{ 8, mpa_huffbits_11, mpa_huffcodes_11 },
{ 8, mpa_huffbits_12, mpa_huffcodes_12 },
{ 16, mpa_huffbits_13, mpa_huffcodes_13 },
{ 16, mpa_huffbits_15, mpa_huffcodes_15 },
{ 16, mpa_huffbits_16, mpa_huffcodes_16 },
{ 16, mpa_huffbits_24, mpa_huffcodes_24 },
};

const uint8_t mpa_huff_data[32][2] = {
{ 0, 0 },
{ 1, 0 },
{ 2, 0 },
{ 3, 0 },
{ 0, 0 },
{ 4, 0 },
{ 5, 0 },
{ 6, 0 },
{ 7, 0 },
{ 8, 0 },
{ 9, 0 },
{ 10, 0 },
{ 11, 0 },
{ 12, 0 },
{ 0, 0 },
{ 13, 0 },
{ 14, 1 },
{ 14, 2 },
{ 14, 3 },
{ 14, 4 },
{ 14, 6 },
{ 14, 8 },
{ 14, 10 },
{ 14, 13 },
{ 15, 4 },
{ 15, 5 },
{ 15, 6 },
{ 15, 7 },
{ 15, 8 },
{ 15, 9 },
{ 15, 11 },
{ 15, 13 },
};


/* huffman tables for quadrules */
static const uint8_t mpa_quad_codes[2][16] = {
    {  1,  5,  4,  5,  6,  5,  4,  4, 7,  3,  6,  0,  7,  2,  3,  1, },
    { 15, 14, 13, 12, 11, 10,  9,  8, 7,  6,  5,  4,  3,  2,  1,  0, },
};

static const uint8_t mpa_quad_bits[2][16] = {
    { 1, 4, 4, 5, 4, 6, 5, 6, 4, 5, 5, 6, 5, 6, 6, 6, },
    { 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, },
};

/* band size tables */
const uint8_t band_size_long[9][22] = {
{ 4, 4, 4, 4, 4, 4, 6, 6, 8, 8, 10,
  12, 16, 20, 24, 28, 34, 42, 50, 54, 76, 158, }, /* 44100 */
{ 4, 4, 4, 4, 4, 4, 6, 6, 6, 8, 10,
  12, 16, 18, 22, 28, 34, 40, 46, 54, 54, 192, }, /* 48000 */
{ 4, 4, 4, 4, 4, 4, 6, 6, 8, 10, 12,
  16, 20, 24, 30, 38, 46, 56, 68, 84, 102, 26, }, /* 32000 */
{ 6, 6, 6, 6, 6, 6, 8, 10, 12, 14, 16,
  20, 24, 28, 32, 38, 46, 52, 60, 68, 58, 54, }, /* 22050 */
{ 6, 6, 6, 6, 6, 6, 8, 10, 12, 14, 16,
  18, 22, 26, 32, 38, 46, 52, 64, 70, 76, 36, }, /* 24000 */
{ 6, 6, 6, 6, 6, 6, 8, 10, 12, 14, 16,
  20, 24, 28, 32, 38, 46, 52, 60, 68, 58, 54, }, /* 16000 */
{ 6, 6, 6, 6, 6, 6, 8, 10, 12, 14, 16,
  20, 24, 28, 32, 38, 46, 52, 60, 68, 58, 54, }, /* 11025 */
{ 6, 6, 6, 6, 6, 6, 8, 10, 12, 14, 16,
  20, 24, 28, 32, 38, 46, 52, 60, 68, 58, 54, }, /* 12000 */
{ 12, 12, 12, 12, 12, 12, 16, 20, 24, 28, 32,
  40, 48, 56, 64, 76, 90, 2, 2, 2, 2, 2, }, /* 8000 */
};

const uint8_t band_size_short[9][13] = {
{ 4, 4, 4, 4, 6, 8, 10, 12, 14, 18, 22, 30, 56, }, /* 44100 */
{ 4, 4, 4, 4, 6, 6, 10, 12, 14, 16, 20, 26, 66, }, /* 48000 */
{ 4, 4, 4, 4, 6, 8, 12, 16, 20, 26, 34, 42, 12, }, /* 32000 */
{ 4, 4, 4, 6, 6, 8, 10, 14, 18, 26, 32, 42, 18, }, /* 22050 */
{ 4, 4, 4, 6, 8, 10, 12, 14, 18, 24, 32, 44, 12, }, /* 24000 */
{ 4, 4, 4, 6, 8, 10, 12, 14, 18, 24, 30, 40, 18, }, /* 16000 */
{ 4, 4, 4, 6, 8, 10, 12, 14, 18, 24, 30, 40, 18, }, /* 11025 */
{ 4, 4, 4, 6, 8, 10, 12, 14, 18, 24, 30, 40, 18, }, /* 12000 */
{ 8, 8, 8, 12, 16, 20, 24, 28, 36, 2, 2, 2, 26, }, /* 8000 */
};

const uint8_t mpa_pretab[2][22] = {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, 3, 3, 2, 0 },
};

/* table for alias reduction (XXX: store it as integer !) */
const float ci_table[8] = {
    -0.6, -0.535, -0.33, -0.185, -0.095, -0.041, -0.0142, -0.0037,
};
