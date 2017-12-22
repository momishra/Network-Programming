#include "unp.h"
#include "arp.h"
#include <stddef.h>

struct in_addr src_ip_struct;
int arp_cache_size =0;
struct ip_mac_pair pairs[50];
int count =0;
int arp_cache_entries =0;
int packetfd,unixfd;

int get_ip_mac_pairs(){
  struct hwa_info	*hwa, *hwahead;
	struct sockaddr	*sa;
	char   *ptr;
	int    i, prflag;

	printf("\n");
   printf("*************\n");
	for (hwahead = hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next) {
		if(strcmp(hwa->if_name,"lo") != 0){
     
      printf("Interface index = %d and", hwa->if_index);
      struct sockaddr_in *src_ip = (struct sockaddr_in*)hwa->ip_addr;
      pairs[count].ip = src_ip->sin_addr;
      memcpy(pairs[count].mac,hwa->if_haddr,6);
      count++;
      
      printf(" Interface name is %s %s", hwa->if_name, ((hwa->ip_alias) == IP_ALIAS) ? " (alias)\n" : "\n");
      if((sa = hwa->ip_addr) != NULL){
        printf("IP addr = %s : ", Sock_ntop_host(sa, sizeof(*sa)));
      }
    		
		prflag = 0;
		i = 0;
		do {
			if (hwa->if_haddr[i] != '\0') {
				prflag = 1;
				break;
			}
		} while (++i < IF_HADDR);

		if (prflag) {
			printf(" HW addr = ");
			ptr = hwa->if_haddr;
			i = IF_HADDR;
			do {
				printf("%.2x%s", *ptr++ & 0xff, (i == 1) ? " " : ":");
			} while (--i > 0);
		}
      printf("\n\n");
    }
	}
  printf("*************\n");
  free(hwahead);
  return count;
}

void selectsockets(){
  arp_entry *entry;
  fd_set set;
  struct arpacket *arp_packet;
  struct sockaddr_ll lladdr;
  struct ether_frame frame;
  struct arp_req arp_data;
  int tmpfd = -1, maxfdp1;
  struct in_addr dest_ip;
  FD_ZERO(&set);
  FD_SET(unixfd,&set);
  FD_SET(packetfd,&set);
  while(1){
    maxfdp1 = max(unixfd,packetfd);
    FD_SET(packetfd,&set);
    FD_SET(unixfd,&set);
    if(tmpfd !=-1){
      maxfdp1 = max(maxfdp1,tmpfd);
      FD_SET(tmpfd,&set);
    }
    Select(maxfdp1+1,&set, NULL,NULL,NULL);
    if(FD_ISSET(unixfd,&set)){
      printf("something to read on unixfd\n");
      int datalen = sizeof(struct arp_req);
      tmpfd = Accept(unixfd,NULL,NULL);
      int r = read(unixfd,&arp_data,datalen);
      
      dest_ip.s_addr = arp_data.ip.s_addr;
      if((entry = searchARPCache(&dest_ip))!= NULL){
        printf("Entry is present in ARP Cache!\n");
        Writen(tmpfd,entry->macaddr,6);
        close(tmpfd);
        tmpfd =-1;
      }else{
        updateCache(arp_data.index,arp_data.head_type,&(arp_data.ip),NULL,tmpfd,1);
        struct ether_frame reqpacket;
        char dst_hw[IF_HADDR] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
        createarpPacket(&reqpacket,dst_hw,&dest_ip,pairs[0].mac,&(pairs[0].ip),arp_data.head_type,arp_data.head_len,1);
        sendPacket(&reqpacket, arp_data.index,arp_data.head_type,arp_data.head_len);
      }
    }
    
   if(FD_ISSET(packetfd,&set)){
     printf("something to read on packet socket\n");
     if(ethernetpacket(&frame,&lladdr) == 1){
     
       int result = processpacket(&frame,&lladdr);
       if(result = 0){ 
         close(tmpfd);
         tmpfd = -1;
        }
     }
    }
    
    if( tmpfd!= -1 && FD_ISSET(tmpfd,&set)){
      if((entry = searchARPCache(&dest_ip))!=NULL){
        char buf[40];
        entry->status = 0;
        printf("ARP cached entry for %s is invalidated\n",inet_ntop(AF_INET,&dest_ip,buf,INET_ADDRSTRLEN));
      }
    }
    
  }

}

