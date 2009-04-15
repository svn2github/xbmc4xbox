#include "Carbon/Carbon.h"
#import "XBMCHelper.h"
#include <getopt.h>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <iterator>

using namespace std;

//instantiate XBMCHelper which registers itself to IR handling stuff
XBMCHelper* g_xbmchelper;
eRemoteMode g_mode = DEFAULT_MODE;
std::string g_server_address="localhost";
bool g_universal_mode = false;
std::string g_app_path = "";
std::string g_app_home = "";

//
const char* PROGNAME="OSXRemote";
const char* PROGVERS="0.1";

void ParseOptions(int argc, char** argv);
void ReadConfig();

static struct option long_options[] = {
{ "help",       no_argument,       0, 'h' },
{ "server",     required_argument, 0, 's' },
{ "universal",  no_argument,       0, 'u' },
{ "multiremote",  no_argument,     0, 'm' },
{ "timeout",    required_argument, 0, 't' },
{ "verbose",    no_argument,       0, 'v' },
{ "externalConfig", no_argument,   0, 'x' },
{ "appPath", required_argument,   0, 'a' },
{ "appHome",    required_argument, 0, 'z' }, 
{ 0, 0, 0, 0 },
};
static const char *options = "hvt:ums:a:z:";

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void usage(void)
{
  printf("%s (version %s)\n", PROGNAME, PROGVERS);
  printf("   Sends Apple Remote events to XBMC.\n\n");
  printf("Usage: %s [OPTIONS...]\n\nOptions:\n", PROGNAME);
  printf("  -h, --help           print this help message and exit.\n");
  printf("  -s, --server <addr>  send events to the specified IP.\n");
  printf("  -u, --universal      runs in Universal Remote mode.\n");
  printf("  -t, --timeout <ms>   timeout length for sequences (default: 500ms).\n");
  printf("  -m, --multiremote    runs in Multi-Remote mode (adds remote identifier as additional idenfier to buttons)\n");
  printf("  -a, --appPath        path to XBMC.app (MenuPress launch support).\n");
  printf("  -z, --appHome        path to XBMC.app/Content/Resources/XBMX \n");
  printf("  -v, --verbose        prints lots of debugging information.\n");
}

//----------------------------------------------------------------------------
void ReadConfig()
{
	// Compute filename.
  std::string strFile = getenv("HOME");
  strFile += "/Library/Application Support/XBMC/XBMCHelper.conf";
  
	// Open file.
  std::ifstream ifs(strFile.c_str());
	if (!ifs)
		return;
  
	// Read file.
	stringstream oss;
	oss << ifs.rdbuf();
  
	if (!ifs && !ifs.eof())
		return;
  
	// Tokenize.
	string strData(oss.str());
	istringstream is(strData);
	vector<string> args = vector<string>(istream_iterator<string>(is), istream_iterator<string>());
  
	// Convert to char**.
	int argc = args.size() + 1;
	char** argv = new char*[argc + 1];
	int i = 0;
	argv[i++] = "XBMCHelper";
  
	for (vector<string>::iterator it = args.begin(); it != args.end(); )
		argv[i++] = (char* )(*it++).c_str();
	
	argv[i] = 0;
  
	// Parse the arguments.
	ParseOptions(argc, argv);
  
	delete[] argv;
}

//----------------------------------------------------------------------------
void ParseOptions(int argc, char** argv)
{
  int c, option_index = 0;
  //set the defaults
	bool readExternal = false;
  g_universal_mode = false;
  g_server_address = "localhost";
  g_mode = DEFAULT_MODE;
  g_app_path = "";
  g_app_home = "";
  
  while ((c = getopt_long(argc, argv, options, long_options, &option_index)) != -1) 
	{
    switch (c) {
      case 'h':
        usage();
        exit(0);
        break;
      case 'v':
        [g_xbmchelper enableVerboseMode:true];
        break;
      case 's':
        g_server_address = optarg;
        break;
      case 'u':
        g_mode = UNIVERSAL_MODE;
        break;
      case 'm':
        g_mode = MULTIREMOTE_MODE;
        break;        
      case 't':
        [g_xbmchelper setUniversalModeTimeout:atof(optarg) * 0.001];
        break;
      case 'x':
        readExternal = true;
        break;
      case 'a':
        g_app_path = optarg;
        break;
      case 'z':
        g_app_home = optarg;
        break;
      default:
        usage();
        exit(1);
        break;
    }
  }
	
	if (readExternal == true)
		ReadConfig();	
}

//----------------------------------------------------------------------------
void Reconfigure(int nSignal)
{
	if (nSignal == SIGHUP){
		ReadConfig();
    //set apppath to startup when pressing Menu
    [g_xbmchelper setApplicationPath:[NSString stringWithCString:g_app_path.c_str()]];    
    //set apppath to startup when pressing Menu
    [g_xbmchelper setApplicationHome:[NSString stringWithCString:g_app_home.c_str()]];
    //connect to specified server
    [g_xbmchelper connectToServer:[NSString stringWithCString:g_server_address.c_str()] withMode:g_mode];
  }
	else
    exit(0);
}

//----------------------------------------------------------------------------
int main (int argc,  char * argv[]) {
  NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
  
  g_xbmchelper = [[XBMCHelper alloc] init];  
  
  signal(SIGHUP, Reconfigure);
	signal(SIGINT, Reconfigure);
	signal(SIGTERM, Reconfigure);
  
  ParseOptions(argc,argv);
  
  //set apppath to startup when pressing Menu
  [g_xbmchelper setApplicationPath:[NSString stringWithCString:g_app_path.c_str()]];
  
  //set apppath to startup when pressing Menu
  [g_xbmchelper setApplicationHome:[NSString stringWithCString:g_app_home.c_str()]];  
  
  //connect to specified server
  [g_xbmchelper connectToServer:[NSString stringWithCString:g_server_address.c_str()] withMode:g_mode];
  
  //run event loop in this thread
  RunCurrentEventLoop(kEventDurationForever);
  
  //cleanup
  [g_xbmchelper release];
  [pool drain];
  return 0;
}
