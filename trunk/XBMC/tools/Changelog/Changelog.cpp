// Changelog.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "tinyxml.h"

char header[] = "*************************************************************************************************************\r\n"
                "*************************************************************************************************************\r\n"
                "                                     Xbox Media Center CHANGELOG\r\n"
                "*************************************************************************************************************\r\n"
                "*************************************************************************************************************\r\n"
                "\r\n"
                "Date        Rev   Message\r\n"
                "=============================================================================================================\r\n";

int _tmain(int argc, _TCHAR* argv[])
{
  std::string input = "svn_log.xml";
  std::string output = "Changelog.txt";
  if (argc < 2)
  {
    // output help information
    printf("usage:\n");
    printf("\n");
    printf("  Changelog input <output>\n");
    printf("\n");
    printf("  input    : input .xml file generated from SVN (using svn log --xml)\n");
    printf("             DOWNLOAD to download direct from XBMC SVN\n");
    printf("  <output> : output .txt file for the changelog (defaults to Changelog.txt)\n");
    printf("\n");
    return 0;
  }
  input = argv[1];
  if (argc > 2)
    output = argv[2];
  FILE *file = fopen(output.c_str(), "wb");
  if (!file)
    return 1;
  fprintf(file, header);
  if (input.compare("download") == 0)
  {
    // download our input file
    printf("Downloading changelog from CVS - this will take some time (around 1MB to download)\n");
    system("svn log --xml https://svn.sourceforge.net/svnroot/xbmc/trunk/XBMC > svn_log.xml");
    input = "svn_log.xml";
    printf("Downloading done - processing\n");
  }
  TiXmlDocument doc;
  if (!doc.LoadFile(input.c_str()))
  {
    return 1;
  }

  TiXmlElement *root = doc.RootElement();
  if (!root) return 1;

  TiXmlElement *logitem = root->FirstChildElement("logentry");
  while (logitem)
  {
    int revision;
    logitem->Attribute("revision", &revision);
    TiXmlNode *date = logitem->FirstChild("date");
    std::string dateString;
    if (date && date->FirstChild())
      dateString = date->FirstChild()->Value();
    TiXmlNode *msg = logitem->FirstChild("msg");
    if (msg && msg->FirstChild())
    {
      fprintf(file, "%s  %4i  %s\r\n", dateString.substr(0,10).c_str(), revision, msg->FirstChild()->Value());
    }
    logitem = logitem->NextSiblingElement("logentry");
  }
  fclose(file);
  printf("Changelog saved as: %s\n", output.c_str());
	return 0;
}