int ethernetpacket(struct ether_frame *frame,struct sockaddr_ll *sockAddr){
  printf("coming in ethernetpacket()");
  size_t frameLen;
  socklen_t sockLen;
  sockLen = sizeof(struct sockaddr_ll);
  frameLen= sizeof(struct ether_frame);
  memset(sockAddr,'\0',sockLen);
  memset(frame, '\0',frameLen);
  
  if (recvfrom(unixfd, frame, frameLen, 0, (struct sockaddr *) sockAddr, &sockLen) < 0){
       		perror("Recv From");
           return 0;
    }
  printf("\nReceived Ethernet Packet.......\n");
	  char src_ip[20];
   char dest_ip[20];
   char *dest_add = ether_ntop(frame->destmac);
   char *src_add = ether_ntop(frame->srcmac);

    printf ("\nEthernet frame :\n");
    printf ("\nDest MAC: %s  Src MAC %s", dest_add,src_add);
    if (frame->packet.op == 1)
        printf("\n ARP Request Message ");
    else
        printf("\n ARP Reply Message ");

    printf(" ID: %x", frame->packet.id);
    printf(" Protocol Number: %x", frame->packet.p_type);
    printf(" Source IP Address: %s",inet_ntop(AF_INET,&(frame->packet.srcip),src_ip,INET_ADDRSTRLEN));
    printf("\tSrcMAC: %s", src_add);
    printf("\tDestIP: %s",inet_ntop(AF_INET,&(frame->packet.destip),dest_ip,INET_ADDRSTRLEN));
    printf("\tDestMAC: %s \n", dest_add);
    return 1;
}

void createarpPacket(struct ether_frame *frame,char *destHWAddr, struct in_addr *destIP,char *srcHWAddr, struct in_addr *srcIP,uint16_t hw_type,uint8_t hw_len,uint16_t opt_type){
   size_t frameLen=sizeof(struct ether_frame);
    memset(frame, '\0',frameLen);
    memcpy(frame->destmac, destHWAddr, 6);
    memcpy(frame->srcmac, srcHWAddr, 6);
    frame->f_type = htons(FRAME_TYPE);//doubt
    struct arpacket *arpPck;
    arpPck= &(frame->packet);

    arpPck->id = IDENT;
    arpPck->h_type = hw_type;
    arpPck->p_type = PROTO;
    arpPck->h_size = hw_len;
    arpPck->p_size = sizeof(PROTO);
    arpPck->op   = opt_type;
    arpPck->srcip.s_addr = srcIP->s_addr;
    arpPck->destip.s_addr = destIP->s_addr;
    memcpy(arpPck->srcmac, srcHWAddr, 6);
    memcpy(arpPck->destmac, destHWAddr,6);
}

void sendPacket(struct ether_frame *frame,int ifIndex, uint16_t hw_type,uint8_t hw_len){
    struct sockaddr_ll sockAddr;
    size_t sockLen=sizeof(struct sockaddr_ll);
    size_t frameLen=sizeof(struct ether_frame);
    memset(&sockAddr,'\0',sockLen);

    sockAddr.sll_family   = PF_PACKET;
    sockAddr.sll_halen    = ETH_ALEN;
    sockAddr.sll_ifindex  = 2;
    sockAddr.sll_addr[0] = frame->destmac[0];
    sockAddr.sll_addr[1] = frame->destmac[1]; 
    sockAddr.sll_addr[2] = frame->destmac[2];
    sockAddr.sll_addr[3] = frame->destmac[3];
    sockAddr.sll_addr[4] = frame->destmac[4];
    sockAddr.sll_addr[5] = frame->destmac[5];
    sockAddr.sll_addr[6] = 0x00;
    sockAddr.sll_addr[7] = 0x00;
    int s = sendto(packetfd, (void *)frame, frameLen, 0,(struct sockaddr *) &sockAddr, sockLen);
}


int processpacket(struct ether_frame *frame,struct sockaddr_ll *sockAddr){
 int i;
 char destHWAddr[6];
  int flag=0;
	memset(destHWAddr,'\0',IF_HADDR);
  struct arp_entry *srcRow;
	struct ether_frame repPck;
   if (frame->packet.op == 1){
  	printf("\nARP REQUEST received..");
     for (i = 0; i < count; i++){
     if (frame->packet.destip.s_addr==pairs[i].ip.s_addr) {
				memcpy(destHWAddr,pairs[i].mac,6);
				flag =1;
        break;
			}
    }
    if(flag == 1){
      uint16_t hw_type = sockAddr->sll_hatype;
			uint8_t hw_len = sockAddr->sll_halen; 
			struct in_addr destIP;
			destIP.s_addr = frame->packet.srcip.s_addr;
     updateCache(sockAddr->sll_ifindex,sockAddr->sll_hatype,&(frame->packet.srcip),frame->srcmac,0,1);

    struct ether_frame repPck;
			memset(&repPck,'\0',sizeof(struct ether_frame));
			createarpPacket(&repPck,frame->srcmac,&destIP,pairs[0].mac,&(pairs[0].ip), hw_type, hw_len,1);
   sendPacket(&repPck, sockAddr->sll_ifindex, hw_type,hw_len);
	}else {
   updateCache(sockAddr->sll_ifindex,sockAddr->sll_hatype,&(frame->packet.srcip),frame->srcmac, 0, 0);
		}
  }
   else if (frame->packet.op == 2){
     int sockfd;
     srcRow= searchARPCache(&(frame->packet.srcip));
  	  sockfd = srcRow->sockfd;
	    if (sockfd > 0)
			  Writen(sockfd,frame->srcmac,6);

      updateCache(sockAddr->sll_ifindex,sockAddr->sll_hatype,&(frame->packet.srcip),frame->srcmac, 0, 0);
    }
    return frame->packet.op;
}


