#include "stdafx.h"
#include <sys/types.h>
#include <utime.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>

#include "../../../Util.h"

extern "C"
{

static char xbp_cw_dir[MAX_PATH] = "Q:\\python";

char* xbp_getcwd(char *buf, int size)
{
	if (buf == NULL) buf = (char *)malloc(size);
	strcpy(buf, xbp_cw_dir);
	return buf;
}

int xbp_chdir(const char *dirname)
{
  if (strlen(dirname) > MAX_PATH) return -1;

  strcpy(xbp_cw_dir, dirname);

  return 0;
}

int xbp_unlink(const char *filename)
{
  CStdString strName = _P(filename);
  return unlink(strName.c_str());
}

int xbp_access(const char *path, int mode)
{
  CStdString strName = _P(path);
  return access(strName.c_str(), mode);
}

int xbp_chmod(const char *filename, int pmode)
{
  CStdString strName = _P(filename);
  return chmod(strName.c_str(), pmode);
}

int xbp_rmdir(const char *dirname)
{
  CStdString strName = _P(dirname);
  return rmdir(strName.c_str());
}

int xbp_utime(const char *filename, struct utimbuf *times)
{
  CStdString strName = _P(filename);
  return utime(strName.c_str(), times);
}

int xbp_rename(const char *oldname, const char *newname)
{
  CStdString strOldName = _P(oldname);
  CStdString strNewName = _P(newname);
  return rename(strOldName.c_str(), strNewName.c_str());
}

int xbp_mkdir(const char *dirname)
{
  CStdString strName = _P(dirname);

  // If the dir already exists, don't try to recreate it
  struct stat buf;
  if (stat(strName.c_str(), &buf) == 0 && S_ISDIR(buf.st_mode))
    return 0;
    
  return mkdir(strName.c_str(), 0755);
}

int xbp_open(const char *filename, int oflag, int pmode)
{
  CStdString strName = _P(filename);
  return open(strName.c_str(), oflag, pmode);
}

FILE* xbp_fopen(const char *filename, const char *mode)
{
  CStdString strName = _P(filename);
  return fopen(strName.c_str(), mode);
}

FILE* xbp_freopen(const char *path, const char *mode, FILE *stream)
{
  CStdString strName = _P(path);
  return freopen(strName.c_str(), mode, stream);
}

FILE* xbp_fopen64(const char *filename, const char *mode)
{
  CStdString strName = _P(filename);
  return fopen64(strName.c_str(), mode);
}

DIR *xbp_opendir(const char *name)
{
  CStdString strName = _P(name);
  return opendir(strName.c_str());
}

} // extern "C"
