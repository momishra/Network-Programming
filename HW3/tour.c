#include "tour.h"
#include<stddef.h>

struct in_addr src_ip_struct;
struct in_addr ip_list[20];
char srcmac[6];
int isEnd = 0,isPing =0,numOfPings = 5,isExit=0;
int rt,pg,udp,pf,ip_count=0;
struct in_addr multisock;
char multi_data[1024] = "MULTICAST MSG :: Hello Let's form a GANG and BUG others!!";
static int multi_mem =0;
static int sent_msg =0;
int alreadyPinged[15] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
struct in_addr pingedAddrs[15];

void getaddrs(char *ip,char *mac){
  struct ifaddrs *ifaddr, *ifa;
  int family, s, n;
  char host[NI_MAXHOST];
  if (getifaddrs(&ifaddr) == -1) {
    perror("getifaddrs");
  }
  for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {
    if (ifa->ifa_addr == NULL)
      continue;
      family = ifa->ifa_addr->sa_family;
      if (family == AF_INET && !(ifa->ifa_flags & IFF_LOOPBACK)) {
        s = getnameinfo(ifa->ifa_addr,sizeof(struct sockaddr_in),host, NI_MAXHOST,NULL, 0, NI_NUMERICHOST);
        if (s != 0) {
          printf("getnameinfo() failed: %s\n", gai_strerror(s));
          exit(EXIT_FAILURE);
          }
        //printf("\t\taddress: <%s>\n", host);
        strcpy(ip,host);
        }
        if(family == AF_PACKET){ 
        struct sockaddr_ll *s = (struct sockaddr_ll*)ifa->ifa_addr;
        if(s->sll_ifindex==2){
          int i;
            int len = 0;
            for(i = 0; i < 6; i++)
                len+=sprintf(mac+len,"%02X%s",s->sll_addr[i],i < 5 ? ":":""); 
        }
        }
      }
      freeifaddrs(ifaddr);
    }


int host_ip(char *hostname, char *ip){
	struct hostent *host;
	struct in_addr **addr_list;
	int i;
	
	if((host = gethostbyname(hostname))== NULL){
		perror("No such host exist!");
		return 1;
	}
	addr_list = (struct in_addr **) host->h_addr_list;
	for(i=0;addr_list[i] != NULL; i++){
		strcpy(ip, inet_ntoa(*addr_list[i]));
    printf("%s IP : %s\n",hostname,ip);
	}
	return 1;
}


