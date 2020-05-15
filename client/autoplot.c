// This code is in the public domain

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "autoplot.h"


static pthread_once_t ap_once = PTHREAD_ONCE_INIT;
static int ap_fd = -1;

static void
ap_init(void)
{
  const char *ap_host = getenv("AUTOPLOT_HOST") ?: "127.0.0.1";
  if(!strcmp(ap_host, "DISABLE"))
    return;

  ap_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if(ap_fd == -1)
    return;

  struct sockaddr_in sin = {
    .sin_family = AF_INET,
    .sin_port = htons(5678),
    .sin_addr.s_addr = inet_addr(ap_host)
  };

  if(connect(ap_fd, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
    perror("autoplot connect()");
    close(ap_fd);
    ap_fd = -1;
    return;
  }
}


void
autoplot(autoplot_kind_t kind, ...)
{
  char packet[1400];

  pthread_once(&ap_once, ap_init);

  if(ap_fd == -1)
    return;

  va_list ap;
  va_start(ap, kind);

  char *cur = packet;

  while(kind) {
    float f;
    uint64_t u64;

    const char *name = va_arg(ap, const char *);

    size_t namelen = strlen(name);
    if(namelen > 255)
      break;

    if(cur + 1 + 1 + namelen + 8 > packet + sizeof(packet))
      break;

    *cur++ = kind;
    *cur++ = namelen;
    memcpy(cur, name, namelen);
    cur += namelen;

    switch(kind) {
    default:
      abort();

    case AUTOPLOT_GAUGE:
      f = va_arg(ap, double);
      memcpy(cur, &f, sizeof(f));
      cur += sizeof(f);
      break;

    case AUTOPLOT_COUNTER:
      u64 = va_arg(ap, uint64_t);
      memcpy(cur, &u64, sizeof(u64));
      cur += sizeof(u64);
      break;
    }
    kind = va_arg(ap, autoplot_kind_t);
  }
  va_end(ap);

  if(write(ap_fd, packet, cur - packet)) {}

}
