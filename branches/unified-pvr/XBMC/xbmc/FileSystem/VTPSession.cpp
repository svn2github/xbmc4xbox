#include "stdafx.h"
#include "VTPSession.h"

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string>
#include <vector>

#if defined(_LINUX)
#include <unistd.h>
#elif defined(_WIN32)
typedef int socklen_t;
extern "C" int inet_pton(int af, const char *src, void *dst);
#endif

using namespace std;

#define DEBUG


#if VTP_STANDALONE

#ifdef HAVE_WINSOCK2
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#define closesocket(a) close(a)
typedef int SOCKET;
#define SOCKET_ERROR   (-1)
#define INVALID_SOCKET (-1)
#endif


const int LOGDEBUG = 0;
const int LOGERROR = 1;
namespace CLog {
  void Log(int level, const char* format) 
  { printf(format);
    printf("\n"); 
  }
  template<typename T1>
  void Log(int level, const char* format, T1 p1) 
  { printf(format, p1); 
    printf("\n"); 
  }

  template<typename T1, typename T2>
  void Log(int level, const char* format, T1 p1, T2 p2) 
  { printf(format, p1, p2); 
    printf("\n"); 
  }
  template<typename T1, typename T2, typename T3>
  void Log(int level, const char* format, T1 p1, T2 p2, T3 p3) 
  { printf(format, p1, p2, p3); 
    printf("\n"); 
  }
}
#endif

CVTPSession::CVTPSession()
  : m_socket(INVALID_SOCKET)
{}

CVTPSession::~CVTPSession()
{
  Close();
  Quit();
}

bool CVTPSession::OpenStreamSocket(SOCKET& sock, struct sockaddr_in& address2)
{
  struct sockaddr_in address(address2);

  sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  if(sock == INVALID_SOCKET)
  {
    CLog::Log(LOGERROR, "CVTPSession::OpenStreamSocket - invalid socket");
    return false;
  }

  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port        = 0;

  if(bind(sock, (struct sockaddr*) &address, sizeof(address)) == SOCKET_ERROR)
  {
    CLog::Log(LOGERROR, "CVTPSession::OpenStreamSocket - bind failed");
    return false;
  }

  socklen_t len = sizeof(address);
  if(getsockname(sock, (struct sockaddr*) &address, &len) == SOCKET_ERROR)
  {
    CLog::Log(LOGERROR, "CVTPSession::OpenStreamSocket - bind failed");
    return false;
  }

  if(listen(sock, 1) == SOCKET_ERROR)
  {
    CLog::Log(LOGERROR, "CVTPSession::OpenStreamSocket - listen failed");
    return false;
  }

  address2.sin_port = address.sin_port;

  CLog::Log(LOGDEBUG, "CVTPSession::OpenStreamSocket - listening on %s:%d", inet_ntoa(address.sin_addr), address.sin_port);
  return true;
}

bool CVTPSession::AcceptStreamSocket(SOCKET& sock2)
{
  SOCKET sock;
  sock = accept(sock2, NULL, NULL);
  if(sock == INVALID_SOCKET)
  {
    CLog::Log(LOGERROR, "CVTPStream::Accept - failed to accept incomming connection");
    return false;
  }

  closesocket(sock2);
  sock2 = sock;
  return true;
}

void CVTPSession::Close()
{
  if(m_socket != INVALID_SOCKET)
    closesocket(m_socket);
}

