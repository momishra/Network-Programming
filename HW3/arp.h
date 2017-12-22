#ifndef __ARP_H
#define __ARP_H


#include <sys/socket.h>
#include <linux/if_packet.h>
#include <linux/if_arp.h>
#include <stdlib.h>
#include <stdio.h>
#include <ifaddrs.h>
#include<netinet/ip.h>
#include<netinet/ip_icmp.h>
#include "unp.h"
#include "hw_addrs.h"

#define FRAME_TYPE 0x0806
#define PROTO 169
#define IDENT 0x6169
#define PATH_NAME "/tmp/path_arp"

#define MAX_BACKLOG 10

struct arpacket{
  uint16_t id;
  uint16_t h_type; //type of hw address - 1for ethernet
  uint16_t p_type; //protocol address type - 0x0800 for IP 
  uint8_t h_size; //6 
  uint8_t p_size; //4
  uint16_t op; //either request or reply 1 or 2 
  struct in_addr srcip;
  struct in_addr destip;
  char srcmac[6]; //sender hw address 6 byte
  char destmac[6]; //dest hw address 6 byte 
};

struct ether_frame{
  char destmac[6]; //change this -- 6byte
  char srcmac[6]; //6 byte
  uint16_t f_type; //2 byte
  struct arpacket packet;  
};

struct arp_req{
  uint8_t index;
  int head_type;
  int head_len;
  struct in_addr ip;
};

typedef struct arp_entry{
  struct in_addr ip;
  char macaddr[6];
  int ifini;
  int sockfd;
  int h_type;
  int status;
}arp_entry;

typedef struct ip_mac_pair{
  struct in_addr ip;
  char mac[6];
}ip_mac_pair;

arp_entry arp_table[100];

int ethernetpacket(struct ether_frame *frame,struct sockaddr_ll *sockAddr);
int processpacket(struct ether_frame *frame,struct sockaddr_ll *sockAddr);
void sendPacket(struct ether_frame *frame,int ifIndex, uint16_t hw_type,uint8_t hw_len);
void sendARP(struct in_addr *destip, struct arp_req *data,int op);
void updateCache(int ifindex,int h_type,struct in_addr *srcip,char *smac,int sockfd,int flag);
arp_entry *searchARPCache(struct in_addr *ip);
char *ether_ntop(char *addr);
void createarpPacket(struct ether_frame *frame,char *destHWAddr, struct in_addr *destIP,char *srcHWAddr, struct in_addr *srcIP,uint16_t hw_type,uint8_t hw_len,uint16_t opt_type);

#endif