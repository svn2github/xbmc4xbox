#include "LinuxFileSystem.h"
#include "RegExp.h"

using namespace std;

// Example of what pmount returns
// /dev/sdc1 on /media/YUVAL'S DOK type vfat (rw,nosuid,nodev,shortname=mixed,uid=1000,utf8,umask=077,usefree)

vector<CStdString> CLinuxFileSystem::GetRemoveableDrives()
{
   vector<CStdString> result;
   
   CRegExp reMount;
#ifdef __APPLE__
   reMount.RegComp("on (.+) \\(([^,]+)");
#else
   reMount.RegComp("on (.+) type ([^ ]+)");
#endif
   
   char line[1024];
   
#ifdef __APPLE__
   FILE* pipe = popen("mount", "r");
#else
   FILE* pipe = popen("pmount", "r");
#endif
   if (pipe)
   {
      while (fgets(line, sizeof(line) - 1, pipe))
      {
         if (reMount.RegFind(line) != -1)
         {
            char* mount = reMount.GetReplaceString("\\1");
            char* fs = reMount.GetReplaceString("\\2");

            // ignore iso9660 as we support it natively in XBMC, although 
            // this is a better way 
            if (strcmp(fs, "iso9660") == 0)
               continue;
            
#ifdef __APPLE__
            // Ignore the stuff that doesn't make sense.
            if (strcmp(fs, "devfs") == 0 || strcmp(fs, "fdesc") == 0 || strcmp(fs, "autofs") == 0)
              continue;
            
            // Skip this for now, until we can figure out the name of the root volume.
            if (strcmp(mount, "/") == 0)
              continue;
#endif
            
            result.push_back(mount);

            free(fs);
            free(mount);
         }
      }
       
      pclose(pipe);
   }   
   
   return result;
}

vector<CStdString> CLinuxFileSystem::GetAllMountedDrives()
{
  vector<CStdString> result;
  CRegExp reMount;
#ifdef __APPLE__
  reMount.RegComp("on (.+) \\(([^,]+)");
#else
  reMount.RegComp("on (.+) type ([^ ]+)");
#endif
  char line[1024];

#ifdef __APPLE__
  FILE* pipe = popen("mount", "r");
#else
  FILE* pipe = popen("mount", "r");
#endif

  if (pipe)
  {
    while (fgets(line, sizeof(line) - 1, pipe))
    {
      if (reMount.RegFind(line) != -1)
      {
        char* mount = reMount.GetReplaceString("\\1");
        char* fs    = reMount.GetReplaceString("\\2");

#ifdef __APPLE__
        // Ignore the stuff that doesn't make sense.
        if (strcmp(fs, "devfs") == 0 || strcmp(fs, "fdesc") == 0 || strcmp(fs, "autofs") == 0)
          continue;
            
        // Skip this for now, until we can figure out the name of the root volume.
        if (strcmp(mount, "/") == 0)
          continue;

        result.push_back(mount);
#else
        // Ignore root
        if (strcmp(mount, "/") == 0)
          continue;
        // Here we choose wich filesystems are approved
        if (strcmp(fs, "fuseblk") == 0 || strcmp(fs, "vfat") == 0 || strcmp(fs, "ext2") == 0 || strcmp(fs, "ext3") == 0 || strcmp(fs, "reiserfs") == 0 || strcmp(fs, "xfs") == 0 || strcmp(fs, "ntfs-3g") == 0)
          result.push_back(mount);
#endif

        free(fs);
        free(mount);
      }
    }
    pclose(pipe);
  }
  return result;
}
