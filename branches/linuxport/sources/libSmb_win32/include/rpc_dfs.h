/*
 * Unix SMB/CIFS implementation.
 * header auto-generated by pidl. DO NOT MODIFY!
 */


#ifndef _RPC_NETDFS_H
#define _RPC_NETDFS_H

#define DFS_GETMANAGERVERSION 0
#define DFS_ADD 1
#define DFS_REMOVE 2
#define DFS_SETINFO 3
#define DFS_GETINFO 4
#define DFS_ENUM 5
#define DFS_RENAME 6
#define DFS_MOVE 7
#define DFS_MANAGERGETCONFIGINFO 8
#define DFS_MANAGERSENDSITEINFO 9
#define DFS_ADDFTROOT 10
#define DFS_REMOVEFTROOT 11
#define DFS_ADDSTDROOT 12
#define DFS_REMOVESTDROOT 13
#define DFS_MANAGERINITIALIZE 14
#define DFS_ADDSTDROOTFORCED 15
#define DFS_GETDCADDRESS 16
#define DFS_SETDCADDRESS 17
#define DFS_FLUSHFTTABLE 18
#define DFS_ADD2 19
#define DFS_REMOVE2 20
#define DFS_ENUMEX 21
#define DFS_SETINFO2 22

typedef struct netdfs_dfs_Info0 {
	uint32 dummy;
} NETDFS_DFS_INFO0;

typedef struct netdfs_dfs_Info1 {
	uint32 ptr0_path;
	UNISTR2 path;
} NETDFS_DFS_INFO1;

typedef struct netdfs_dfs_Info2 {
	uint32 ptr0_path;
	UNISTR2 path;
	uint32 ptr0_comment;
	UNISTR2 comment;
	uint32 state;
	uint32 num_stores;
} NETDFS_DFS_INFO2;

typedef struct netdfs_dfs_StorageInfo {
	uint32 state;
	uint32 ptr0_server;
	UNISTR2 server;
	uint32 ptr0_share;
	UNISTR2 share;
} NETDFS_DFS_STORAGEINFO;

typedef struct netdfs_dfs_Info3 {
	uint32 ptr0_path;
	UNISTR2 path;
	uint32 ptr0_comment;
	UNISTR2 comment;
	uint32 state;
	uint32 num_stores;
	uint32 ptr0_stores;
	uint32 size_stores;
	NETDFS_DFS_STORAGEINFO *stores;
} NETDFS_DFS_INFO3;

typedef struct netdfs_dfs_Info4 {
	uint32 ptr0_path;
	UNISTR2 path;
	uint32 ptr0_comment;
	UNISTR2 comment;
	uint32 state;
	uint32 timeout;
	struct uuid guid;
	uint32 num_stores;
	uint32 ptr0_stores;
	uint32 size_stores;
	NETDFS_DFS_STORAGEINFO *stores;
} NETDFS_DFS_INFO4;

typedef struct netdfs_dfs_Info100 {
	uint32 ptr0_comment;
	UNISTR2 comment;
} NETDFS_DFS_INFO100;

typedef struct netdfs_dfs_Info101 {
	uint32 state;
} NETDFS_DFS_INFO101;

typedef struct netdfs_dfs_Info102 {
	uint32 timeout;
} NETDFS_DFS_INFO102;

typedef struct netdfs_dfs_Info200 {
	uint32 ptr0_dom_root;
	UNISTR2 dom_root;
} NETDFS_DFS_INFO200;

typedef struct netdfs_dfs_Info300 {
	uint32 flags;
	uint32 ptr0_dom_root;
	UNISTR2 dom_root;
} NETDFS_DFS_INFO300;

typedef struct netdfs_dfs_Info_ctr {
	uint32 switch_value;
	uint32 ptr0;
	union netdfs_dfs_Info {
			NETDFS_DFS_INFO0 info0;
			NETDFS_DFS_INFO1 info1;
			NETDFS_DFS_INFO2 info2;
			NETDFS_DFS_INFO3 info3;
			NETDFS_DFS_INFO4 info4;
			NETDFS_DFS_INFO100 info100;
			NETDFS_DFS_INFO101 info101;
			NETDFS_DFS_INFO102 info102;
	} u;
} NETDFS_DFS_INFO_CTR;

typedef struct netdfs_dfs_EnumArray1 {
	uint32 count;
	uint32 ptr0_s;
	uint32 size_s;
	NETDFS_DFS_INFO1 *s;
} NETDFS_DFS_ENUMARRAY1;

typedef struct netdfs_dfs_EnumArray2 {
	uint32 count;
	uint32 ptr0_s;
	uint32 size_s;
	NETDFS_DFS_INFO2 *s;
} NETDFS_DFS_ENUMARRAY2;

