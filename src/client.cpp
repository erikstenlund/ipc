#include "Socket.hpp"

#include <cstdio>
#include <iostream>

#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char** argv) {
    est::socket::UnixDomainSocket socket("/tmp/socket-foo");
    int fd = socket; int b =2l;
    printf("Socket %d\n", fd);
    int rc, written; char buf[100];
  while( (rc=read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
      printf("Read %d\n", rc);
    if ((written = write(fd, buf, rc)) != rc) {
      if (rc > 0) printf("%s, %d\n","partial write", written);
      else {
        printf("write error");
        exit(-1);
      }
    }
  }
	return 0;
}
