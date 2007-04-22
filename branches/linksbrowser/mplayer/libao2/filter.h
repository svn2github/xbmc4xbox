/*=============================================================================
//	
//  This software has been released under the terms of the GNU Public
//  license. See http://www.gnu.org/copyleft/gpl.html for details.
//
//  Copyright 2001 Anders Johansson ajh@atri.curtin.edu.au
//
//=============================================================================
*/

/* Polyphase filters. Designed using window method and kaiser window
   with beta=10. */

#define W4 { \
0, 36, 32767, -35, 0, 113, 32756, -103, \
0, 193, 32733, -167, 0, 278, 32699, -227, \
0, 367, 32654, -282, -1, 461, 32597, -335, \
-1, 560, 32530, -383, -1, 664, 32451, -429, \
-1, 772, 32361, -470, -2, 886, 32260, -509, \
-2, 1004, 32148, -544, -2, 1128, 32026, -577, \
-3, 1257, 31892, -606, -4, 1392, 31749, -633, \
-4, 1532, 31594, -657, -5, 1677, 31430, -678, \
-6, 1828, 31255, -697, -7, 1985, 31070, -713, \
-8, 2147, 30876, -727, -9, 2315, 30671, -739, \
-10, 2489, 30457, -749, -11, 2668, 30234, -756, \
-13, 2854, 30002, -762, -14, 3045, 29761, -766, \
-16, 3243, 29511, -769, -18, 3446, 29252, -769, \
-20, 3656, 28985, -769, -22, 3871, 28710, -766, \
-25, 4093, 28427, -763, -27, 4320, 28137, -758, \
-30, 4553, 27839, -752, -33, 4793, 27534, -745, \
-36, 5038, 27222, -737, -39, 5290, 26904, -727, \
-43, 5547, 26579, -717, -47, 5811, 26248, -707, \
-51, 6080, 25911, -695, -55, 6355, 25569, -683, \
-60, 6636, 25221, -670, -65, 6922, 24868, -657, \
-70, 7214, 24511, -643, -76, 7512, 24149, -628, \
-81, 7815, 23782, -614, -87, 8124, 23412, -599, \
-94, 8437, 23038, -583, -101, 8757, 22661, -568, \
-108, 9081, 22281, -552, -115, 9410, 21898, -537, \
-123, 9744, 21513, -521, -131, 10082, 21125, -505, \
-139, 10425, 20735, -489, -148, 10773, 20344, -473, \
-157, 11125, 19951, -457, -166, 11481, 19557, -441, \
-176, 11841, 19162, -426, -186, 12205, 18767, -410, \
-197, 12572, 18372, -395, -208, 12942, 17976, -380, \
-219, 13316, 17581, -365, -231, 13693, 17186, -350, \
-243, 14073, 16791, -336, -255, 14455, 16398, -321, \
-268, 14840, 16006, -307, -281, 15227, 15616, -294, \
-294, 15616, 15227, -281, -307, 16006, 14840, -268, \
-321, 16398, 14455, -255, -336, 16791, 14073, -243, \
-350, 17186, 13693, -231, -365, 17581, 13316, -219, \
-380, 17976, 12942, -208, -395, 18372, 12572, -197, \
-410, 18767, 12205, -186, -426, 19162, 11841, -176, \
-441, 19557, 11481, -166, -457, 19951, 11125, -157, \
-473, 20344, 10773, -148, -489, 20735, 10425, -139, \
-505, 21125, 10082, -131, -521, 21513, 9744, -123, \
-537, 21898, 9410, -115, -552, 22281, 9081, -108, \
-568, 22661, 8757, -101, -583, 23038, 8437, -94, \
-599, 23412, 8124, -87, -614, 23782, 7815, -81, \
-628, 24149, 7512, -76, -643, 24511, 7214, -70, \
-657, 24868, 6922, -65, -670, 25221, 6636, -60, \
-683, 25569, 6355, -55, -695, 25911, 6080, -51, \
-707, 26248, 5811, -47, -717, 26579, 5547, -43, \
-727, 26904, 5290, -39, -737, 27222, 5038, -36, \
-745, 27534, 4793, -33, -752, 27839, 4553, -30, \
-758, 28137, 4320, -27, -763, 28427, 4093, -25, \
-766, 28710, 3871, -22, -769, 28985, 3656, -20, \
-769, 29252, 3446, -18, -769, 29511, 3243, -16, \
-766, 29761, 3045, -14, -762, 30002, 2854, -13, \
-756, 30234, 2668, -11, -749, 30457, 2489, -10, \
-739, 30671, 2315, -9, -727, 30876, 2147, -8, \
-713, 31070, 1985, -7, -697, 31255, 1828, -6, \
-678, 31430, 1677, -5, -657, 31594, 1532, -4, \
-633, 31749, 1392, -4, -606, 31892, 1257, -3, \
-577, 32026, 1128, -2, -544, 32148, 1004, -2, \
-509, 32260, 886, -2, -470, 32361, 772, -1, \
-429, 32451, 664, -1, -383, 32530, 560, -1, \
-335, 32597, 461, -1, -282, 32654, 367, 0, \
-227, 32699, 278, 0, -167, 32733, 193, 0, \
-103, 32756, 113, 0, -35, 32767, 36, 0, \
}

