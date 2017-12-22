#include "unprtt.h"
#include "funcs.h"
#include<setjmp.h>
#include <signal.h>
#include <stdlib.h>
extern int errno;
static int rttfirst = 0;
static int tout_flag;
int seed;
int rcvWindow;
int delay;
int serverport; 
char serverip[20];
double prob;

sigjmp_buf jmpbuf;

static void signal_function(int sigNum){
        siglongjmp(jmpbuf,1);
}

//write method for lostPacket()

int getData(int sockfd, struct sockaddr_in *servaddr,socklen_t servlen,int winSize, char *command);

void readclientfile(){
  FILE* cfd;
  cfd = fopen("client.in","r");
  if(cfd == NULL){
    printf("file can't open");
    exit(-1);
  }
  char *val = (char*)malloc(256);
  fgets(serverip,16,cfd);
  fgets(val,256,cfd);
  serverport = atoi(val);
  *val = '\0';
  fgets(val,256,cfd);
  seed = atoi(val);
  *val = '\0';
  fgets(val,256,cfd);
  prob = atof(val);
  *val = '\0';
  fgets(val,256,cfd);
  rcvWindow = atoi(val);
  *val = '\0';
  fgets(val,256,cfd);
  delay = atoi(val);
  *val = '\0';
  free(val);
  fclose(cfd); 

}