int main(int argc, char **argv){
  printf("\n********Startin tour application********\n");
	
  //creating list of IP addresses - START
  int ret,len,status;
	char localname[10],my_ip[4],srcip[4];
  struct sockaddr_ll packetaddr;
  fd_set set,sel;
  int maxfdp1;
  struct timeval p_timeout;
  struct sockaddr_in multisockaddr;
  
  inet_pton(AF_INET,MULTICAST_IP,&multisock);
	localname[10] ='\0';
	if((ret = gethostname(localname,10))<0){
    printf("Name of the host system - not found\n");
  }

  getaddrs(srcip,srcmac);
  
  printf("Source IP : %s\n",srcip);
  printf("Source MAC :%s and interface index is 2\n",srcmac);
  
  inet_pton(AF_INET,srcip,&src_ip_struct);
  ip_list[ip_count++] = src_ip_struct;
  
	int i=1;
	for(i=1;i<argc;i++){
		char *hostname = argv[i];
   struct in_addr ipaddr;
   
		if((strncmp(hostname,localname,4)) == 0){
			printf("Error : Source VM can't be entered\n");
			return(-1);
		} 
		if((strncmp(hostname,argv[i-1],4)) == 0){
			printf("Error : Consecutive entries of same VM can't be entered\n");
			return(-1);
		}
		host_ip(hostname,my_ip);
    if(my_ip == NULL){
      printf("IP is not available\n");
      return(-1);
    }
    inet_pton(AF_INET,my_ip,&ipaddr);
    ip_list[ip_count++] = ipaddr;
	}
   
    //creating list of IP addresses - END
    printf("Number of IP's in list is %d\n",ip_count);
    
    //creating Sockets - START 
    rt = socket(AF_INET,SOCK_RAW,PROTO_ID); //169 id/protocol
    if(rt < 0){
      printf("Unable to create RT socket\n");
      return 0;
    }
    
    pg = socket(AF_INET,SOCK_RAW,IPPROTO_ICMP);
     if(pg <0){
       printf("Error creating PG socket\n");
       return 0;
     }
     
    udp = socket(AF_INET,SOCK_DGRAM,0);
     if(udp <0){
       printf("Error creating UDP socket\n");
       return 0;
     }
    
    pf = socket(PF_PACKET,SOCK_RAW,htons(ETH_P_IP));
     if(pf <0){
       printf("Error creating PF socket\n");
       return 0;
     }
     
    int one =1;
	  
    if(setsockopt(rt, IPPROTO_IP, IP_HDRINCL,&one,sizeof(one)) < 0){
	  	printf("Error setting IP_HDRINCL. Eror number %d. Error Message %s \n", errno,   strerror(errno));
	  	exit(0);
	  }  
     setsockopt(udp, SOL_SOCKET,SO_REUSEADDR,&one,sizeof(one));
    
     if(argc <=1){
       printf("Starting tour..\n");
     //  return 0;
     }else{
       do_multicast();
       struct ippacket *tourp;
       tourp = (void*)malloc(sizeof(struct ippacket));
       tourp->ip = multisock;
       tourp->port = MULTICAST_PORT;
       tourp->index =0;
       tourp->l_index = ip_count;
       memcpy(tourp->payload,ip_list,sizeof(ip_list));
     //send this packet to next node
     if(ip_count !=0) sendpackettonext(tourp,ip_count);
   }
  
   FD_ZERO(&set);
   
   while(1){//while- START
     FD_SET(rt,&set);
     maxfdp1 = rt;   
     if(multi_mem == 1){//if multicasting is enabled you have to multiplex on udp socket too
       FD_SET(udp,&set);
       maxfdp1 = max(rt,udp);
     }
     
     if(isPing==1 || isExit ==1){
       FD_SET(pg,&set);
       maxfdp1 = max(maxfdp1,pg);
       p_timeout.tv_sec =5;
       p_timeout.tv_usec =0;
       if(isExit ==1){
         p_timeout.tv_sec = 5;
         p_timeout.tv_usec =0;
         isPing =1;
       }
     }
   maxfdp1 = maxfdp1 +1;
   int status = select(maxfdp1,&set,NULL,NULL,isPing?&p_timeout:NULL);
   if(status ==0) {
     if(isExit == 1) exit(0);
     if(isEnd == 1){
       if(numOfPings >=0) numOfPings --;
     }
     
     if(numOfPings == 0){
       isPing = 0;
       //stop multicasting 
     struct sockaddr_in multicast_socket;
      char buf[256];
     multicast_socket.sin_family = AF_INET;
    multicast_socket.sin_addr.s_addr  = multisock.s_addr;     
    sent_msg = 1;
    multicast_socket.sin_port = htons(MULTICAST_PORT);
    memset(buf,'\0',100);
    int s = sendto(udp,(void *)buf,100,0,(struct sockaddr *)&multicast_socket,sizeof(struct sockaddr));
  isExit = 1;
  printf("Done multicasting\n");
   }else{
     struct in_addr pingdest;
     for(int i=1;i< 11; i++){
       if(alreadyPinged[i] == 1){
         pingdest.s_addr = pingedAddrs[i].s_addr;
         requestPing(pf,src_ip_struct,pingdest);
       }
     }
   }
   } //s==0
   else if(FD_ISSET(rt,&set)){
     printf("something to read on RT socket\n");
     struct ippacket tour_packet;
     int len = sizeof(struct ippacket);
     int ret = recvfrom(rt,(void*)&tour_packet,len,0,NULL,NULL);
     //check the identification field of the packet 
     do_multicast();
     int prevind = tour_packet.index -1;
     struct in_addr dest;
     dest.s_addr = tour_packet.payload[prevind].s_addr;
     printf("\n Received source routing packet from %s\n",inet_ntoa(dest));
     isPing =1;
     struct hostent *host = gethostbyaddr(&dest,sizeof(dest),AF_INET);
     char *vm_name = host->h_name;
     vm_name++;
     vm_name++;
     int vmno = atoi(vm_name);
     alreadyPinged[vmno] = 1;
     pingedAddrs[vmno].s_addr = tour_packet.ip.s_addr;
     requestPing(pf,src_ip_struct,dest);
     if(tour_packet.index == tour_packet.l_index){
       isEnd =1;
       continue;
     }
     sendpackettonext(&tour_packet,ip_count);
   }else if(FD_ISSET(udp,&set)){
     printf("something to read on UDP socket\n");
       char msg[1024];
       int ret = recvfrom(udp,msg,sizeof(msg),0,NULL,NULL);
       if(sent_msg == 0){ //no msg yet sent 
         multisockaddr.sin_family = AF_INET;
         multisockaddr.sin_addr.s_addr = multisock.s_addr;
         multisockaddr.sin_port = htons(MULTICAST_PORT);
         int status = sendto(udp,(void*)multi_data,sizeof(multi_data),0,(struct sockaddr *)&multisockaddr,sizeof(struct sockaddr));
         sent_msg = 1;
         isEnd =1;
         isPing = 0;
         isExit =1;
         continue;
       }
     }
     else if(FD_ISSET(pg,&set)){
       proc_v4(pg);
     }
   }//while - END
	return 1;	
}