bool CVTPSession::Open(const string &host, int port)
{
  struct sockaddr_in address = {};

  m_socket = socket(AF_INET, SOCK_STREAM, 0);
  if(m_socket == INVALID_SOCKET)
    return false;


  address.sin_family = AF_INET;
  address.sin_port   = htons(port);

  if(inet_pton(AF_INET, host.c_str(), &address.sin_addr) != 1)
  {
    struct hostent* hp = gethostbyname(host.c_str());
    if(!hp)
    {
      CLog::Log(LOGERROR, "CVTPSession::Open - failed to resolve hostname %s", host.c_str());
      Close();
      return false;
    }

    memcpy(&address.sin_addr, hp->h_addr_list[0], hp->h_length);
  }

  CLog::Log(LOGDEBUG, "CVTPSession::Open - connecting to: %s:%d ...", inet_ntoa(address.sin_addr), address.sin_port);

  if(connect(m_socket, (struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR)
  {
    CLog::Log(LOGERROR, "CVTPSession::Open - failed to connect to server");
    Close();
    return false;
  }

  // VTP Server will send a greeting
  string line;
  int    code;
  ReadResponse(code, line);

  CLog::Log(LOGERROR, "CVTPSession::Open - server greeting: %s", line.c_str());

  return true;
}

bool CVTPSession::IsOpen()
{
  if(m_socket == INVALID_SOCKET)
    return false;
  else
    return true;
}

bool CVTPSession::ReadResponse(int &code, string &line)
{
  vector<string> lines;
  if(ReadResponse(code, lines))
  {
    line = lines[lines.size()-1];
    return true;
  }
  return false;
}

bool CVTPSession::ReadResponse(int &code, vector<string> &lines)
{
  fd_set         set_r, set_e;
  struct timeval tv;
  int            result;
  int            retries = 5;
  char           buffer[256];
  char           cont = 0;
  string         line;
  size_t         pos1 = 0, pos2 = 0, pos3 = 0;

  while(true)
  {
    while( (pos1 = line.find("\r\n", pos3)) != std::string::npos)
    {
      if(sscanf(line.c_str(), "%d%c", &code, &cont) != 2)
      {
        CLog::Log(LOGDEBUG, "CVTPSession::ReadResponse - unknown line format: %s", line.c_str());
        line.erase(0, pos1 + 2);
        continue;
      }

      pos2 = line.find(cont);

      lines.push_back(line.substr(pos2+1, pos1-pos2-1));

      line.erase(0, pos1 + 2);
      pos3 = 0;
    }

    // we only need to recheck 1 byte
    if(line.size() > 0)
      pos3 = line.size() - 1;
    else
      pos3 = 0;

    if(cont == ' ')
      break;

    // fill with new data
    tv.tv_sec  = 10;
    tv.tv_usec = 0;

    FD_ZERO(&set_r);
    FD_ZERO(&set_e);
    FD_SET(m_socket, &set_r);
    FD_SET(m_socket, &set_e);
    result = select(FD_SETSIZE, &set_r, NULL, &set_e, &tv);
    if(result < 0)
    {
      CLog::Log(LOGDEBUG, "CVTPSession::ReadResponse - select failed");
      m_socket = INVALID_SOCKET;
      return false;
    }

    if(result == 0)
    {
      CLog::Log(LOGDEBUG, "CVTPSession::ReadResponse - timeout waiting for response, retrying...");
      if (retries != 0) {
          retries--;
      continue;
    }
      else {
          m_socket = INVALID_SOCKET;
          return false;
      }
    }

    result = recv(m_socket, buffer, sizeof(buffer), 0);
    if(result < 0)
    {
      CLog::Log(LOGDEBUG, "CVTPSession::ReadResponse - recv failed");
      m_socket = INVALID_SOCKET;
      return false;
    }
    buffer[result] = 0;

    line.append(buffer);
  }

#ifdef DEBUG
  CLog::Log(LOGDEBUG, "CVTPSession::ReadResponse - Response code %d", code);
  for(unsigned i=0; i<lines.size(); i++)
    CLog::Log(LOGDEBUG, "CVTPSession::ReadResponse - Line %d: %s", i, lines[i].c_str());
#endif

  return true;
}

bool CVTPSession::SendCommand(const string &command)
{
  fd_set set_w, set_e;
  struct timeval tv;
  int  result;
  char buffer[1024];
  int  len;

  len = sprintf(buffer, "%s\r\n", command.c_str());
#ifdef DEBUG
  CLog::Log(LOGERROR, "CVTPSession::SendCommand - sending '%s'", command.c_str());
#endif
  // fill with new data
  tv.tv_sec  = 0;
  tv.tv_usec = 0;

  FD_ZERO(&set_w);
  FD_ZERO(&set_e);
  FD_SET(m_socket, &set_w);
  FD_SET(m_socket, &set_e);
  result = select(FD_SETSIZE, &set_w, NULL, &set_e, &tv);
  if(result < 0)
  {
    CLog::Log(LOGERROR, "CVTPSession::SendCommand - select failed");
    m_socket = INVALID_SOCKET;
    return false;
  }
  if (FD_ISSET(m_socket, &set_w))
  {
    CLog::Log(LOGERROR, "CVTPSession::SendCommand - failed to send data");
    m_socket = INVALID_SOCKET;
    return false;
  }
  if(send(m_socket, buffer, len, 0) != len)
  {
    CLog::Log(LOGERROR, "CVTPSession::SendCommand - failed to send data");
    m_socket = INVALID_SOCKET;
    return false;
  }
  return true;
}

bool CVTPSession::SendCommand(const string &command, int &code, string line)
{
  vector<string> lines;
  if(SendCommand(command, code, lines))
  {
    line = lines[lines.size()-1];
    return true;
  }
  return false;
}

bool CVTPSession::SendCommand(const string &command, int &code, vector<string> &lines)
{
  if(!SendCommand(command))
    return false;

  if(!ReadResponse(code, lines))
    return false;

  if(code < 200 || code > 299)
  {
    CLog::Log(LOGERROR, "CVTPSession::SendCommand - Failed with code: %d (%s)", code, lines[lines.size()-1].c_str());
    return false;
  }

  return true;
}

bool CVTPSession::GetChannels(vector<Channel> &channels)
{
  vector<string> lines;
  int            code;

  if(!SendCommand("LSTC", code, lines))
    return false;

  for(vector<string>::iterator it = lines.begin(); it != lines.end(); it++)
  {
    string& data(*it);
    size_t space, colon;
    struct Channel channel;

    space = data.find(" ");
    if(space == string::npos)
    {
      CLog::Log(LOGERROR, "CVTPSession::GetChannels - failed to parse line %s", it->c_str());
      continue;    
    }

    colon = data.find(":", space+1);
    if(colon == string::npos)
    {
      CLog::Log(LOGERROR, "CVTPSession::GetChannels - failed to parse line %s", it->c_str());
      continue;
    }

    channel.index = atoi(data.substr(0, space).c_str());
    channel.name  = data.substr(space+1, colon-space-1);

    colon = channel.name.find(";");
    if(colon != string::npos)
    {
      channel.network = channel.name.substr(colon+1);
      channel.name.erase(colon);
    }

    channels.push_back(channel);

#ifdef DEBUG
    CLog::Log(LOGDEBUG, "CVTPSession::GetChannels - Channel:%d, Name: '%s', Network: '%s'", channel.index, channel.name.c_str(), channel.network.c_str());
#endif
  }
  return true;
}

SOCKET CVTPSession::GetStreamLive(int channel)
{

  sockaddr_in address;
  SOCKET      sock;
  socklen_t   len = sizeof(address);
  char        buffer[1024];
  string      result;
  int         code;

  if(!SendCommand("CAPS TS", code, result))
  {
    CLog::Log(LOGERROR, "CVTPSession::GetStreamLive - server is unable to provide mpeg-ts");
    return INVALID_SOCKET;
  }

  sprintf(buffer, "PROV %d %d", 100, channel);
  if(!SendCommand(buffer, code, result))
  {
    CLog::Log(LOGERROR, "CVTPSession::GetStreamLive - server is unable to provide channel");
    return INVALID_SOCKET;
  }

  sprintf(buffer, "TUNE %d", channel);
  if(!SendCommand(buffer, code, result))
  {
    CLog::Log(LOGERROR, "CVTPSession::GetStreamLive - server is unable to tune to said channel");
    return INVALID_SOCKET;
  }

  if(getsockname(m_socket, (struct sockaddr*) &address, &len) == SOCKET_ERROR)
  {
    CLog::Log(LOGERROR, "CVTPSession::GetStreamLive - getsockname failed");
    return INVALID_SOCKET;
  }

  CLog::Log(LOGDEBUG, "CVTPSession::GetStreamLive - local address %s:%d", inet_ntoa(address.sin_addr), ntohs(address.sin_port) );

  if(!OpenStreamSocket(sock, address))
    return INVALID_SOCKET;

  int port = ntohs(address.sin_port);
  int addr = ntohl(address.sin_addr.s_addr);

  sprintf(buffer, "PORT 0 %d,%d,%d,%d,%d,%d"
                , (addr & 0xFF000000)>>24
                , (addr & 0x00FF0000)>>16
                , (addr & 0x0000FF00)>>8
                , (addr & 0x000000FF)>>0
                , (port & 0xFF00)>>8
                , (port & 0x00FF)>>0);

  if(!SendCommand(buffer, code, result))
    return NULL;

  if(!AcceptStreamSocket(sock))
  {
    closesocket(sock);
    return INVALID_SOCKET;
  }
  return sock;
}

SOCKET CVTPSession::GetStreamRecording(int recording, uint64_t *size, uint32_t *frames) {

    sockaddr_in address;
    SOCKET      sock;
    socklen_t   len = sizeof(address);
    char        buffer[1024];
    string      result;
    int         code;
    vector<string> lines;

    sprintf(buffer, "PLAY %d", recording);
    if (!SendCommand(buffer, code, lines)) {
        return INVALID_SOCKET;
    }

    vector<string>::iterator it = lines.begin();
    string& data(*it);

    sscanf(data.c_str(), "%I64u", size);
    data.erase(0,data.find(" ", 0)+1);
    *frames = atol(data.c_str());

    if(getsockname(m_socket, (struct sockaddr*) &address, &len) == SOCKET_ERROR) {
        CLog::Log(LOGERROR, "CVTPSession::GetStreamRecording - getsockname failed");
        return INVALID_SOCKET;
    }

    CLog::Log(LOGDEBUG, "CVTPSession::GetStreamRecording - local address %s:%d", inet_ntoa(address.sin_addr), ntohs(address.sin_port) );

    if(!OpenStreamSocket(sock, address))
        return INVALID_SOCKET;

    int port = ntohs(address.sin_port);
    int addr = ntohl(address.sin_addr.s_addr);

    sprintf(buffer, "PORT 1 %d,%d,%d,%d,%d,%d"
                  , (addr & 0xFF000000)>>24
                  , (addr & 0x00FF0000)>>16
                  , (addr & 0x0000FF00)>>8
                  , (addr & 0x000000FF)>>0
                  , (port & 0xFF00)>>8
                  , (port & 0x00FF)>>0);

    if(!SendCommand(buffer, code, result)) {
        return INVALID_SOCKET;
    }

    if(!AcceptStreamSocket(sock)) {
        closesocket(sock);
        return INVALID_SOCKET;
    }
    return sock;
}

void CVTPSession::AbortStreamLive()
{
  if(m_socket == INVALID_SOCKET)
    return;

  string line;
  int    code;
  if(!SendCommand("ABRT 0", code, line))
    CLog::Log(LOGERROR, "CVTPSession::AbortStreamLive - failed");
}

void CVTPSession::AbortStreamRecording()
{
  if(m_socket == INVALID_SOCKET)
    return;

  string line;
  int    code;
  if(!SendCommand("ABRT 1", code, line))
    CLog::Log(LOGERROR, "CVTPSession::AbortStreamRecording - failed");
}

bool CVTPSession::CanStreamLive(int channel)
{
  if(m_socket == INVALID_SOCKET)
    return false;

  char   buffer[1024];
  string line;
  int    code;

  sprintf(buffer, "PROV %d %d", -1, channel);
  if(!SendCommand(buffer, code, line))
  {
    CLog::Log(LOGERROR, "CVTPSession::CanStreamLive - server is unable to provide channel %d", channel);
    return false;
  }
  return true;
}

bool CVTPSession::SuspendServer() {

    vector<string> lines;
    int            code;
    bool           ret;

    ret = SendCommand("SUSP", code, lines);

    if (!ret || code != 220)
    {
        CLog::Log(LOGERROR, "CVTPSession::SuspendServer: Couldn't suspend server");
        return false;
    }
    return true;
}

bool CVTPSession::Quit() {

    vector<string> lines;
    int            code;
    bool           ret;

    ret = SendCommand("QUIT", code, lines);

    if (!ret || code != 221)
    {
        CLog::Log(LOGERROR, "CVTPSession::Quit: Couldn't quit command connection");
        return false;
    }
    return true;
}

#if VTP_STANDALONE
int main()
{
  CVTPSession session;
  session.Open("localhost", 2004);

  vector<CVTPSession::Channel> channels;
  session.GetChannels(channels);

  SOCKET s = session.GetStreamLive(1);
  if(s == INVALID_SOCKET)
  {
    printf("invalid socket");
    return 0;
  }

  char buffer[1024];
  int r = recv(s, buffer, sizeof(buffer)-1, 0);
  if(r < 0)
    perror("Error");

  if(r > 0)
    buffer[r] = 0;
  else
    buffer[0] = 0;

  printf("len %d, data %s", r, buffer);
}
#endif