typedef struct netdfs_dfs_EnumArray3 {
	uint32 count;
	uint32 ptr0_s;
	uint32 size_s;
	NETDFS_DFS_INFO3 *s;
} NETDFS_DFS_ENUMARRAY3;

typedef struct netdfs_dfs_EnumArray4 {
	uint32 count;
	uint32 ptr0_s;
	uint32 size_s;
	NETDFS_DFS_INFO4 *s;
} NETDFS_DFS_ENUMARRAY4;

typedef struct netdfs_dfs_EnumArray200 {
	uint32 count;
	uint32 ptr0_s;
	uint32 size_s;
	NETDFS_DFS_INFO200 *s;
} NETDFS_DFS_ENUMARRAY200;

typedef struct netdfs_dfs_EnumArray300 {
	uint32 count;
	uint32 ptr0_s;
	uint32 size_s;
	NETDFS_DFS_INFO300 *s;
} NETDFS_DFS_ENUMARRAY300;

typedef struct netdfs_dfs_EnumInfo_ctr {
	uint32 switch_value;
	uint32 ptr0;
	union netdfs_dfs_EnumInfo {
			NETDFS_DFS_ENUMARRAY1 info1;
			NETDFS_DFS_ENUMARRAY2 info2;
			NETDFS_DFS_ENUMARRAY3 info3;
			NETDFS_DFS_ENUMARRAY4 info4;
			NETDFS_DFS_ENUMARRAY200 info200;
			NETDFS_DFS_ENUMARRAY300 info300;
	} u;
} NETDFS_DFS_ENUMINFO_CTR;

typedef struct netdfs_dfs_EnumStruct {
	uint32 level;
	NETDFS_DFS_ENUMINFO_CTR e;
} NETDFS_DFS_ENUMSTRUCT;

typedef struct netdfs_q_dfs_GetManagerVersion {
	uint32 dummy;
} NETDFS_Q_DFS_GETMANAGERVERSION;

typedef struct netdfs_r_dfs_GetManagerVersion {
	uint32 exist_flag;
} NETDFS_R_DFS_GETMANAGERVERSION;

typedef struct netdfs_q_dfs_Add {
	UNISTR2 path;
	UNISTR2 server;
	uint32 ptr0_share;
	UNISTR2 share;
	uint32 ptr0_comment;
	UNISTR2 comment;
	uint32 flags;
} NETDFS_Q_DFS_ADD;

typedef struct netdfs_r_dfs_Add {
	WERROR status;
} NETDFS_R_DFS_ADD;

typedef struct netdfs_q_dfs_Remove {
	UNISTR2 path;
	uint32 ptr0_server;
	UNISTR2 server;
	uint32 ptr0_share;
	UNISTR2 share;
} NETDFS_Q_DFS_REMOVE;

typedef struct netdfs_r_dfs_Remove {
	WERROR status;
} NETDFS_R_DFS_REMOVE;

typedef struct netdfs_q_dfs_SetInfo {
	uint32 dummy;
} NETDFS_Q_DFS_SETINFO;

typedef struct netdfs_r_dfs_SetInfo {
	WERROR status;
} NETDFS_R_DFS_SETINFO;

typedef struct netdfs_q_dfs_GetInfo {
	UNISTR2 path;
	uint32 ptr0_server;
	UNISTR2 server;
	uint32 ptr0_share;
	UNISTR2 share;
	uint32 level;
} NETDFS_Q_DFS_GETINFO;

typedef struct netdfs_r_dfs_GetInfo {
	NETDFS_DFS_INFO_CTR info;
	WERROR status;
} NETDFS_R_DFS_GETINFO;

typedef struct netdfs_q_dfs_Enum {
	uint32 level;
	uint32 bufsize;
	uint32 ptr0_info;
	NETDFS_DFS_ENUMSTRUCT info;
	uint32 ptr0_total;
	uint32 total;
} NETDFS_Q_DFS_ENUM;

typedef struct netdfs_r_dfs_Enum {
	uint32 ptr0_info;
	NETDFS_DFS_ENUMSTRUCT info;
	uint32 ptr0_total;
	uint32 total;
	WERROR status;
} NETDFS_R_DFS_ENUM;

typedef struct netdfs_q_dfs_Rename {
	uint32 dummy;
} NETDFS_Q_DFS_RENAME;

typedef struct netdfs_r_dfs_Rename {
	WERROR status;
} NETDFS_R_DFS_RENAME;

typedef struct netdfs_q_dfs_Move {
	uint32 dummy;
} NETDFS_Q_DFS_MOVE;