int sendpackettonext(struct ippacket *packet,int vm_count){
  uint32_t srcip = src_ip_struct.s_addr;
  struct sockaddr_in *destvm = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));

  struct ip *header;
  header = (struct ip*)&(packet->header);
  packet->index++; //updating the list
  struct in_addr temp_addr;
  temp_addr.s_addr = packet->payload[packet->index].s_addr; //get next node's ip
  
  struct hostent *host = gethostbyaddr(&temp_addr,sizeof(temp_addr),AF_INET);
  if(host->h_name!=NULL) printf("Next vm to be accessed is %s\n",host->h_name);
  while(host->h_name == NULL){
    printf("Next vm is not accesssible\n");
    if(packet->index == packet->l_index){
      printf("End the tour..last node , that's why next node is not accessible");
      isEnd = 1;
      return 1;
    }
   packet->index++; //now again update the list -- as previous node is not accessible 
  }
  
  printf("IP of next vm node is %s",inet_ntoa(packet->payload[packet->index]));
  destvm->sin_family = AF_INET;
  destvm->sin_addr.s_addr = packet->payload[packet->index].s_addr; //this is final
  int size = sizeof(struct ippacket);
  header->ip_p = PROTO_ID; 
  header->ip_ttl = 1;
  header->ip_id = htons(IDENT);
  header->ip_len = htons(size);
  header->ip_off = 0;
  header->ip_src.s_addr = srcip;
  header->ip_dst.s_addr = packet->payload[packet->index].s_addr; 
  header->ip_v = IPVERSION;
  header->ip_tos = 0;
  header->ip_hl = sizeof(struct ip) >> 2;
  header->ip_sum = in_cksum((uint16_t*)packet,size);
  
  int status = sendto(rt, packet, size,0,(struct sockaddr*)destvm,sizeof(struct sockaddr));
  if(status <=0) perror("Sendto error\n");
  printf("Done sending packet to rt socket\n");
	return size;
}

void do_multicast(){
  struct sockaddr_in multisockaddr;
  struct ip_mreq multi_req;
    if(multi_mem != 1){ //already present in the multicast group
      printf("Adding to the multicast group\n");
      int one =1;
      int two =1;
      multisockaddr.sin_family = AF_INET;
      multisockaddr.sin_port = htons(MULTICAST_PORT);
      multisockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
      //binding multicast address with udp socket
      if((bind(udp,(struct sockaddr*)&multisockaddr, sizeof(multisockaddr))) < 0){
        perror("Error binding multicast with udp socket\n");
      }
      setsockopt(udp,IPPROTO_IP,IP_MULTICAST_TTL,&one,sizeof(one));
      setsockopt(udp,IPPROTO_IP,IP_MULTICAST_LOOP,&two,sizeof(two));
      multisockaddr.sin_addr.s_addr = multisock.s_addr;
      multi_req.imr_multiaddr.s_addr = multisock.s_addr;
      multi_req.imr_interface.s_addr = src_ip_struct.s_addr;
      
      int status = setsockopt(udp,IPPROTO_IP,IP_ADD_MEMBERSHIP,(const void *)&multi_req,sizeof(struct ip_mreq ));
	    multi_mem = 1;
  } else{
    printf("The node is already in multicast group\n");
  }  
}


