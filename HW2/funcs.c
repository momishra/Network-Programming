#include "funcs.h"
#include "/home/courses/cse533/Stevens/unpv13e_solaris2.10/lib/unp.h"
#include "/home/courses/cse533/Stevens/unpv13e_solaris2.10/lib/unpifi.h"
#include <stdio.h>
#include <string.h>
#include <net/if.h>

struct sockaddr_in *getNetMaskAddress(int sockfd, char *flag){
  struct ifreq ifr;
  struct sockaddr_in *mask;
  ifr.ifr_addr.sa_family = AF_INET;
  strncpy(ifr.ifr_name,flag,IFNAMSIZ-1);
  if(ioctl(sockfd, SIOCGIFNETMASK, &ifr)==0){
    mask = (struct sockaddr_in*)(&ifr.ifr_addr);
  }
  return mask;
}

int ifLocal(int sockfd, struct sockaddr_in *cliaddr,struct sockaddr_in *servaddr){
	int islocal = 0;
	int isSame = 0;
	struct ifi_info *ifi;
	struct ifi_info *ifiClient, *ifiClientNew;
	struct sockaddr_in *mask,*localaddr,client,server;
	unsigned long temp =0,tempmask =0,longestnetmask=0,longestnetmaskdiff=0;
	char* loopBack = "127.0.0.1";
	unsigned long loopBackAddress =0;
	inet_pton(AF_INET, loopBack, &loopBackAddress);

  for(ifi = Get_ifi_info(AF_INET, 1);  ifi != NULL; ifi=ifi->ifi_next){
    mask = getNetMaskAddress(sockfd,ifi->ifi_name);
    localaddr = ((struct sockaddr_in *)ifi->ifi_addr);
   
    if((servaddr->sin_addr.s_addr & mask->sin_addr.s_addr) == (localaddr->sin_addr.s_addr & mask->sin_addr.s_addr)){
			islocal = 1;
			temp = mask->sin_addr.s_addr;
			if(temp > longestnetmask)
			{
				longestnetmask = temp;
				ifiClient = ifi;
			}
			if(servaddr->sin_addr.s_addr == localaddr->sin_addr.s_addr)
			{
				isSame = 1;
			}
    }else{
			tempmask = mask->sin_addr.s_addr;
			if(tempmask > longestnetmaskdiff)
			{
				longestnetmaskdiff = temp;
				ifiClientNew = ifi;
			}
    }
  }

	if(isSame == 1){
    localaddr->sin_addr.s_addr = loopBackAddress;
    client.sin_addr.s_addr = loopBackAddress;
		printf("Server is loopback address.\n");
	}else if(islocal == 1){
		localaddr = (struct sockaddr_in *)ifiClient->ifi_addr;
		client.sin_addr = localaddr->sin_addr;
		printf("Server is on local network.\n");
	}else{
		localaddr = ((struct sockaddr_in *)ifiClientNew->ifi_addr);
		client.sin_addr = localaddr->sin_addr;
		printf("Server is on different network.\n");
	}
	*cliaddr = client;
	free_ifi_info(ifi);
   return islocal;
}

int get_interfaces(struct ifi_info *ifihead){
	struct ifi_info *ifi;
	struct sockaddr *sa;
	u_char *ptr;
	int i, count=0;
       
	for(ifi = Get_ifi_info(AF_INET,1); ifi != NULL; ifi = (struct ifi_info *)ifi->ifi_next){
	  printf("Interface %s : ", ifi->ifi_name);

	  if(ifi->ifi_index != 0)
	    printf("%d ",ifi->ifi_index); 
	  printf("<");  
	   
	  if(ifi->ifi_flags & IFF_UP) printf(" Connected and supports ");
	  if(ifi->ifi_flags & IFF_BROADCAST) printf(" BCAST ");
	  if(ifi->ifi_flags & IFF_MULTICAST) printf(" MCAST ");
	  if(ifi->ifi_flags & IFF_LOOPBACK) printf(" LOOP ");
	  if(ifi->ifi_flags & IFF_POINTOPOINT) printf(" P2P ");
	  printf(">\n");
     
	  if ((i = ifi->ifi_hlen) > 0) {
	    ptr = ifi->ifi_haddr;
	    do {
	      printf("%s%x",(i==ifi->ifi_hlen) ? " " : ":",*ptr++);
	    } while (--i > 0);
	    printf("\n");
	  }
	  if (ifi->ifi_mtu != 0){
	    printf("   MTU:%d \n",ifi->ifi_mtu);
	  }
     
	  if((sa = ifi->ifi_addr ) != NULL){
	    printf(" IP address : %s\n",Sock_ntop_host(sa,sizeof(*sa)));
	  }
	  if((sa = ifi->ifi_brdaddr) != NULL)
	    printf(" Broadcast address : %s\n",Sock_ntop_host(sa,sizeof(*sa)));
	  if((sa = ifi->ifi_dstaddr) != NULL)
	    printf(" Destination addr : %s \n",Sock_ntop_host(sa,sizeof(*sa)));
	  count = count +  1;
	}
  free_ifi_info(ifi);	
	return count;
}

int initTimer(struct itimerval *timer,long int start){
        timer->it_interval.tv_sec = 0;
        timer->it_interval.tv_usec = 0;
        timer->it_value.tv_sec = start/1000;
        timer->it_value.tv_usec = (start % 1000)*1000;
	      setitimer(ITIMER_REAL,timer,0);
}