typedef struct netdfs_r_dfs_Move {
	WERROR status;
} NETDFS_R_DFS_MOVE;

typedef struct netdfs_q_dfs_ManagerGetConfigInfo {
	uint32 dummy;
} NETDFS_Q_DFS_MANAGERGETCONFIGINFO;

typedef struct netdfs_r_dfs_ManagerGetConfigInfo {
	WERROR status;
} NETDFS_R_DFS_MANAGERGETCONFIGINFO;

typedef struct netdfs_q_dfs_ManagerSendSiteInfo {
	uint32 dummy;
} NETDFS_Q_DFS_MANAGERSENDSITEINFO;

typedef struct netdfs_r_dfs_ManagerSendSiteInfo {
	WERROR status;
} NETDFS_R_DFS_MANAGERSENDSITEINFO;

typedef struct netdfs_q_dfs_AddFtRoot {
	uint32 dummy;
} NETDFS_Q_DFS_ADDFTROOT;

typedef struct netdfs_r_dfs_AddFtRoot {
	WERROR status;
} NETDFS_R_DFS_ADDFTROOT;

typedef struct netdfs_q_dfs_RemoveFtRoot {
	uint32 dummy;
} NETDFS_Q_DFS_REMOVEFTROOT;

typedef struct netdfs_r_dfs_RemoveFtRoot {
	WERROR status;
} NETDFS_R_DFS_REMOVEFTROOT;

typedef struct netdfs_q_dfs_AddStdRoot {
	uint32 dummy;
} NETDFS_Q_DFS_ADDSTDROOT;

typedef struct netdfs_r_dfs_AddStdRoot {
	WERROR status;
} NETDFS_R_DFS_ADDSTDROOT;

typedef struct netdfs_q_dfs_RemoveStdRoot {
	uint32 dummy;
} NETDFS_Q_DFS_REMOVESTDROOT;

typedef struct netdfs_r_dfs_RemoveStdRoot {
	WERROR status;
} NETDFS_R_DFS_REMOVESTDROOT;

typedef struct netdfs_q_dfs_ManagerInitialize {
	uint32 dummy;
} NETDFS_Q_DFS_MANAGERINITIALIZE;

typedef struct netdfs_r_dfs_ManagerInitialize {
	WERROR status;
} NETDFS_R_DFS_MANAGERINITIALIZE;

typedef struct netdfs_q_dfs_AddStdRootForced {
	uint32 dummy;
} NETDFS_Q_DFS_ADDSTDROOTFORCED;

typedef struct netdfs_r_dfs_AddStdRootForced {
	WERROR status;
} NETDFS_R_DFS_ADDSTDROOTFORCED;

typedef struct netdfs_q_dfs_GetDcAddress {
	uint32 dummy;
} NETDFS_Q_DFS_GETDCADDRESS;

typedef struct netdfs_r_dfs_GetDcAddress {
	WERROR status;
} NETDFS_R_DFS_GETDCADDRESS;

typedef struct netdfs_q_dfs_SetDcAddress {
	uint32 dummy;
} NETDFS_Q_DFS_SETDCADDRESS;

typedef struct netdfs_r_dfs_SetDcAddress {
	WERROR status;
} NETDFS_R_DFS_SETDCADDRESS;

typedef struct netdfs_q_dfs_FlushFtTable {
	uint32 dummy;
} NETDFS_Q_DFS_FLUSHFTTABLE;

typedef struct netdfs_r_dfs_FlushFtTable {
	WERROR status;
} NETDFS_R_DFS_FLUSHFTTABLE;

typedef struct netdfs_q_dfs_Add2 {
	uint32 dummy;
} NETDFS_Q_DFS_ADD2;

typedef struct netdfs_r_dfs_Add2 {
	WERROR status;
} NETDFS_R_DFS_ADD2;

typedef struct netdfs_q_dfs_Remove2 {
	uint32 dummy;
} NETDFS_Q_DFS_REMOVE2;

typedef struct netdfs_r_dfs_Remove2 {
	WERROR status;
} NETDFS_R_DFS_REMOVE2;

typedef struct netdfs_q_dfs_EnumEx {
	uint32 dummy;
} NETDFS_Q_DFS_ENUMEX;

typedef struct netdfs_r_dfs_EnumEx {
	WERROR status;
} NETDFS_R_DFS_ENUMEX;

typedef struct netdfs_q_dfs_SetInfo2 {
	uint32 dummy;
} NETDFS_Q_DFS_SETINFO2;

typedef struct netdfs_r_dfs_SetInfo2 {
	WERROR status;
} NETDFS_R_DFS_SETINFO2;

#endif /* _RPC_NETDFS_H */
