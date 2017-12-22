#include "tour.h"

void getipaddr(char *ip,char *mac){
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
  printf("********Startin tour application********\n");
  struct hwa_info	*hwa, *hwahead;
	struct sockaddr	*sa;
  struct sockaddr_in *addr;
	char   *ptr;
   char srcip[4],srcmac[6];
   long my_ip;
	int    i, prflag,status,if_index;
	struct sockaddr_un servaddr;
  /*
  //creating list of IP addresses - START
  int ret,len,status;
	char localname[10],srcmac[6];
  struct sockaddr_ll packetaddr;
	localname[10] ='\0';
	if((ret = gethostname(localname,10))<0){
    printf("Name of the host system - not found\n");
  }
	struct addrlist *head;
	head = (struct addrlist*)malloc(sizeof(struct addrlist));
  getipaddr(head->ip,srcmac);
  printf("Source IP : %s\n",head->ip);
  printf("Source MAC :%s\n",srcmac);
  head->next = NULL;
	
	int i=1;
	for(i=1;i<argc;i++){
		char *hostname = argv[i];
		struct addrlist *node = (struct addrlist*)malloc(sizeof(struct addrlist));
		host_ip(hostname,node->ip);
    node->next = NULL;
    if(head->next == NULL){
      head->next = node;
    }else{
      struct addrlist* temp = head;
      while(1){
        if(temp->next==NULL){
          temp->next = node;
          break;
        }
        temp = temp->next;
      }
    }
	}
  //creating list of IP addresses - END
  */
   //create UNIX domain socket - to communicate on same machine
   int unixfd = socket(AF_UNIX,SOCK_STREAM,0);
   if(unixfd <0){
     printf("Error in creating socket\n");
     return 0;
   }
   
   bzero(&servaddr,sizeof(servaddr));
   servaddr.sun_family = AF_UNIX;
   //strcpy(servaddr.sun_path,ARP_PATH);//dont knwo?/
   unlink(servaddr.sun_path);
   
   if(bind(unixfd, (struct sockaddr*)&servaddr,sizeof(servaddr))<0){
     printf("Error binding the Unix domain socket\n");
     return 0;
   }
   
   //create packet socket 
   int packetfd = socket(AF_PACKET,SOCK_RAW,PF_PROTOCOL); //protocol value is 0x800
   if(packetfd <0){
     printf("Error creating packet socket\n");
     return 0;
   }

  hwa = Get_hw_addrs();
  for (; hwa != NULL; hwa = hwa->hwa_next) {
		status = strncmp(hwa->if_name, "eht0",strlen("eth0"));
     if(status == 0){
       addr = (struct sockaddr_in*)hwa->ip_addr;
       my_ip = addr->sin_addr.s_addr;
       memcpy((void*)srcmac,(void*)hwa->if_haddr,6);
       if_index = hwa->if_index;
     }
   
		printf("%s :%s", hwa->if_name, ((hwa->ip_alias) == IP_ALIAS) ? " (alias)\n" : "\n");
		
		if ( (sa = hwa->ip_addr) != NULL)
		//	printf("IP addr = %s\n", Sock_ntop_host(sa, sizeof(*sa)));
				
		  prflag = 0;
		  i = 0;
		  do {
			if (hwa->if_haddr[i] != '\0') {
				prflag = 1;
				break;
			}
		} while (++i < IF_HADDR);

		if (prflag) {
			printf("HW addr = ");
			ptr = hwa->if_haddr;
			i = IF_HADDR;
			do {
				printf("%.2x%s", *ptr++ & 0xff, (i == 1) ? " " : ":");
			} while (--i > 0);
		}

		printf("\ninterface index = %d\n\n", hwa->if_index);
	}

	//free_hwa_info(hwahead);
	inet_ntop(AF_INET, &(my_ip), srcip, INET_ADDRSTRLEN);
	fflush(stdout);
	printf("\nSource IP address: %s", srcip);
	printf("\nSource MAC address = ");
	ptr = srcmac;
	i = IF_HADDR;
	do {
		printf("%.2x%s", *ptr++ & 0xff, (i == 1) ? " " : ":");
	} while (--i > 0);
	fflush(stdout);
	printf("\nSource if_index = %d", if_index);

	/* Listen echo socket, check for errors */
	status = listen(unixfd, 10);
	if (status != 0) {
		printf("\nStatus = %d, Unable to listen echo service socket !!!",status);
		return 0;			
	}

 /*
 while(1){
   fd_set multfd;
   FD_ZERO(&multfd);
   FD_SET(unixfd,&multfd);
   FD_SET(packetfd,&multfd);
   int maxfdp1 = Math.max(unixfd,packetfd)+1;
   
   int ret = select(maxfdp1,&multfd,NULL,NULL,NULL);
   if(ret <0){
     printf("Error in multiplexing\n");
     return 0;
   }
   
   if(FD_ISSET(unixfd,&multfd){
     
   
   }else if(FD_ISSET(packetfd,&multfd){
     len = sizeof();
   
   }
 */
  
  //creating rt socket - START
	char buffer[PACKETSZ];
	struct ipheader *iph = (struct ipheader*)buffer;
	memset(buffer,0,PACKETSZ);
	int one =1;
	const int *val = &one;
	int rt = socket(AF_INET,SOCK_RAW,169); //169 id/protocol
 
	//tell kernel not to fill up the packet
  if(setsockopt(rt, IPPROTO_IP, IP_HDRINCL,val,sizeof(one)) < 0){
		printf("Error setting IP_HDRINCL. Eror number %d. Error Message %s \n", errno, strerror(errno));
		exit(0);
	}
	/*
	//fill in the header values --needs to be verified
	iph->ihl = 44;
	iph->ver = 4;
	iph->tos = 16;
	iph->len = sizeof(struct ipheader);
	iph->ident = htons(169);
	iph->ttl = 64;
	iph->protocol = 169;
	iph->sourceip = head->ip;
	iph->destip = head->next->ip;
  
  
  struct ippacket *packet = (struct ippacket *)malloc(sizeof(struct ippacket));
  packet->header = iph;
  packet->payload = head;
*/
	return 0;	
}


