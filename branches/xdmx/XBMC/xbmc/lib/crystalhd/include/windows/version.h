/********************************************************************
* Copyright(c) 2006 Broadcom Corporation, all rights reserved.
* Contains proprietary and confidential information.
*
* This source file is the property of Broadcom Corporation, and
* may not be copied or distributed in any isomorphic form without
* the prior written consent of Broadcom Corporation.
*
*
*  Name: version.h
*
*  Description: Version numbering for the driver use.
*
*  AU
*
*  HISTORY:
*
*******************************************************************/

#ifndef _BC_DTS_VERSION_
#define _BC_DTS_VERSION_
//
// The version format that we are adopting is 
// MajorVersion.MinorVersion.Revision
// This will be the same for all the components.
//
//
#define STRINGIFY_VERSION(MAJ,MIN,REV) STRINGIFIED_VERSION(MAJ,MIN,REV)
#define STRINGIFIED_VERSION(MAJ,MIN,REV) #MAJ "." #MIN "." #REV

#define STRINGIFY_VERSION_W(MAJ,MIN,REV) STRINGIFIED_VERSION_W(MAJ,MIN,REV)
#define STRINGIFIED_VERSION_W(MAJ,MIN,REV) #MAJ "." #MIN "." #REV

//
//  Product Version number is:
//  x.y.z.a
//
//  x = Major release.      1 = Dozer, 2 = Dozer + Link
//  y = Minor release.      Should increase +1 per "real" release.
//  z = Branch release.     0 for main branch.  This is +1 per branch release.
//  a = Build number        +1 per candidate release.  Reset to 0 every "real" release.
//
//
// Enabling Check-In rules enforcement 08092007
//
#define INVALID_VERSION             0xFFFF

/*========================== Common For All Components =================================*/
#define RC_COMPANY_NAME             "Broadcom Corporation\0"
#define RC_PRODUCT_VERSION          "2.7.0.4601"
#define RC_W_PRODUCT_VERSION        L"2.7.0.4601"
#define RC_PRODUCT_NAME             "Broadcom CrystalHD Decoder\0"
#define RC_COMMENTS                 "Broadcom BCM700XX Controller\0"
#define RC_COPYRIGHT                "Copyright(c) 2007 Broadcom Corporation"
#define RC_PRIVATE_BUILD            "Broadcom Corp. Private\0"
#define RC_LEGAL_TRADEMARKS         " \0"
#define BRCM_MAJOR_VERSION          2


/*========================== WDM Driver =================================*/

/* 
 * Version number scheme for driver DVMJVer.DVMNVer.DVRev.UNMODIFIED 
 * Where DVMJVer = DRIVER_MAJOR_VERSION
 * DVMNVer = DRIVER_MINOR_VERSION
 * DVRev = DRIVER_REVISION
 * UNMODIFIED = This is for Compatibility with windows INF file version scheme.
 */

#define RC_FILE_DESCRIPTION         "Broadcom CrystalHD Decoder Driver\0"
#define RC_INTERNAL_NAME            ""
#define RC_ORIGINAL_NAME            RC_INTERNAL_NAME
#define RC_SPECIAL_BUILD            ""

#define DRIVER_MAJOR_VERSION        BRCM_MAJOR_VERSION
#define DRIVER_MINOR_VERSION        36
#define DRIVER_REVISION             0

#define RC_FILE_VERSION             STRINGIFY_VERSION(DRIVER_MAJOR_VERSION,DRIVER_MINOR_VERSION,DRIVER_REVISION) ".0"

/*======================= Device Interface Library ========================*/
#define DIL_MAJOR_VERSION           BRCM_MAJOR_VERSION
#define DIL_MINOR_VERSION           37
#define DIL_REVISION                0

#define DIL_RC_FILE_VERSION         STRINGIFY_VERSION(DIL_MAJOR_VERSION,DIL_MINOR_VERSION,DIL_REVISION)

/*========================== Direct Show Filter ==============================*/
#define DFILTER_MAJOR_VERSION       BRCM_MAJOR_VERSION
#define DFILTER_MINOR_VERSION       27
#define DFILTER_REVISION            0

#define DFILTER_RC_FILE_VERSION     STRINGIFY_VERSION(DFILTER_MAJOR_VERSION,DFILTER_MINOR_VERSION,DFILTER_REVISION)

/*========================== Direct Show NS Filter ==============================*/
#define NS_DFILTER_MAJOR_VERSION    BRCM_MAJOR_VERSION
#define NS_DFILTER_MINOR_VERSION    57
#define NS_DFILTER_REVISION         0

#define NS_DFILTER_RC_FILE_VERSION  STRINGIFY_VERSION(NS_DFILTER_MAJOR_VERSION,NS_DFILTER_MINOR_VERSION,NS_DFILTER_REVISION)

/*========================== deconf utility ==============================*/
#define DECONF_MAJOR_VERSION        BRCM_MAJOR_VERSION
#define DECONF_MINOR_VERSION        11
#define DECONF_REVISION             0

/*========================== MP-4 Demux Filter ==============================*/
#define MP4DEMUX_MAJOR_VERSION      BRCM_MAJOR_VERSION
#define MP4DEMUX_MINOR_VERSION      7
#define MP4DEMUX_REVISION           0

