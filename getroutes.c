/*
 * Copyright (c) 2019 Janne Johansson <jj@deadzoft.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* Includes code gleaned from OpenBSD unwind(8) */

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>

#include <net/route.h>
#include <netinet/in.h>

#define DEBUG 1

#define ROUNDUP(a) \
        ((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))


int main() {
  int mysocket, rtfilter;
  static int rtm_seq = 4711;
  size_t readlen = 0;

  struct rt_msghdr         rtm;
  struct sockaddr_in       sin_addr, sin_mask;
  
  struct iovec iov[3];
  
  mysocket=socket(AF_ROUTE, SOCK_RAW, AF_INET);
  if (mysocket < 0 ) {
    err(errno, "socket() failed");
    return 1;
  } else {
    
#ifdef DEBUG
    printf("socket gotten\n");
#endif
    
    rtfilter = ROUTE_FILTER(RTM_GET);
    
    if (setsockopt(mysocket, AF_ROUTE, ROUTE_MSGFILTER,
		   &rtfilter, sizeof(rtfilter)) < 0) {
      err(errno, "setsockopt(ROUTE_MSGFILTER)");
      return 1;
    }
#ifdef DEBUG
    printf("filter set\n");
#endif
    
    /* init sockaddr for 'address' and 'mask' */
    /* They could be the same but... */
    
    memset(&sin_addr, 0, sizeof(sin_addr));
    sin_addr.sin_family = AF_INET;
    sin_addr.sin_len = sizeof(sin_addr);
    
    memset(&sin_mask, 0, sizeof(sin_mask));
    sin_mask.sin_family = AF_INET;
    sin_mask.sin_len = sizeof(sin_mask);
    
    memset(&rtm, 0, sizeof(rtm));                                           
    
    rtm.rtm_version = RTM_VERSION;
    rtm.rtm_type = RTM_GET;
    rtm.rtm_msglen = sizeof(rtm);
    rtm.rtm_tableid = 0; /* no fancy rtable/rdomain stuff */
    rtm.rtm_seq = ++rtm_seq;
    rtm.rtm_pid = getpid();
    rtm.rtm_addrs = RTA_DST | RTA_NETMASK;
    
    iov[0].iov_base = &rtm;
    iov[0].iov_len = sizeof(rtm);
    
    iov[1].iov_base = &sin_addr;
    iov[1].iov_len = sizeof(sin_addr);
    rtm.rtm_msglen += iov[1].iov_len;
    
    iov[2].iov_base = &sin_mask;
    iov[2].iov_len = sizeof(sin_mask);
    rtm.rtm_msglen += iov[2].iov_len;
    
    if (writev(mysocket, iov, 3) == -1) {
      err(errno, "writev() failed to write to socket");
      return 1;
    }
    
#ifdef DEBUG
    printf("route message written, waiting for reply.\n");
#endif
    
    do {
      readlen = read(mysocket, (char *)&rtm, sizeof(rtm));
    } while (readlen > 0 && (rtm.rtm_seq != rtm_seq || rtm.rtm_pid != getpid()));

    if (rtm.rtm_errno)
      { perror("RTM says:"); }
    close(mysocket);

#ifdef DEBUG
    printf("route reply received, size %zu\n", readlen);
#endif

    /*    printf("Default gw", rtm.);  */
    
    return 0;
  }
}