struct hwa_info *
get_hw_addrs()
{
	struct hwa_info	*hwa, *hwahead, **hwapnext;
	int		sockfd, len, lastlen, alias, nInterfaces, i;
	char		*ptr, *buf, lastname[IF_NAME], *cptr;
	struct ifconf	ifc;
	struct ifreq	*ifr, *item, ifrcopy;
	struct sockaddr	*sinptr;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	lastlen = 0;
	len = 100 * sizeof(struct ifreq);	/* initial buffer size guess */
	for ( ; ; ) {
		buf = (char*) malloc(len);
		ifc.ifc_len = len;
		ifc.ifc_buf = buf;
		if (ioctl(sockfd, SIOCGIFCONF, &ifc) < 0) {
			if (errno != EINVAL || lastlen != 0)
				printf("ioctl error");
		} else {
			if (ifc.ifc_len == lastlen)
				break;		/* success, len has not changed */
			lastlen = ifc.ifc_len;
		}
		len += 10 * sizeof(struct ifreq);	/* increment */
		free(buf);
	}

	hwahead = NULL;
	hwapnext = &hwahead;
	lastname[0] = 0;
    
	ifr = ifc.ifc_req;
	nInterfaces = ifc.ifc_len / sizeof(struct ifreq);
	for(i = 0; i < nInterfaces; i++)  {
		item = &ifr[i];
 		alias = 0; 
		hwa = (struct hwa_info *) calloc(1, sizeof(struct hwa_info));
		memcpy(hwa->if_name, item->ifr_name, IF_NAME);		/* interface name */
		hwa->if_name[IF_NAME-1] = '\0';
				/* start to check if alias address */
		if ( (cptr = (char *) strchr(item->ifr_name, ':')) != NULL)
			*cptr = 0;		/* replace colon will null */
		if (strncmp(lastname, item->ifr_name, IF_NAME) == 0) {
			alias = IP_ALIAS;
		}
		memcpy(lastname, item->ifr_name, IF_NAME);
		ifrcopy = *item;
		*hwapnext = hwa;		/* prev points to this new one */
		hwapnext = &hwa->hwa_next;	/* pointer to next one goes here */

		hwa->ip_alias = alias;		/* alias IP address flag: 0 if no; 1 if yes */
                sinptr = &item->ifr_addr;
		hwa->ip_addr = (struct sockaddr *) calloc(1, sizeof(struct sockaddr));
	        memcpy(hwa->ip_addr, sinptr, sizeof(struct sockaddr));	/* IP address */
		if (ioctl(sockfd, SIOCGIFHWADDR, &ifrcopy) < 0)
                          perror("SIOCGIFHWADDR");	/* get hw address */
		memcpy(hwa->if_haddr, ifrcopy.ifr_hwaddr.sa_data, IF_HADDR);
		if (ioctl(sockfd, SIOCGIFINDEX, &ifrcopy) < 0)
                          perror("SIOCGIFINDEX");	/* get interface index */
		memcpy(&hwa->if_index, &ifrcopy.ifr_ifindex, sizeof(int));
	}
	free(buf);
	return(hwahead);	/* pointer to first structure in linked list */
}

void
free_hwa_info(struct hwa_info *hwahead)
{
	struct hwa_info	*hwa, *hwanext;

	for (hwa = hwahead; hwa != NULL; hwa = hwanext) {
		free(hwa->ip_addr);
		hwanext = hwa->hwa_next;	/* can't fetch hwa_next after free() */
		free(hwa);			/* the hwa_info{} itself */
	}
}
/* end free_hwa_info */

struct hwa_info *
Get_hw_addrs()
{
	struct hwa_info	*hwa;

	if ( (hwa = get_hw_addrs()) == NULL)
		printf("get_hw_addrs error");
	return(hwa);
}