int main(int argc, char **argv){
  //using get_hw_addrs get all the interface information and printf it on stdout
  get_ip_mac_pairs();
  printf("Count of <IP,HW> pairs : %d", count);
  struct sockaddr_un arpserv, arpcli;
  ssize_t size;
  
  //create packet socket 
   packetfd = socket(PF_PACKET,SOCK_RAW,htons(PROTO));
   if(packetfd <0){
     printf("Error creating packet socket\n");
     return 0;
   }
   //create UNIX domain socket - listening socket bound to PATH_NAME
   unixfd = socket(AF_LOCAL,SOCK_STREAM,0);// for communication with areq
   if(unixfd <0){
     printf("Error in creating unix socket\n");
     return 0;
   }
   
   bzero(&arpserv,sizeof(struct sockaddr_un));
   arpserv.sun_family = AF_LOCAL;
   strcpy(arpserv.sun_path,PATH_NAME);
   unlink(PATH_NAME);
   
  Bind(unixfd,(SA*)&arpserv,sizeof(struct sockaddr_un));
  
  Listen(unixfd, MAX_BACKLOG);
  selectsockets();
  
  close(packetfd);
  close(unixfd);
}

arp_entry *searchARPCache(struct in_addr *ip){
  for(int i=0;i<arp_cache_entries;i++){
    if(arp_table[i].status && ip->s_addr == arp_table[i].ip.s_addr){
      return &arp_table[i];
    }
  }

  return NULL;
}

void updateCache(int ifindex,int h_type,struct in_addr *srcip,char *smac,int sockfd,int flag){
  int i, row;
	row = 0;
   if(flag==1)
     row = arp_cache_entries;
    for (i = 0; i < arp_cache_entries; i++){
        if (arp_table[i].status) {
            if(arp_table[i].ip.s_addr == srcip->s_addr) {
                row = i;
                break;
            }
        }

    }

    if ((flag ==1) || (row != arp_cache_entries)){
	    arp_table[row].ip.s_addr = srcip->s_addr;
      arp_table[row].ifini = ifindex;
      arp_table[row].h_type  = h_type;
	  if (sockfd != -1)
        	arp_table[row].sockfd  = sockfd;
          arp_table[row].status = 1;
	  if (smac != NULL)
        	memcpy(arp_table[row].macaddr, smac, 6);
     if (row == arp_cache_entries)
            arp_cache_entries++;
        printf("\nCache Updated\n");
    }
}


void sendARP(struct in_addr *destip, struct arp_req *data,int op){
  struct ether_frame frame;
  struct sockaddr_ll lladdr;
  size_t framelen = sizeof(struct ether_frame);
  size_t socklen = sizeof(struct sockaddr_ll);
  bzero(&frame,framelen);
  char tmphw[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
  
  memcpy(frame.destmac,tmphw,6);
  memcpy(frame.srcmac,pairs[0].mac,6);
  frame.f_type = htons(PROTO);
  struct arpacket *arp_pack = &(frame.packet);
  
  arp_pack->id = IDENT;
  arp_pack->h_type = data->head_type;
  arp_pack->p_type = PROTO;
  arp_pack->h_size = data->head_len;
  arp_pack->p_size = sizeof(PROTO);
  arp_pack->op = op;
  arp_pack->srcip.s_addr = pairs[0].ip.s_addr;
  arp_pack->destip.s_addr = destip->s_addr;
  memcpy(arp_pack->srcmac,pairs[0].mac,6);
  memcpy(arp_pack->destmac,tmphw,6);
  
  bzero(&lladdr,socklen);
  lladdr.sll_family = PF_PACKET;
  lladdr.sll_family   = PF_PACKET;
  lladdr.sll_halen    = ETH_ALEN;
  lladdr.sll_ifindex  = 2;
  lladdr.sll_addr[0] = frame.destmac[0];
  lladdr.sll_addr[1] = frame.destmac[1]; 
  lladdr.sll_addr[2] = frame.destmac[2];
  lladdr.sll_addr[3] = frame.destmac[3];
  lladdr.sll_addr[4] = frame.destmac[4];
  lladdr.sll_addr[5] = frame.destmac[5];
  lladdr.sll_addr[6] = 0x00;
  lladdr.sll_addr[7] = 0x00;
  Sendto(packetfd, &frame, framelen, 0,(struct sockaddr *) &lladdr, socklen);
}

char *ether_ntop(char *addr){
   char buf[10];
   char *temp ;
   int i;
   temp[0] = '\0';
   for (i = 0; i < 6; i++) {
        sprintf(buf, "%.2x%s", addr[i] & 0xff , i == 5 ? "" : ":");
        strcat(temp, buf);
    }
     int len = strlen(temp);
     temp[len] = '\0';
     return temp;
}