int main(int argc, char **argv){
  readclientfile();
  struct hostent *host = NULL;
  srand(seed);
  char cmd[30];
  int len =0, read;
  int sockfd,n;
  struct sockaddr_in servaddr,cliaddr;
  bzero(&servaddr,sizeof(servaddr));
  bzero(&cliaddr,sizeof(cliaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(serverport);
  struct in_addr ip;

  int ret = inet_pton(AF_INET,serverip,&ip);
  char addr[20];
  if(ret > 0){
    host = gethostbyaddr(&ip,sizeof(ip),AF_INET);
  }else{
    host = gethostbyname(serverip);
  }

  if(host == NULL){
    printf("Server IP does not exist");
    exit(0);
  }else{
    if(inet_ntop(AF_INET,host->h_addr,addr,INET_ADDRSTRLEN)>0){
      servaddr.sin_addr = *(struct in_addr*)host->h_addr;
    }
  }   
  sockfd = socket(AF_INET,SOCK_DGRAM,0);

  int local = ifLocal(sockfd,&cliaddr,&servaddr);
  if(local){
    int option =1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_DONTROUTE, &option, sizeof(option));
    }
    
  cliaddr.sin_family = AF_INET;
  cliaddr.sin_port = htons(0);
  
  if(bind(sockfd,(struct sockaddr *)&cliaddr,sizeof(cliaddr))<0){
    fprintf(stderr,"bind error");
    exit(1);
  }

  socklen_t length = sizeof(cliaddr);
  getsockname(sockfd,(struct sockaddr*)&cliaddr,&length);
  printf("Client socket is bound to IP : %s and port %d\n", inet_ntoa(cliaddr.sin_addr),ntohs(cliaddr.sin_port));

  length = sizeof(servaddr);
  getpeername(sockfd,(struct sockaddr*)&servaddr,&length);
  printf("Connecting to server with IP %s and port %d \n", inet_ntoa(servaddr.sin_addr),ntohs(servaddr.sin_port));
   fgets(cmd,sizeof(cmd),stdin);
    while(cmd != NULL){
      if(strcmp(cmd,"list\n")==0){
        getData(sockfd, &servaddr, sizeof(servaddr),rcvWindow,cmd);
      }else if(strstr(cmd,"download")!=NULL){
        getData(sockfd, &servaddr, sizeof(servaddr),rcvWindow,cmd);
      }else if(strcmp(cmd,"quit\n")==0){
	      break;
      }
      fgets(cmd,sizeof(cmd),stdin);
    }
    return 0;
}

void readFile(int *sock, uint32_t window){
  int sockfd = *sock;
  udp_pkt_s *pkt;
  char data[PAYLOAD];
  void *rcvbuf[PACKETSIZE]; //for receiving ack
  int maxfdp1 = sockfd+1;
  fd_set rset;
  FD_ZERO(&rset);
  FD_SET(sockfd,&rset);
  for(;;){
  if(select(maxfdp1, &rset, NULL, NULL, NULL) < 0){
		if(errno == EINTR){
			continue;
   }else{
			perror("Select Error :");
			exit(1);
		}
	}
	if(FD_ISSET(sockfd, &rset)){
		if(read(sockfd,(void*)rcvbuf,PAYLOAD) > 0){
	   pkt = (udp_pkt_s*)rcvbuf;
    fputs(pkt->buf,stdout);    		
		}else{
      break;
    }
  }
  }
  exit(0);
}


int getData(int sockfd, struct sockaddr_in *servaddr,socklen_t servlen,int winSize,char *command){
	udp_pkt_s udp_pkt,*rcv_pkt;
	int i =0,n=0,bytes_sent=0;
	struct sockaddr_in preply_addr;
  char rcvData[496];
	bzero(&udp_pkt,sizeof(udp_pkt));
   udp_pkt.seqno = 1;
   udp_pkt.ackno = 1;
	udp_pkt.window = winSize;
	udp_pkt.flag = SYN; 
	memset(udp_pkt.buf,'\0',PAYLOAD);
 if(strcmp(command,"list\n")==0){
  strcpy(udp_pkt.buf,command); 
 }else{
   strtok(command," ");
   char *filename = strtok(NULL," ");
   printf("File to download %s",filename);
   strcpy(udp_pkt.buf,filename);
 }
	n = initconn(sockfd,udp_pkt,servaddr,servlen, &rcv_pkt, &preply_addr);
	if( n < 0){
	  printf("Giving up! Client could not connect to get a response and exiting after 5th attempt\n");
	  exit(-1);
	} 
   readFile(&sockfd,winSize);
   return n;
}

int initconn(int sockfd, udp_pkt_s send_pkt,struct sockaddr_in* servaddr, int servlen,udp_pkt_s *rcv_pkt, struct sockaddr_in *replyaddr){
  sigset_t alarm_sig;
  fd_set rset;
  int maxfdp1,r;
  struct itimerval *timer;
  socklen_t len = sizeof(replyaddr);
	int retrans=0,sent_bytes=0,rcv_bytes=0;
   int newport;
   void *rcvbuf[PACKETSIZE];
   maxfdp1 = sockfd+1;
   FD_ZERO(&rset);
   FD_SET(sockfd,&rset);
  Signal(SIGALRM,signal_function);
  sigemptyset(&alarm_sig);
  sigaddset(&alarm_sig,SIGALRM);
  timer = (struct itimerval *)Malloc(sizeof(struct itimerval));
  long int start = TIMEND;
  timer->it_interval.tv_sec = 0;
  timer->it_interval.tv_usec = 0;
  timer->it_value.tv_sec = start/1000;
  timer->it_value.tv_usec = (start % 1000)*1000;
 
send_conn_req :
  initTimer(timer,start);
  Sendto(sockfd,&send_pkt,sizeof(udp_pkt_s),0,(struct sockaddr*)servaddr,servlen);
  if (sigsetjmp(jmpbuf, 1) != 0) {
        retrans++;
        printf("Send to timed out\n");
        if(retrans >= MAX_RETRANS){
          printf("Server down, not replying\n");
          exit(-1);
        }
        goto send_conn_req;
  }
  
  sigprocmask(SIG_UNBLOCK,&alarm_sig,NULL);
  if(select(maxfdp1, &rset, NULL, NULL, &timer) < 0){
		if(errno == EINTR){
			goto send_conn_req;
   }else{
			perror("Select Error :");
			exit(1);
		}
	}
	if(FD_ISSET(sockfd, &rset)){
		if(recvfrom(sockfd,rcvbuf,PACKETSIZE, 0, (struct sockaddr*)replyaddr, &len) < 0){
				perror("RecvFrom error :");
				exit(1);
			}
       printf("Received ACK from server!\n");
		}
   sigprocmask(SIG_BLOCK,&alarm_sig,NULL);
   initTimer(timer,0);
   sigprocmask(SIG_UNBLOCK,&alarm_sig,NULL);
   rcv_pkt = (udp_pkt_s *)rcvbuf;
   newport = atoi(rcv_pkt->buf);
   printf("Now client will communicate with server IP %s and new port %d\n",inet_ntoa(servaddr->sin_addr),newport); 
   servaddr->sin_port = htons((newport));
   connect(sockfd, (SA*)servaddr,sizeof(servaddr));
   
send_command_req:
  send_pkt.flag = SYN_ACK_ACK;
  send_pkt.seqno = 1;
  send_pkt.ackno =1;
  Sendto(sockfd,&send_pkt,sizeof(udp_pkt_s),0,(struct sockaddr*)servaddr,servlen);
  printf("Client has sent the command request\n");
  printf("Waiting for server's reply....\n");
  
return 1;
}