void requestPing(int pfsock,struct in_addr srcip,struct in_addr destip){
  //before pinging do arp request to get the hardware address of the destination system
  struct sockaddr_in *IPaddr = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
  IPaddr->sin_family = AF_INET;
  IPaddr->sin_port = 0;
  IPaddr->sin_addr.s_addr = destip.s_addr;
  
  struct hwaddr *hw_info = (struct hwaddr *)malloc(sizeof(struct hwaddr));
  hw_info->sll_ifindex =2; //may be this is creating problem?
  hw_info->sll_hatype = ARPHRD_ETHER;
  hw_info->sll_halen = ETH_ALEN;
  
  printf("Calling AREQ to get dest MAC address..\n");
  
  int arp_ret = areq((struct sockaddr*)IPaddr,sizeof(struct sockaddr),hw_info);
  printf("done AREQ\n");
  //now fill up ping packet
  struct pingpacket packet;
  bzero(&packet,sizeof(struct pingpacket));
  struct pingheader pph;
  char destmac[6];
  char src_mac[6];
  memcpy(destmac,hw_info->sll_addr,6);
  memcpy(src_mac,srcmac,6);
  
  //fill ping header
  fill_ping_header(&packet,srcip.s_addr,destip.s_addr,destmac);
  
  struct sockaddr_ll sockaddr;
  sockaddr.sll_family = PF_PACKET;
  sockaddr.sll_halen    = ETH_ALEN;
  sockaddr.sll_ifindex  = 2;
  sockaddr.sll_addr[0] = destmac[0];
  sockaddr.sll_addr[1] = destmac[1];
  sockaddr.sll_addr[2] = destmac[2];
  sockaddr.sll_addr[3] = destmac[3];
  sockaddr.sll_addr[4] = destmac[4];
  sockaddr.sll_addr[5] = destmac[5];
  sockaddr.sll_addr[6] = 0x00;
  sockaddr.sll_addr[7] = 0x00;
  printf("Sending ping request\n");
  Sendto(pf,&packet,sizeof(packet),0,(struct sockaddr *)&sockaddr,sizeof(struct sockaddr_ll));
}


