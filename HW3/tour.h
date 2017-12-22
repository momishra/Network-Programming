#ifndef __TOUR_H
#define __TOUR_H

#include <sys/socket.h>
#include <linux/if_packet.h>
#include <linux/if_arp.h>
#include <stdlib.h>
#include <stdio.h>
#include <ifaddrs.h>
#include<netinet/ip.h>
#include<netinet/ip_icmp.h>
#include "unp.h"
#include "arp.h"

#define PROTO_ID 169
#define IDENT 0x6169
#define MULTICAST_IP "230.246.100.100"
#define MULTICAST_PORT 7369
#define TTL 1

struct hwaddr{
  int sll_ifindex; /*interface number*/
  unsigned short sll_hatype; /*hardware type*/
  unsigned char sll_halen; /*length of address*/
  unsigned char sll_addr[8]; /*physical layer address*/
};

struct pingheader{
  struct ip iph;
  struct icmp icmp_ph;
};

struct pingpacket{
  struct pingheader pph;
  char destmac[6];
  char srcmac[6];
  uint16_t protocol;
};

struct ippacket{
  struct ip header;
  struct in_addr ip;
  uint16_t port;
  uint16_t index;
  uint16_t l_index;
  struct in_addr payload[100];
};

struct pingednode{
  int stat;
  struct in_addr ip;
};

int host_ip(char *hostname, char *ip);
void getmacaddr(char *ip,char *mac);
int sendpackettonext(struct ippacket *packet,int vm_count);
uint16_t in_cksum(uint16_t *addr, int len);
void do_multicast();
void requestPing(int pfsock,struct in_addr srcip,struct in_addr destip);
int areq(struct sockaddr *IPaddr,socklen_t len, struct hwaddr *HWaddr);
void proc_v4(int fd);
int fill_ping_header(struct pingpacket *packet,uint32_t src_ip,uint32_t dst_ip,char *destmac);

#endif