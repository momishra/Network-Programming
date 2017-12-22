#ifndef FUNCS_H
#define FUNCS_H

#include "unprtt.h"
#include "/home/courses/cse533/Stevens/unpv13e_solaris2.10/lib/unpifi.h"
#include "/home/courses/cse533/Stevens/unpv13e_solaris2.10/lib/unp.h"

#define PAYLOAD 495
#define PACKETSIZE 512
#define MAX_RETRANS 5
#define SYN 1
#define SYN_ACK 2
#define SYN_ACK_ACK 3
#define ACK 4
#define DATA 5
#define FIN 6
#define FIN_ACK 7
#define PROBE 8
#define PROBE_ACK 9
#define DUP_ACK 10

#define TIMEND 3000

typedef struct udp_pkt_s{
  uint32_t seqno;
  uint32_t ackno;
  uint32_t window;
  uint32_t flag;
  uint32_t ts; /*timestamp*/
  char buf[PAYLOAD];
}udp_pkt_s;

typedef struct clireqlist{
  struct sockaddr_in cliaddr;
  struct clireqlist *nextcli;
  pid_t pid;
}clireqlist;

typedef struct packetq{ /*this queue is for maintaining the packet sent */
  struct udp_pkt_s packet;
  struct packetq *nextp;
  struct packetq *prevp;
  int size;
  int present;
  int retransmit;
  uint32_t ts; //discard packets exceeding max RTT
}packetq;

typedef struct congestion{
  uint32_t nextno,nextsendno,lastsendno;
  uint32_t seq,ack;
  uint32_t rcvwin,sendwin,congwin,ss;
  struct packetq *start,*curr;
}congestion;

void serverCongestionConf(congestion *serv,int cliWindow,int sendWindow);
//void sendData(int *sockfd,int fd,struct rtt_info rttinfo,congestion *cc);
int initTimer(struct itimerval *timer,long int start);
int ifLocal(int sockfd, struct sockaddr_in *cliaddr,struct sockaddr_in *servaddr);
int get_interfaces(struct ifi_info *ifihead);
#endif