int areq(struct sockaddr *IPaddr,socklen_t len, struct hwaddr *HWaddr){
  struct sockaddr_un namesock;
  int sock;
  size_t size;
  char macbuf[INET_ADDRSTRLEN];
  
  
  bzero(&namesock,sizeof(struct sockaddr_un));
  memset(&namesock,'\0',sizeof(struct sockaddr_un));
  
  sock = socket(AF_LOCAL,SOCK_STREAM,0);
  if(sock <0){
    perror("ERROR creating named socket\n");
    exit(EXIT_FAILURE);
  }
  namesock.sun_family = AF_LOCAL;
  
  strcpy(namesock.sun_path,PATH_NAME);
  
  Connect(sock, (struct sockaddr*)&namesock,sizeof(struct sockaddr_un)); //its throwing error
  
  printf("Connected to named socket\n");
  struct arp_req req;
  req.index = HWaddr->sll_ifindex; //ethernet index
  req.head_type = HWaddr->sll_hatype;
  req.head_len = HWaddr->sll_halen;
  req.ip.s_addr = ((struct sockaddr_in*)IPaddr)->sin_addr.s_addr;
  char ip_buf[4];
  printf("\n AREQ requested Hardware Address of %s",inet_ntop(AF_INET,&(req.ip),ip_buf,INET_ADDRSTRLEN));

  int s = write(sock,&req,sizeof(struct arp_req)); //arp reuqest
  if(s < 0) perror("ARP Request error\n");
  fd_set areq;
  struct timeval timeout;
  FD_ZERO(&areq);
  FD_SET(sock,&areq);
  timeout.tv_sec = 5;
  timeout.tv_usec = 0;
  
  Select(sock+1, &areq, NULL,NULL,&timeout);
  
  char rcvbuf[6];
  int r = Read(sock,rcvbuf,6);
  memcpy(HWaddr->sll_addr,rcvbuf,6);
  char *hw_buf= ether_ntop(HWaddr->sll_addr);
  printf("\n Received Hardware address is %s\n",hw_buf);
  close(sock);
  return 1;
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


uint16_t in_cksum(uint16_t *addr, int len)
{
         int nleft = len;
         uint16_t *w = addr;
         uint32_t sum = 0;
        uint16_t answer = 0;

       /* 
         * Our algorithm is simple, using a 32 bit accumulator (sum), we add
         * sequential 16 bit words to it, and at the end, fold back all the
         * carry bits from the top 16 bits into the lower 16 bits.
         */
        while (nleft > 1)  {
                sum += *w++;
                nleft -= 2;
        }

        /* mop up an odd byte, if necessary */
        if (nleft == 1) {
                *(uint8_t *)(&answer) = *(uint8_t *)w ;
                sum += answer;
        }

        /* add back carry outs from top 16 bits to low 16 bits */
        sum = (sum >> 16) + (sum & 0xffff);     /* add hi 16 to low 16 */
        sum += (sum >> 16);                     /* add carry */
        answer = ~sum;                          /* truncate to 16 bits */
        return (answer);
}
void proc_v4(int fd){
  struct pingheader packet;
  int  hlen1, icmplen;
  double rtt;
  struct ip *ip;
  struct icmp *icmp;
  struct timeval  *tvsend,*tvrecv;
  char buf[INET_ADDRSTRLEN];
  tvrecv = (struct timeval *)malloc(sizeof(struct timeval));
  Gettimeofday(tvrecv,NULL);
	static int nsent = 1;
  int rv = recvfrom(fd,&packet,sizeof(packet),0,NULL,NULL);
  if (rv < 0)
    perror("Error in Recvfrom");
	icmplen = rv;
 ip = &(packet.iph);
 icmp = &(packet.icmp_ph);

  hlen1 = ip->ip_hl << 2;
  if (ip->ip_p != IPPROTO_ICMP)
    return;
  if (icmp->icmp_type == ICMP_ECHOREPLY) {
    tvsend = (struct timeval *) icmp->icmp_data;
    tv_sub(tvrecv, tvsend);
    rtt = tvrecv->tv_sec * 1000.0 + tvrecv->tv_usec / 1000.0;
    printf("\n PING REPLY %d bytes from %s: seq=%u, ttl=%d, rtt=%.3f ms\n",
    icmplen,inet_ntop(AF_INET,&(ip->ip_src),buf,INET_ADDRSTRLEN),ntohs(icmp->icmp_seq), ip->ip_ttl, rtt);
	}
}

int fill_ping_header(struct pingpacket *packet,uint32_t src_ip,uint32_t dst_ip,char *destmac) {

        int   len;
        struct icmp     *icmp_hdr;
        static int nsent = 1;
        memcpy(packet->destmac,destmac,6);
        memcpy(packet->srcmac, srcmac,6);
        packet->protocol = htons(ETH_P_IP);

        icmp_hdr = &(packet->pph.icmp_ph);

        icmp_hdr->icmp_type = ICMP_ECHO;
        icmp_hdr->icmp_code = 0;
        icmp_hdr->icmp_id = htons(IDENT);
        icmp_hdr->icmp_seq = htons(++nsent);
        Gettimeofday((struct timeval *) icmp_hdr->icmp_data, NULL);
        len = sizeof(struct icmp);
        icmp_hdr->icmp_cksum = 0;
        icmp_hdr->icmp_cksum = in_cksum((uint16_t *) icmp_hdr, len);


        struct ip *ip_hdr ;
        ip_hdr = (struct ip*)&(packet->pph.iph);

        ip_hdr->ip_hl  = sizeof(struct ip) >> 2;
        ip_hdr->ip_v = IPVERSION;
        ip_hdr->ip_tos = 0;
        ip_hdr->ip_len = htons(sizeof(struct pingheader));
        ip_hdr->ip_id  = htons(IDENT);
        ip_hdr->ip_off = 0;
        ip_hdr->ip_ttl = 64;
        ip_hdr->ip_p   = IPPROTO_ICMP;
        ip_hdr->ip_src.s_addr = src_ip;
        ip_hdr->ip_dst.s_addr = dst_ip;
        int size = sizeof(struct ip);
        ip_hdr->ip_sum = in_cksum((uint16_t *)ip_hdr,size);

        return 1;

}