#include <jni.h>
#include <jvmti.h>

#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <errno.h>

#include "../pd.h"

#define BUF_SZ 1024
#define LOG_OPEN_FLAG O_CREAT | O_APPEND | O_WRONLY | O_CLOEXEC | \
                                                O_LARGEFILE | O_DSYNC
#define LOG_OPEN_MODE S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH

#define SYSCALL_ERR_CHECK(x) \
  if((x) == -1){                                                       \
    char buf[BUF_SZ];                                                  \
    pd_get_errorstr(buf, BUF_SZ);                                      \
    fprintf(stderr, "stdlog: %d @ %s: %s\n", __LINE__, __FILE__, buf); \
    return -1;                                                         \
  }

static char *file;
static unsigned int count;
static unsigned short port;


static int initialize_listener(void){
  int sock;
  struct sockaddr_in addr;
  socklen_t addr_len;

  addr_len = sizeof(struct sockaddr_in);
  memset(&addr, 0, addr_len);
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  addr.sin_port = htons(port);

  SYSCALL_ERR_CHECK(sock = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0))
  SYSCALL_ERR_CHECK(bind(sock, (struct sockaddr *)&addr, addr_len))
  SYSCALL_ERR_CHECK(listen(sock, 0))

  SYSCALL_ERR_CHECK(getsockname(sock, (struct sockaddr *)&addr, &addr_len))
  printf("stdlog: binding port = %hu\n", ntohs(addr.sin_port));

  return sock;
}

static int reopen(void){
  int redirect_fd;

  SYSCALL_ERR_CHECK(redirect_fd = open(file, LOG_OPEN_FLAG, LOG_OPEN_MODE))
  SYSCALL_ERR_CHECK(lseek64(redirect_fd, 0L, SEEK_END))
  SYSCALL_ERR_CHECK(dup2(redirect_fd, STDOUT_FILENO))
  SYSCALL_ERR_CHECK(dup2(redirect_fd, STDERR_FILENO))

  close(redirect_fd);
}

static int rotate(void){
  unsigned int idx;
  char src[PATH_MAX], dest[PATH_MAX];

  /* Remove the oldest log */
  sprintf(src, "%s.%u", file, count);
  unlink(src);

  /* Rotate */ 
  for(idx = count - 1; idx > 0; idx--){
    sprintf(src, "%s.%d", file, idx);
    sprintf(dest, "%s.%d", file, idx + 1);

    rename(src, dest);
  }

  /* Reopen log and move current log to old. */
  if(count == 0){
    SYSCALL_ERR_CHECK(unlink(file))
  }
  else{
    sprintf(dest, "%s.1", file);
    SYSCALL_ERR_CHECK(rename(file, dest))
  }

  return reopen();
}

static int process_command(int fd){
  char buf[BUF_SZ];

  read(fd, buf, BUF_SZ);

  if(strcmp(buf, "rotate") == 0){
    if(rotate() != 0){
      pd_get_errorstr(buf, BUF_SZ);
      write(fd, buf, strlen(buf));
      return -1;
    }
  }
  else if(strcmp(buf, "reopen") == 0){
    if(reopen() == -1){
      pd_get_errorstr(buf, BUF_SZ);
      write(fd, buf, strlen(buf));
      return -1;
    }
  }
  else{
    strcpy(buf, "Unknown command");
    write(fd, buf, strlen(buf));
    return -1;
  }

  return 0;
}

int pd_init(char *n, unsigned int p, unsigned int c){
  file = strdup(n);
  port = p;
  count = c;

  return reopen();
}

int pd_finalize(){
  free(file);
  return 0;
}

void commander_entry(jvmtiEnv *jvmti, JNIEnv *env, void *arg){
  int listener_fd, fd;
  struct sockaddr_in addr;
  socklen_t addr_len;

  if((listener_fd = initialize_listener()) == -1){
    fprintf(stderr, "stdlog: command listener is disabled.\n");
  }

  while((fd = accept4(listener_fd, (struct sockaddr *)&addr,
                                               &addr_len, SOCK_CLOEXEC)) != -1){
    process_command(fd);
    close(fd);
  }

  close(listener_fd);
}

void pd_get_errorstr(char *buf, size_t buf_size){
  char *err_msg = strerror_r(errno, buf, buf_size);
  strncpy(buf, err_msg, buf_size);
}