#define W8 { \
0, 2, -18, 95, 32767, -94, 18, -2, \
0, 6, -55, 289, 32759, -279, 53, -5, \
0, 9, -93, 488, 32744, -458, 87, -8, \
0, 13, -132, 692, 32720, -633, 120, -11, \
0, 18, -173, 900, 32689, -804, 151, -14, \
0, 22, -214, 1113, 32651, -969, 182, -17, \
0, 27, -256, 1331, 32604, -1130, 212, -20, \
0, 31, -299, 1553, 32550, -1286, 241, -22, \
0, 36, -343, 1780, 32488, -1437, 268, -25, \
0, 41, -389, 2011, 32419, -1583, 295, -27, \
-1, 47, -435, 2247, 32342, -1724, 320, -29, \
-1, 52, -482, 2488, 32257, -1861, 345, -31, \
-1, 58, -530, 2733, 32165, -1993, 368, -32, \
-1, 64, -579, 2982, 32066, -2121, 391, -34, \
-1, 70, -629, 3236, 31959, -2243, 412, -36, \
-1, 76, -679, 3494, 31844, -2361, 433, -37, \
-1, 82, -731, 3756, 31723, -2475, 452, -38, \
-1, 89, -783, 4022, 31594, -2583, 471, -40, \
-2, 96, -837, 4293, 31458, -2688, 488, -41, \
-2, 102, -891, 4567, 31315, -2787, 505, -42, \
-2, 110, -946, 4846, 31164, -2883, 521, -42, \
-2, 117, -1001, 5128, 31007, -2973, 535, -43, \
-2, 124, -1057, 5414, 30843, -3060, 549, -44, \
-3, 132, -1114, 5704, 30672, -3142, 562, -44, \
-3, 140, -1172, 5998, 30495, -3219, 574, -45, \
-3, 148, -1230, 6295, 30311, -3292, 585, -45, \
-3, 156, -1289, 6596, 30120, -3362, 596, -46, \
-4, 164, -1349, 6900, 29923, -3426, 605, -46, \
-4, 173, -1408, 7208, 29719, -3487, 614, -46, \
-4, 181, -1469, 7519, 29509, -3544, 621, -46, \
-4, 190, -1530, 7832, 29294, -3597, 628, -46, \
-5, 199, -1591, 8149, 29072, -3645, 635, -46, \
-5, 208, -1652, 8469, 28844, -3690, 640, -46, \
-6, 217, -1714, 8792, 28610, -3731, 645, -46, \
-6, 227, -1777, 9118, 28371, -3768, 649, -45, \
-6, 236, -1839, 9446, 28126, -3801, 652, -45, \
-7, 246, -1902, 9776, 27875, -3831, 655, -45, \
-7, 255, -1964, 10109, 27620, -3857, 657, -44, \
-8, 265, -2027, 10445, 27359, -3880, 658, -44, \
-8, 275, -2090, 10782, 27092, -3899, 659, -43, \
-9, 285, -2153, 11121, 26821, -3915, 659, -43, \
-9, 295, -2216, 11463, 26546, -3927, 658, -42, \
-10, 306, -2278, 11806, 26265, -3936, 657, -42, \
-10, 316, -2341, 12151, 25980, -3942, 655, -41, \
-11, 326, -2403, 12497, 25690, -3945, 653, -40, \
-11, 337, -2465, 12845, 25396, -3945, 650, -40, \
-12, 347, -2526, 13194, 25098, -3941, 647, -39, \
-12, 358, -2588, 13544, 24796, -3935, 643, -38, \
-13, 368, -2648, 13896, 24490, -3926, 639, -38, \
-14, 379, -2708, 14248, 24181, -3914, 634, -37, \
-14, 389, -2768, 14601, 23867, -3900, 629, -36, \
-15, 400, -2827, 14954, 23551, -3883, 623, -35, \
-16, 410, -2885, 15308, 23231, -3863, 617, -34, \
-16, 421, -2943, 15662, 22908, -3841, 611, -34, \
-17, 431, -2999, 16016, 22581, -3817, 604, -33, \
-18, 442, -3055, 16371, 22252, -3790, 597, -32, \
-19, 452, -3109, 16725, 21921, -3761, 590, -31, \
-19, 462, -3163, 17079, 21587, -3730, 582, -30, \
-20, 472, -3215, 17432, 21250, -3697, 574, -29, \
-21, 483, -3266, 17785, 20911, -3662, 566, -28, \
-22, 492, -3316, 18138, 20570, -3624, 558, -28, \
-23, 502, -3365, 18489, 20227, -3585, 549, -27, \
-23, 512, -3412, 18839, 19883, -3545, 540, -26, \
-24, 522, -3458, 19188, 19536, -3502, 531, -25, \
-25, 531, -3502, 19536, 19188, -3458, 522, -24, \
-26, 540, -3545, 19883, 18839, -3412, 512, -23, \
-27, 549, -3585, 20227, 18489, -3365, 502, -23, \
-28, 558, -3624, 20570, 18138, -3316, 492, -22, \
-28, 566, -3662, 20911, 17785, -3266, 483, -21, \
-29, 574, -3697, 21250, 17432, -3215, 472, -20, \
-30, 582, -3730, 21587, 17079, -3163, 462, -19, \
-31, 590, -3761, 21921, 16725, -3109, 452, -19, \
-32, 597, -3790, 22252, 16371, -3055, 442, -18, \
-33, 604, -3817, 22581, 16016, -2999, 431, -17, \
-34, 611, -3841, 22908, 15662, -2943, 421, -16, \
-34, 617, -3863, 23231, 15308, -2885, 410, -16, \
-35, 623, -3883, 23551, 14954, -2827, 400, -15, \
-36, 629, -3900, 23867, 14601, -2768, 389, -14, \
-37, 634, -3914, 24181, 14248, -2708, 379, -14, \
-38, 639, -3926, 24490, 13896, -2648, 368, -13, \
-38, 643, -3935, 24796, 13544, -2588, 358, -12, \
-39, 647, -3941, 25098, 13194, -2526, 347, -12, \
-40, 650, -3945, 25396, 12845, -2465, 337, -11, \
-40, 653, -3945, 25690, 12497, -2403, 326, -11, \
-41, 655, -3942, 25980, 12151, -2341, 316, -10, \
-42, 657, -3936, 26265, 11806, -2278, 306, -10, \
-42, 658, -3927, 26546, 11463, -2216, 295, -9, \
-43, 659, -3915, 26821, 11121, -2153, 285, -9, \
-43, 659, -3899, 27092, 10782, -2090, 275, -8, \
-44, 658, -3880, 27359, 10445, -2027, 265, -8, \
-44, 657, -3857, 27620, 10109, -1964, 255, -7, \
-45, 655, -3831, 27875, 9776, -1902, 246, -7, \
-45, 652, -3801, 28126, 9446, -1839, 236, -6, \
-45, 649, -3768, 28371, 9118, -1777, 227, -6, \
-46, 645, -3731, 28610, 8792, -1714, 217, -6, \
-46, 640, -3690, 28844, 8469, -1652, 208, -5, \
-46, 635, -3645, 29072, 8149, -1591, 199, -5, \
-46, 628, -3597, 29294, 7832, -1530, 190, -4, \
-46, 621, -3544, 29509, 7519, -1469, 181, -4, \
-46, 614, -3487, 29719, 7208, -1408, 173, -4, \
-46, 605, -3426, 29923, 6900, -1349, 164, -4, \
-46, 596, -3362, 30120, 6596, -1289, 156, -3, \
-45, 585, -3292, 30311, 6295, -1230, 148, -3, \
-45, 574, -3219, 30495, 5998, -1172, 140, -3, \
-44, 562, -3142, 30672, 5704, -1114, 132, -3, \
-44, 549, -3060, 30843, 5414, -1057, 124, -2, \
-43, 535, -2973, 31007, 5128, -1001, 117, -2, \
-42, 521, -2883, 31164, 4846, -946, 110, -2, \
-42, 505, -2787, 31315, 4567, -891, 102, -2, \
-41, 488, -2688, 31458, 4293, -837, 96, -2, \
-40, 471, -2583, 31594, 4022, -783, 89, -1, \
-38, 452, -2475, 31723, 3756, -731, 82, -1, \
-37, 433, -2361, 31844, 3494, -679, 76, -1, \
-36, 412, -2243, 31959, 3236, -629, 70, -1, \
-34, 391, -2121, 32066, 2982, -579, 64, -1, \
-32, 368, -1993, 32165, 2733, -530, 58, -1, \
-31, 345, -1861, 32257, 2488, -482, 52, -1, \
-29, 320, -1724, 32342, 2247, -435, 47, -1, \
-27, 295, -1583, 32419, 2011, -389, 41, 0, \
-25, 268, -1437, 32488, 1780, -343, 36, 0, \
-22, 241, -1286, 32550, 1553, -299, 31, 0, \
-20, 212, -1130, 32604, 1331, -256, 27, 0, \
-17, 182, -969, 32651, 1113, -214, 22, 0, \
-14, 151, -804, 32689, 900, -173, 18, 0, \
-11, 120, -633, 32720, 692, -132, 13, 0, \
-8, 87, -458, 32744, 488, -93, 9, 0, \
-5, 53, -279, 32759, 289, -55, 6, 0, \
-2, 18, -94, 32767, 95, -18, 2, 0, \
}

