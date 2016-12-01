#include <jvmti.h>
#include <jni.h>

#include <Windows.h>
#include <io.h>

#include "../pd.h"

#define BUF_SZ 1024

static char *file;
static unsigned short port;
static unsigned int count;
static FILE *redirect = NULL;
static char tmpfname[2][MAX_PATH];
static int tmpfname_idx = 1;


static SOCKET initialize_listener(void){
  struct sockaddr_in addr;
  int addr_len;
  SOCKET sock;

  addr_len = sizeof(struct sockaddr_in);

  ZeroMemory(&addr, addr_len);
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  addr.sin_port = htons(port);

  if((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET){
    return INVALID_SOCKET;
  }

  if(bind(sock, (struct sockaddr *)&addr, addr_len) == SOCKET_ERROR){
    closesocket(sock);
    return INVALID_SOCKET;
  }

  if(listen(sock, 0) == SOCKET_ERROR){
    closesocket(sock);
    return INVALID_SOCKET;
  }

  if(getsockname(sock, (struct sockaddr *)&addr, &addr_len) == SOCKET_ERROR){
    closesocket(sock);
    return INVALID_SOCKET;
  }

  printf("printf: stdlog: binding port = %hu\n", ntohs(addr.sin_port));
  return sock;
}

static int reopen(void) {
  FILE *newfile;
  HANDLE hRedirect;
  int index;

  index = (tmpfname_idx + 1) % 2;

  if((newfile = _fsopen(tmpfname[index], "a", _SH_DENYNO)) == NULL){
    fprintf(stderr, "Could not open file: %s\n", tmpfname[index]);
    return -1;
  }

  if((hRedirect = (HANDLE)_get_osfhandle(_fileno(newfile))) == INVALID_HANDLE_VALUE){
    fprintf(stderr, "Could not get HANDLE for %s\n", tmpfname[index]);
    return -1;
  }

  _dup2(_fileno(newfile), 1);
  _dup2(_fileno(newfile), 2);
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);

  SetStdHandle(STD_OUTPUT_HANDLE, hRedirect);
  SetStdHandle(STD_ERROR_HANDLE, hRedirect);

  if(redirect != NULL){
    fclose(redirect);
  }

  redirect = newfile;
  DeleteFile(file);

  if(CreateHardLink(file, tmpfname[index], NULL) == 0){
    return GetLastError();
  }

  tmpfname_idx = index;

  return 0;
}

static int rotate(){
  unsigned int idx;
  char src[MAX_PATH], dest[MAX_PATH];
  int ret, old_idx;

  /* Remove the oldest log */
  sprintf_s(src, MAX_PATH, "%s.%u", file, count);
  DeleteFile(src);

  /* Rotate */ 
  for(idx = count - 1; idx > 0; idx--){
    sprintf_s(src, MAX_PATH, "%s.%d", file, idx);
    sprintf_s(dest, MAX_PATH, "%s.%d", file, idx + 1);

    MoveFileEx(src, dest, MOVEFILE_REPLACE_EXISTING);
  }

  if((ret = reopen()) != 0){
    return ret;
  }

  /* Reopen log and move current log to old. */
  old_idx = (tmpfname_idx + 1) % 2;
  if(count == 0){
    if(DeleteFile(tmpfname[old_idx]) == 0){
      return GetLastError();
    }
  }
  else{
    sprintf_s(dest, MAX_PATH, "%s.1", file);

    if(MoveFileEx(tmpfname[old_idx], dest, MOVEFILE_REPLACE_EXISTING) == 0){
      return GetLastError();
    }

  }

  return 0;
}

static int process_command(SOCKET sock){
  char buf[BUF_SZ] = { 0 };

  recv(sock, buf, BUF_SZ, 0);

  if(strcmp(buf, "rotate") == 0){
    if(rotate() != 0){
      pd_get_errorstr(buf, BUF_SZ);
      send(sock, buf, (int)strlen(buf), 0);
      return -1;
    }
  }
  else if(strcmp(buf, "reopen") == 0){
    if (reopen() != 0) {
      pd_get_errorstr(buf, BUF_SZ);
      send(sock, buf, (int)strlen(buf), 0);
      return -1;
    }
  }
  else{
    const char *response = "Unknown command";
    strcpy_s(buf, strlen(response), "Unknown command");
    send(sock, buf, (int)strlen(buf), 0);
    return -1;
  }

  return 0;
}

int pd_init(char *n, unsigned int p, unsigned int c){
  file = _strdup(n);
  port = p;
  count = c;

  sprintf_s(tmpfname[0], MAX_PATH, "%s.tmpA", file);
  sprintf_s(tmpfname[1], MAX_PATH, "%s.tmpB", file);

  return reopen();
}

int pd_finalize(){
  free(file);
  return 0;
}

void commander_entry(jvmtiEnv *jvmti, JNIEnv *env, void *arg) {
  char buf[BUF_SZ];
  WSADATA wsaData;
  SOCKET listener_sock, sock;


  if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0){
    pd_get_errorstr(buf, BUF_SZ);
    fprintf(stderr, "stdlog: %d @ %s: %s\n", __LINE__, __FILE__, buf);
    return;
  }

  if((listener_sock = initialize_listener()) == INVALID_SOCKET){
    fprintf(stderr, "stdlog: %d @ %s: %s\n", __LINE__, __FILE__, buf);
    return;
  }

  while((sock = accept(listener_sock, NULL, NULL)) != INVALID_SOCKET){
    process_command(sock);
    closesocket(sock);
  }

  closesocket(listener_sock);
  WSACleanup();
}

void pd_get_errorstr(char *buf, size_t buf_size){
  DWORD errcode = GetLastError();
  if(errcode == 0){
    errcode = WSAGetLastError();
  }

  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, errcode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, (DWORD)buf_size, NULL);
}

