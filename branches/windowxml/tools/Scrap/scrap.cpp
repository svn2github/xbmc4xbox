// scrap.cpp : Defines the entry point for the console application.
//

#include <tchar.h>
#include <iostream>
#include <curl/curl.h>
#include "ScraperParser.h"
#include "Scraper.h"

using namespace std;

size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
  int written = fwrite(ptr, size, nmemb, (FILE *)stream);
  return written;
}
void get_url(CStdString strFilename, CStdString strUrl)
{
  CURL *curl;
  curl = curl_easy_init();
  
  FILE* f = fopen(strFilename.c_str(),"w");

  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA ,f);
  curl_easy_setopt(curl, CURLOPT_URL, strUrl.c_str());
 
  
	curl_easy_perform(curl);
  fclose(f);

  curl_easy_cleanup(curl);

}

int _tmain(int argc, _TCHAR* argv[])
{
 
	if (argc < 3)
	{
    cout << "Error: Not enough arguments. Need a xml file and a movie name" << endl;
    cout << "Usage: " << argv[0] << " imdb.xml \"Fight Club\" [CreateSearchUrl]" << endl;
		return -1;
	}
  Scraper scrap(argv[1]);
  
  //Default operation
  if(!argv[3]) argv[3] = "CreateSearchUrl";
  //only set movie name as buffer 1 if we use GetResults
  if( strcmp(argv[3], "CreateSearchUrl") == 0 )   scrap.SetBuffer(1,argv[2]);
  
  //Let the parser do its job
  if(!scrap.Parse(argv[3])) return -1;
  return 0;
}
