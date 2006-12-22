
#include "../stdafx.h"
#include "StackDirectory.h"
#include "../utils/log.h"
#include "../Util.h"

#define PRE_2_1_STACK_COMPATIBILITY

namespace DIRECTORY
{
  CStackDirectory::CStackDirectory()
  {
  }

  CStackDirectory::~CStackDirectory()
  {
  }

  bool CStackDirectory::GetDirectory(const CStdString& strPath, CFileItemList& items)
  {
    items.Clear();
    // format is:
    // stack://file1 , file2 , file3 , file4
    // filenames with commas are double escaped (ie replaced with ,,), thus the " , " separator used.
    //CStdString folder, file;
    //CUtil::Split(strPath, folder, file);
    // split files on the single comma
    CStdStringArray files;
    StringUtils::SplitString(strPath, " , ", files);
    if (files.empty())
      return false;   // error in path
    // remove "stack://" from the folder
    for (unsigned int i = 0; i < files.size(); i++)
    {
      CStdString file = files[i];
      if (i == 0)
        file = file.Mid(8);
#ifdef PRE_2_1_STACK_COMPATIBILITY
      if (i > 0 && file.Find("\\") == -1 && file.Find('/') == -1)
      {
        CStdString strPath;
        CUtil::GetDirectory(items[0]->m_strPath,strPath);
        CStdString strFile = file;
        CUtil::AddFileToFolder(strPath,strFile,file);
      }
#endif
      // replace double comma's with single ones.
      file.Replace(",,", ",");
      CFileItem *item = new CFileItem(file);
      //CUtil::AddFileToFolder(folder, file, item->m_strPath);
      item->m_strPath = file;
      item->m_bIsFolder = false;
      items.Add(item);
    }
    return true;
  }

  CStdString CStackDirectory::GetStackedTitlePath(const CStdString &strPath)
  {
    CStdString path, file, folder;
    int pos = file.Find(" , ");
    // remove "stack://" from the folder
    folder = folder.Mid(8);
    if (pos > 0)
    {
      CUtil::Split(strPath.Left(pos), folder, file);
      file.Replace(",,", ",");
      CStdString title, volume;
      CUtil::GetVolumeFromFileName(file, title, volume);
      CUtil::AddFileToFolder(folder, title, path);
    }
    return path;
  }

  CStdString CStackDirectory::GetFirstStackedFile(const CStdString &strPath)
  {
    // the stacked files are always in volume order, so just get up to the first filename
    // occurence of " , "
    CStdString path, file, folder;
    int pos = strPath.Find(" , ");
    if (pos > 0)
      CUtil::Split(strPath.Left(pos), folder, file);
    else
      CUtil::Split(strPath, folder, file); // single filed stacks - should really not happen

    // remove "stack://" from the folder    
    folder = folder.Mid(8);
    file.Replace(",,", ",");
    CUtil::AddFileToFolder(folder, file, path);
    
    return path;
  }

  CStdString CStackDirectory::ConstructStackPath(const CFileItemList &items, const vector<int> &stack)
  {
    // no checks on the range of stack here.
    // we replace all instances of comma's with double comma's, then separate
    // the files using " , ".
    CStdString stackedPath = "stack://";
    CStdString folder, file;
    CUtil::Split(items[stack[0]]->m_strPath, folder, file);
    stackedPath += folder;
    // double escape any occurence of commas
    file.Replace(",", ",,");
    stackedPath += file;
    for (unsigned int i = 1; i < stack.size(); ++i)
    {
      stackedPath += " , ";
      file = items[stack[i]]->m_strPath;
      
      // double escape any occurence of commas
      file.Replace(",", ",,");
      stackedPath += file;
    }
    return stackedPath;
  }
}