#define W16 {\
0, 0, -1, 3, -9, 21, -47, 119,32764, -118, 47, -21, 9, -3, 1, 0, \
0, 0, -3, 10, -27, 65, -143, 361,32757, -352, 141, -63, 27, -10, 3, 0, \
0, 1, -5, 17, -46, 108, -240, 607,32743, -581, 233, -105, 44, -16, 4, -1, \
0, 1, -6, 23, -65, 153, -338, 857,32722, -805, 324, -146, 61, -22, 6, -1, \
0, 1, -8, 30, -84, 197, -438, 1112,32695, -1025, 413, -187, 78, -28, 8, -1, \
0, 2, -10, 37, -103, 243, -538, 1370,32661, -1241, 502, -226, 95, -34, 9, -2, \
0, 2, -12, 44, -122, 288, -639, 1632,32620, -1452, 588, -265, 111, -40, 11, -2, \
0, 3, -14, 52, -142, 334, -740, 1898,32572, -1659, 674, -304, 127, -45, 12, -2, \
0, 3, -17, 59, -162, 381, -843, 2168,32517, -1861, 758, -342, 143, -51, 14, -2, \
0, 3, -19, 66, -182, 427, -947, 2442,32455, -2058, 840, -379, 158, -56, 15, -2, \
0, 4, -21, 74, -202, 474, -1051, 2720,32387, -2251, 921, -415, 173, -61, 16, -3, \
0, 4, -23, 81, -222, 521, -1156, 3001,32312, -2440, 1000, -451, 188, -66, 18, -3, \
0, 5, -25, 89, -243, 569, -1261, 3285,32230, -2623, 1077, -486, 202, -71, 19, -3, \
0, 5, -28, 97, -263, 617, -1367, 3574,32142, -2802, 1153, -520, 216, -76, 20, -3, \
0, 5, -30, 104, -284, 665, -1474, 3865,32047, -2976, 1228, -553, 230, -81, 21, -3, \
0, 6, -32, 112, -305, 713, -1581, 4160,31945, -3145, 1300, -586, 243, -85, 22, -4, \
0, 6, -34, 120, -326, 761, -1688, 4458,31837, -3310, 1371, -617, 256, -90, 24, -4, \
0, 7, -37, 128, -347, 810, -1796, 4760,31722, -3470, 1440, -648, 269, -94, 25, -4, \
0, 7, -39, 136, -368, 858, -1903, 5064,31601, -3625, 1507, -679, 281, -98, 26, -4, \
0, 8, -42, 144, -389, 907, -2011, 5371,31474, -3775, 1573, -708, 293, -102, 27, -4, \
-1, 8, -44, 152, -410, 955, -2119, 5682,31340, -3921, 1637, -737, 305, -106, 27, -4, \
-1, 9, -47, 160, -431, 1004, -2228, 5995,31200, -4061, 1699, -764, 316, -109, 28, -4, \
-1, 9, -49, 168, -453, 1052, -2336, 6311,31053, -4197, 1759, -791, 326, -113, 29, -4, \
-1, 10, -52, 176, -474, 1101, -2444, 6629,30901, -4329, 1817, -817, 337, -116, 30, -4, \
-1, 10, -54, 185, -495, 1149, -2552, 6951,30742, -4455, 1873, -842, 347, -119, 31, -5, \
-1, 11, -57, 193, -516, 1197, -2659, 7274,30577, -4576, 1928, -866, 356, -123, 31, -5, \
-1, 12, -59, 201, -537, 1245, -2767, 7600,30406, -4693, 1980, -890, 366, -125, 32, -5, \
-1, 12, -62, 209, -558, 1293, -2874, 7928,30229, -4805, 2031, -912, 374, -128, 33, -5, \
-1, 13, -64, 217, -579, 1341, -2980, 8259,30047, -4912, 2080, -934, 383, -131, 33, -5, \
-1, 13, -67, 225, -600, 1388, -3086, 8591,29858, -5015, 2126, -955, 391, -133, 34, -5, \
-1, 14, -69, 234, -621, 1435, -3192, 8925,29664, -5113, 2171, -974, 399, -136, 34, -5, \
-1, 14, -72, 242, -642, 1482, -3297, 9261,29464, -5206, 2214, -993, 406, -138, 35, -5, \
-1, 15, -75, 250, -662, 1528, -3401, 9599,29259, -5294, 2255, -1012, 413, -140, 35, -5, \
-1, 15, -77, 258, -683, 1574, -3504, 9939,29048, -5378, 2294, -1029, 419, -142, 36, -5, \
-1, 16, -80, 266, -703, 1619, -3607, 10280,28831, -5457, 2332, -1045, 426, -144, 36, -5, \
-1, 17, -82, 274, -723, 1664, -3708, 10622,28610, -5531, 2367, -1060, 431, -146, 36, -5, \
-1, 17, -85, 282, -743, 1709, -3808, 10966,28383, -5601, 2400, -1075, 437, -147, 37, -5, \
-1, 18, -87, 290, -762, 1753, -3908, 11311,28151, -5666, 2431, -1089, 442, -149, 37, -5, \
-1, 18, -90, 297, -782, 1796, -4006, 11657,27914, -5727, 2461, -1101, 447, -150, 37, -5, \
-2, 19, -93, 305, -801, 1839, -4103, 12004,27672, -5783, 2488, -1113, 451, -151, 37, -5, \
-2, 20, -95, 313, -820, 1881, -4198, 12352,27425, -5835, 2514, -1124, 455, -152, 37, -5, \
-2, 20, -98, 320, -838, 1922, -4293, 12701,27173, -5882, 2538, -1134, 458, -153, 37, -5, \
-2, 21, -100, 328, -857, 1963, -4385, 13050,26917, -5925, 2559, -1144, 462, -154, 37, -5, \
-2, 21, -103, 335, -875, 2003, -4476, 13399,26656, -5964, 2579, -1152, 465, -155, 37, -5, \
-2, 22, -105, 342, -892, 2042, -4566, 13749,26390, -5998, 2597, -1160, 467, -155, 38, -5, \
-2, 23, -107, 349, -910, 2080, -4654, 14100,26121, -6028, 2613, -1166, 469, -156, 37, -5, \
-2, 23, -110, 356, -927, 2117, -4740, 14450,25847, -6054, 2628, -1172, 471, -156, 37, -5, \
-2, 24, -112, 363, -943, 2154, -4824, 14801,25568, -6075, 2640, -1177, 473, -156, 37, -5, \
-2, 24, -114, 370, -959, 2189, -4906, 15151,25286, -6093, 2651, -1182, 474, -156, 37, -5, \
-2, 25, -117, 377, -975, 2224, -4986, 15501,25000, -6106, 2659, -1185, 474, -156, 37, -5, \
-2, 26, -119, 383, -991, 2257, -5064, 15851,24710, -6115, 2666, -1187, 475, -156, 37, -5, \
-2, 26, -121, 389, -1005, 2290, -5140, 16200,24416, -6121, 2672, -1189, 475, -156, 37, -4, \
-3, 27, -123, 395, -1020, 2321, -5214, 16549,24118, -6122, 2675, -1190, 475, -155, 37, -4, \
-3, 27, -125, 401, -1034, 2352, -5285, 16897,23817, -6120, 2677, -1190, 474, -155, 36, -4, \
-3, 28, -128, 407, -1047, 2381, -5354, 17244,23513, -6114, 2677, -1190, 474, -154, 36, -4, \
-3, 28, -130, 413, -1060, 2409, -5421, 17590,23205, -6104, 2675, -1188, 472, -154, 36, -4, \
-3, 29, -132, 418, -1073, 2435, -5485, 17935,22895, -6090, 2672, -1186, 471, -153, 35, -4, \
-3, 29, -133, 423, -1085, 2461, -5546, 18279,22581, -6073, 2666, -1183, 469, -152, 35, -4, \
-3, 30, -135, 428, -1096, 2485, -5605, 18621,22264, -6052, 2660, -1180, 467, -151, 35, -4, \
-3, 31, -137, 433, -1107, 2508, -5660, 18962,21945, -6027, 2651, -1176, 465, -150, 34, -4, \
-3, 31, -139, 437, -1117, 2530, -5713, 19301,21622, -5999, 2641, -1171, 463, -149, 34, -4, \
-3, 31, -140, 442, -1126, 2550, -5764, 19639,21297, -5968, 2630, -1165, 460, -148, 34, -4, \
-3, 32, -142, 446, -1135, 2569, -5811, 19975,20970, -5934, 2617, -1159, 457, -146, 33, -4, \
-4, 32, -144, 450, -1144, 2586, -5855, 20309,20641, -5896, 2602, -1151, 453, -145, 33, -4, \
-4, 33, -145, 453, -1151, 2602, -5896, 20641,20309, -5855, 2586, -1144, 450, -144, 32, -4, \
-4, 33, -146, 457, -1159, 2617, -5934, 20970,19975, -5811, 2569, -1135, 446, -142, 32, -3, \
-4, 34, -148, 460, -1165, 2630, -5968, 21297,19639, -5764, 2550, -1126, 442, -140, 31, -3, \
-4, 34, -149, 463, -1171, 2641, -5999, 21622,19301, -5713, 2530, -1117, 437, -139, 31, -3, \
-4, 34, -150, 465, -1176, 2651, -6027, 21945,18962, -5660, 2508, -1107, 433, -137, 31, -3, \
-4, 35, -151, 467, -1180, 2660, -6052, 22264,18621, -5605, 2485, -1096, 428, -135, 30, -3, \
-4, 35, -152, 469, -1183, 2666, -6073, 22581,18279, -5546, 2461, -1085, 423, -133, 29, -3, \
-4, 35, -153, 471, -1186, 2672, -6090, 22895,17935, -5485, 2435, -1073, 418, -132, 29, -3, \
-4, 36, -154, 472, -1188, 2675, -6104, 23205,17590, -5421, 2409, -1060, 413, -130, 28, -3, \
-4, 36, -154, 474, -1190, 2677, -6114, 23513,17244, -5354, 2381, -1047, 407, -128, 28, -3, \
-4, 36, -155, 474, -1190, 2677, -6120, 23817,16897, -5285, 2352, -1034, 401, -125, 27, -3, \
-4, 37, -155, 475, -1190, 2675, -6122, 24118,16549, -5214, 2321, -1020, 395, -123, 27, -3, \
-4, 37, -156, 475, -1189, 2672, -6121, 24416,16200, -5140, 2290, -1005, 389, -121, 26, -2, \
-5, 37, -156, 475, -1187, 2666, -6115, 24710,15851, -5064, 2257, -991, 383, -119, 26, -2, \
-5, 37, -156, 474, -1185, 2659, -6106, 25000,15501, -4986, 2224, -975, 377, -117, 25, -2, \
-5, 37, -156, 474, -1182, 2651, -6093, 25286,15151, -4906, 2189, -959, 370, -114, 24, -2, \
-5, 37, -156, 473, -1177, 2640, -6075, 25568,14801, -4824, 2154, -943, 363, -112, 24, -2, \
-5, 37, -156, 471, -1172, 2628, -6054, 25847,14450, -4740, 2117, -927, 356, -110, 23, -2, \
-5, 37, -156, 469, -1166, 2613, -6028, 26121,14100, -4654, 2080, -910, 349, -107, 23, -2, \
-5, 38, -155, 467, -1160, 2597, -5998, 26390,13749, -4566, 2042, -892, 342, -105, 22, -2, \
-5, 37, -155, 465, -1152, 2579, -5964, 26656,13399, -4476, 2003, -875, 335, -103, 21, -2, \
-5, 37, -154, 462, -1144, 2559, -5925, 26917,13050, -4385, 1963, -857, 328, -100, 21, -2, \
-5, 37, -153, 458, -1134, 2538, -5882, 27173,12701, -4293, 1922, -838, 320, -98, 20, -2, \
-5, 37, -152, 455, -1124, 2514, -5835, 27425,12352, -4198, 1881, -820, 313, -95, 20, -2, \
-5, 37, -151, 451, -1113, 2488, -5783, 27672,12004, -4103, 1839, -801, 305, -93, 19, -2, \
-5, 37, -150, 447, -1101, 2461, -5727, 27914,11657, -4006, 1796, -782, 297, -90, 18, -1, \
-5, 37, -149, 442, -1089, 2431, -5666, 28151,11311, -3908, 1753, -762, 290, -87, 18, -1, \
-5, 37, -147, 437, -1075, 2400, -5601, 28383,10966, -3808, 1709, -743, 282, -85, 17, -1, \
-5, 36, -146, 431, -1060, 2367, -5531, 28610,10622, -3708, 1664, -723, 274, -82, 17, -1, \
-5, 36, -144, 426, -1045, 2332, -5457, 28831,10280, -3607, 1619, -703, 266, -80, 16, -1, \
-5, 36, -142, 419, -1029, 2294, -5378, 29048,9939, -3504, 1574, -683, 258, -77, 15, -1, \
-5, 35, -140, 413, -1012, 2255, -5294, 29259,9599, -3401, 1528, -662, 250, -75, 15, -1, \
-5, 35, -138, 406, -993, 2214, -5206, 29464,9261, -3297, 1482, -642, 242, -72, 14, -1, \
-5, 34, -136, 399, -974, 2171, -5113, 29664,8925, -3192, 1435, -621, 234, -69, 14, -1, \
-5, 34, -133, 391, -955, 2126, -5015, 29858,8591, -3086, 1388, -600, 225, -67, 13, -1, \
-5, 33, -131, 383, -934, 2080, -4912, 30047,8259, -2980, 1341, -579, 217, -64, 13, -1, \
-5, 33, -128, 374, -912, 2031, -4805, 30229,7928, -2874, 1293, -558, 209, -62, 12, -1, \
-5, 32, -125, 366, -890, 1980, -4693, 30406,7600, -2767, 1245, -537, 201, -59, 12, -1, \
-5, 31, -123, 356, -866, 1928, -4576, 30577,7274, -2659, 1197, -516, 193, -57, 11, -1, \
-5, 31, -119, 347, -842, 1873, -4455, 30742,6951, -2552, 1149, -495, 185, -54, 10, -1, \
-4, 30, -116, 337, -817, 1817, -4329, 30901,6629, -2444, 1101, -474, 176, -52, 10, -1, \
-4, 29, -113, 326, -791, 1759, -4197, 31053,6311, -2336, 1052, -453, 168, -49, 9, -1, \
-4, 28, -109, 316, -764, 1699, -4061, 31200,5995, -2228, 1004, -431, 160, -47, 9, -1, \
-4, 27, -106, 305, -737, 1637, -3921, 31340,5682, -2119, 955, -410, 152, -44, 8, -1, \
-4, 27, -102, 293, -708, 1573, -3775, 31474,5371, -2011, 907, -389, 144, -42, 8, 0, \
-4, 26, -98, 281, -679, 1507, -3625, 31601,5064, -1903, 858, -368, 136, -39, 7, 0, \
-4, 25, -94, 269, -648, 1440, -3470, 31722,4760, -1796, 810, -347, 128, -37, 7, 0, \
-4, 24, -90, 256, -617, 1371, -3310, 31837,4458, -1688, 761, -326, 120, -34, 6, 0, \
-4, 22, -85, 243, -586, 1300, -3145, 31945,4160, -1581, 713, -305, 112, -32, 6, 0, \
-3, 21, -81, 230, -553, 1228, -2976, 32047,3865, -1474, 665, -284, 104, -30, 5, 0, \
-3, 20, -76, 216, -520, 1153, -2802, 32142,3574, -1367, 617, -263, 97, -28, 5, 0, \
-3, 19, -71, 202, -486, 1077, -2623, 32230,3285, -1261, 569, -243, 89, -25, 5, 0, \
-3, 18, -66, 188, -451, 1000, -2440, 32312,3001, -1156, 521, -222, 81, -23, 4, 0, \
-3, 16, -61, 173, -415, 921, -2251, 32387,2720, -1051, 474, -202, 74, -21, 4, 0, \
-2, 15, -56, 158, -379, 840, -2058, 32455,2442, -947, 427, -182, 66, -19, 3, 0, \
-2, 14, -51, 143, -342, 758, -1861, 32517,2168, -843, 381, -162, 59, -17, 3, 0, \
-2, 12, -45, 127, -304, 674, -1659, 32572,1898, -740, 334, -142, 52, -14, 3, 0, \
-2, 11, -40, 111, -265, 588, -1452, 32620,1632, -639, 288, -122, 44, -12, 2, 0, \
-2, 9, -34, 95, -226, 502, -1241, 32661,1370, -538, 243, -103, 37, -10, 2, 0, \
-1, 8, -28, 78, -187, 413, -1025, 32695,1112, -438, 197, -84, 30, -8, 1, 0, \
-1, 6, -22, 61, -146, 324, -805, 32722,857, -338, 153, -65, 23, -6, 1, 0, \
-1, 4, -16, 44, -105, 233, -581, 32743,607, -240, 108, -46, 17, -5, 1, 0, \
0, 3, -10, 27, -63, 141, -352, 32757,361, -143, 65, -27, 10, -3, 0, 0, \
0, 1, -3, 9, -21, 47, -118, 32764,119, -47, 21, -9, 3, -1, 0, 0, \
}
