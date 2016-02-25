/*
  zseal - FICS Timeseal 2 client implementation
  (C) 2016 Felipe Bergo <fbergo at gmail.com>
  repository at https://github.com/fbergo/zseal
  
  The MIT License (MIT)

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <pwd.h>

#include <sys/time.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netdb.h>

#ifdef LINUX
#include <net/if.h>
#include <net/if_arp.h>
#include <sys/ioctl.h>
#endif

#include <sys/select.h>

static int  zconnect(const char *hostname,int port);
static int  zstamp(char *s,int l);
static void zwrite(int fd,const char *buffer,int n);
static void zreceive(int fd, char *buffer, int *rd);
static void zsend(int fd, char *buffer, int *rd);
static void zid(char *dest, int sz);
static void zdie();
static void zchomp(char *s);
static void zclean(char *s);

const int   BSZ    =  8192;
const char *TS_KEY = "Timestamp (FICS) v1.0 - programmed by Henrik Gram.";

int main(int argc, char **argv) {
  char   hostname[256], hello[512], id[256];
  int    port = 5000;
  int    fd,n;
  fd_set fds;
  char  *buffer1=NULL, *buffer2 = NULL;
  int    bpos1=0, bpos2=0;

  strcpy(hostname,"freechess.org");

  zid(id,256);
  snprintf(hello,511,"TIMESEAL2|%s|",id);

  buffer1 = (char *) malloc(BSZ);
  buffer2 = (char *) malloc(BSZ);
  if (buffer1 == NULL || buffer2 == NULL)  zdie();
  memset(buffer1,0,BSZ);
  memset(buffer2,0,BSZ);

  if (argc >= 2) { memset(hostname,0,256); strncpy(hostname, argv[1], 255); }
  if (argc >= 3) { port = atoi(argv[2]); }

  fd = zconnect(hostname, port);
  if (fd<0) zdie();
  n = zstamp(hello,strlen(hello));
  zwrite(fd,hello,n);

  while(1) {
    FD_ZERO(&fds);
    FD_SET(0,&fds);
    FD_SET(fd,&fds);
    select(fd+1,&fds,NULL,NULL,NULL);

    // stdin input
    if(FD_ISSET(0,&fds)) {
      bpos1 += n = read(0,buffer1+bpos1,BSZ-bpos1);

      if (n==0) {
        fprintf(stderr,"EOF from stdin\n");
        exit(0);
      }

      if (n==-1) zdie();
      zsend(fd,buffer1,&bpos1);

      if (bpos1==BSZ) {
        fprintf(stderr,"local input too long for our buffer.\n");
        exit(1);
      }
    }

    // network input
    if(FD_ISSET(fd,&fds)) {
      bpos2 += n = read(fd,buffer2,BSZ-bpos2);
      if (!n) {
        fprintf(stderr,"Connection closed by ICS\n");
        exit(0);
      }
      if(n==-1) zdie();

      zreceive(fd,buffer2,&bpos2);
      if(bpos2==BSZ) {
        fprintf(stderr,"network input too long for our buffer.\n");
        exit(1);
      }
    }
  }

  return 0;
}

static void zdie() {
  perror(NULL);
  exit(1);
}

static int zconnect(const char *hostname,int port) {
  int sockfd, res;
  struct addrinfo *addr = NULL;
  char s_port[16];

  memset(s_port,0,16);
  snprintf(s_port,15,"%d",port);

  sockfd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
  if (sockfd < 0) return -1;
  if (getaddrinfo(hostname,s_port,0,&addr)!=0) return -1;
  res = connect(sockfd, addr->ai_addr, (int) addr->ai_addrlen);
  freeaddrinfo(addr);
  if (res < 0) return -1;
  return sockfd;
}

static void zwrite(int fd,const char *buffer,int n) {
  if (write(fd,buffer,n)==-1) zdie();
}

static int zstamp(char *s,int l) {
  int n;
  struct timeval tv;
  s[l++]=0x18;
  gettimeofday(&tv,NULL);
  l+=sprintf(&s[l],"%ld",(tv.tv_sec%10000)*1000+tv.tv_usec/1000);
  s[l++]=0x19;
  for(;l%12;l++)
    s[l]=0x31;
#define SC(A,B) s[B]^=s[A]^=s[B],s[A]^=s[B]
  for(n=0;n<l;n+=12)
    SC(n,n+11), SC(n+2,n+9), SC(n+4,n+7);
  for(n=0;n<l;n++)
    s[n]=((s[n]|0x80)^TS_KEY[n%50])-32;
  s[l++]=0x80;
  s[l++]=0x0a;
  return l;
}

static void zsend(int fd, char *buffer, int *rd) {
  int i,k;
  static char *sendbuffer = NULL;

  if (sendbuffer == NULL) sendbuffer = (char *) malloc(BSZ+32);

  for(i=0;i<*rd;i++) {
    if (buffer[i]==0x0a) {
      memcpy(sendbuffer, buffer, i);

      k = zstamp(sendbuffer,i);
      zwrite(fd,sendbuffer,k);

      (*rd) -= (i+1);
      if ((*rd) > 0) {
	memmove(buffer, buffer+i+1, (*rd)); 
	i = -1;
      }
    }
  }
}

static void zreceive(int fd, char *buffer, int *rd) {
  static const char REQ[4]  = { '[', 'G' , ']', 0x00 };
  static const char ACK[2]  = { 0x02, 0x39 };
  char reply[32];
  int i,j;

  while(*rd>0) {
    // consume and reply to all ping requests
    for(i=0;i<=(*rd)-4;i++) {
      if (memcmp(buffer+i,REQ,4)==0) {
	memcpy(reply,ACK,2);
	j=zstamp(reply,2);
	zwrite(fd,reply,j);

	(*rd) -= 4;
	if ((*rd)-i > 0) {
	  memmove(buffer+i, buffer+i+4, (*rd)-i); 
	  i--;
	}
      }
    }
    // consume line
    for(i=0;i<*rd && buffer[i]!=0x0d;i++) ; 
    if (i<*rd) i++;
    zwrite(1,buffer,i);
    (*rd) -= i;
    if (*rd > 0) memmove(buffer, buffer+i, (*rd)); 
  }
}

static void zclean(char *s) {
  int i;
  for(i=0;i<strlen(s);i++) if (s[i]<32 || s[i]>'z') s[i] = '_';
}

static void zchomp(char *s) {
  int i;
  i = strlen(s) - 1;
  while(i>=0 && s[i]<32) s[i--]=0;
}

static void zid(char *dest, int sz) {
  char user[32], uname[128], mac[32], netdev[64], tmp[512];
  FILE *f;
  struct passwd *pw;

#ifdef LINUX
  struct ifreq ifr;
  int ifsock;
  unsigned char *mptr;
  const char *tok = " \t\n\r";
  char *t;
#endif

  memset(user,0,32);
  memset(uname,0,128);
  memset(mac,0,32);
  memset(netdev,0,64);

  pw = getpwuid(geteuid());
  if (pw != NULL) strncpy(user,pw->pw_name,31);

  f = popen("uname -a 2>&1","r");
  if (f!=NULL) {
    memset(tmp,0,512);
    if (fgets(tmp, 511, f)!=NULL) {
      zchomp(tmp);
      strncpy(uname,tmp,127);
    }
    pclose(f);
  }

#ifdef LINUX
  f = fopen("/proc/net/route","r");
  if (f!=NULL) {
    while(fgets(tmp, 511, f)!=NULL) {
      zchomp(tmp);
      t = strtok(tmp,tok);
      if (t==NULL) continue;
      if (strcmp(t, "Iface")==0) continue;
      strncpy(netdev,t,63);
      t = strtok(NULL, tok);
      if (t==NULL) continue;
      if (strcmp(t, "00000000")==0) break;
      memset(netdev,0,64);
    }
    fclose(f);
  }

  if (netdev[0]!=0) {
    memset(mac,0,32);
    memset(ifr.ifr_name, 0, IFNAMSIZ);
    strncpy(ifr.ifr_name, netdev, IFNAMSIZ-1);
    ifsock = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (ifsock >= 0) {
      if (ioctl(ifsock,SIOCGIFHWADDR,&ifr)>=0) {
	if (ifr.ifr_hwaddr.sa_family==ARPHRD_ETHER) {
	  mptr = (unsigned char *) ifr.ifr_hwaddr.sa_data;
	  snprintf(mac,31,"%.2x:%.2x:%.2x:%.2x:%.2x:%.2x",
		   (int)(mptr[0]),(int)(mptr[1]),(int)(mptr[2]),
		   (int)(mptr[3]),(int)(mptr[4]),(int)(mptr[5]));
	}
      }
      close(ifsock);
    }
  }
#endif

  zclean(user);
  zclean(uname);
  zclean(mac);
  
  memset(dest,0,sz);
  if (strlen(mac)>0)
    snprintf(dest,sz-1,"%s (zseal)|%s %s",user, mac, uname);
  else
    snprintf(dest,sz-1,"%s (zseal)|%s",user, uname);
}