#define MP4DEMUX_RC_FILE_VERSION    STRINGIFY_VERSION(MP4DEMUX_MAJOR_VERSION,MP4DEMUX_MINOR_VERSION,MP4DEMUX_REVISION)

/*========================== MFT Decoder ==============================*/
#define MFTDECODER_MAJOR_VERSION    BRCM_MAJOR_VERSION
#define MFTDECODER_MINOR_VERSION    5
#define MFTDECODER_REVISION         0

#define MFTDECODER_RC_FILE_VERSION  STRINGIFY_VERSION(MFTDECODER_MAJOR_VERSION,MFTDECODER_MINOR_VERSION,MFTDECODER_REVISION)

/*======================= deconft utility ========================*/
#define DECONFT_MAJOR_VERSION       BRCM_MAJOR_VERSION
#define DECONFT_MINOR_VERSION       9
#define DECONFT_REVISION            0

#define DECONFT_RC_FILE_VERSION     STRINGIFY_VERSION(DECONFT_MAJOR_VERSION,DECONFT_MINOR_VERSION,DECONFT_REVISION)

/*======================= Gensig utility ========================*/
#define GENSIG_MAJOR_VERSION        BRCM_MAJOR_VERSION
#define GENSIG_MINOR_VERSION        8
#define GENSIG_REVISION             0

#define GENSIG_RC_FILE_VERSION      STRINGIFY_VERSION(GENSIG_MAJOR_VERSION,GENSIG_MINOR_VERSION,GENSIG_REVISION)

/*======================= FpgaProg utility ========================*/
#define FPGAPROG_MAJOR_VERSION      BRCM_MAJOR_VERSION
#define FPGAPROG_MINOR_VERSION      2
#define FPGAPROG_REVISION           0

#define FPGAPROG_RC_FILE_VERSION    STRINGIFY_VERSION(FPGAPROG_MAJOR_VERSION,FPGAPROG_MINOR_VERSION,FPGAPROG_REVISION)

/*======================= MPCGensig DataBase Library ========================*/
#define MPCGSDB_MAJOR_VERSION       BRCM_MAJOR_VERSION
#define MPCGSDB_MINOR_VERSION       3
#define MPCGSDB_REVISION            0

#define MPCGSDB_RC_FILE_VERSION     STRINGIFY_VERSION(MPCGSDB_MAJOR_VERSION,MPCGSDB_MINOR_VERSION,MPCGSDB_REVISION)

/*======================= MpcDosDiag utility ========================*/
#define DOSDIAG_MAJOR_VERSION       BRCM_MAJOR_VERSION
#define DOSDIAG_MINOR_VERSION       9
#define DOSDIAG_REVISION            0

#define DOSDIAG_RC_FILE_VERSION     STRINGIFY_VERSION(DOSDIAG_MAJOR_VERSION,DOSDIAG_MINOR_VERSION,DOSDIAG_REVISION)

/*======================= MpcWinDiag utility ========================*/
#define WINDIAG_MAJOR_VERSION       BRCM_MAJOR_VERSION
#define WINDIAG_MINOR_VERSION       12
#define WINDIAG_REVISION            0

#define WINDIAG_RC_FILE_VERSION     STRINGIFY_VERSION(WINDIAG_MAJOR_VERSION,WINDIAG_MINOR_VERSION,WINDIAG_REVISION)

/*======================= MpcWinInfo utility ========================*/
#define WININFO_MAJOR_VERSION       BRCM_MAJOR_VERSION
#define WININFO_MINOR_VERSION       1
#define WININFO_REVISION            0

#define WININFO_RC_FILE_VERSION     STRINGIFY_VERSION(WININFO_MAJOR_VERSION,WININFO_MINOR_VERSION,WININFO_REVISION)

/*========================== WDM Encode Driver =================================*/
#define	ENCODE_WDM_COMMENTS			"Broadcom Corp. BCM70013 Encoder\0"
#define ENCODE_WDM_PRODUCT_NAME		"Broadcom Corp. MediaPC HD Video Encoder\0"
#define ENCODE_WDM_PRIVATE_BUILD	"Broadcom Corp. Private\0"

#define ENCODE_WDM_FILE_DESC		"Broadcom Corp BCM70013 WDM Driver\0"
#define ENCODE_WDM_INTRL_NAME		"Link B0"
#define ENCODE_WDM_ORIG_NAME		ENCODE_WDM_INTRL_NAME
#define ENCODE_WDM_SPECIAL_BUILD	""

#define ENCODE_WDM_MAJOR_VERSION	BRCM_MAJOR_VERSION
#define ENCODE_WDM_MINOR_VERSION    0
#define ENCODE_WDM_REVISION         5

#define ENCODE_WDM_FILE_VERSION     STRINGIFY_VERSION(ENCODE_WDM_MAJOR_VERSION,ENCODE_WDM_MINOR_VERSION,ENCODE_WDM_REVISION) ".0"

/*========================== CmdUtil Application ==============================*/
#define CMDUTIL_MAJOR_VERSION        BRCM_MAJOR_VERSION
#define CMDUTIL_MINOR_VERSION        0
#define CMDUTIL_REVISION             2

/*========================== CmdUtilt utility ==============================*/
#define CMDUTILT_MAJOR_VERSION        BRCM_MAJOR_VERSION
#define CMDUTILT_MINOR_VERSION        0
#define CMDUTILT_REVISION			  2

#endif
