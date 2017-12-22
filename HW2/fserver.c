#include "/home/courses/cse533/Stevens/unpv13e_solaris2.10/lib/unp.h"
#include "/home/courses/cse533/Stevens/unpv13e_solaris2.10/lib/unpifi.h"
#include "funcs.h"
#include <setjmp.h>
#include "unprtt.h"
#include <dirent.h>

void sendFile(int *sock,int fd);
void listenconn(int *sockfd, struct ifi_info *ifireqlist, int ips);
int deleteClientProcess(int pid);
int addClientRequest(struct sockaddr_in cliaddr,pid_t pid);
sigjmp_buf jmpbuf;

static void signal_function(int sigNum){
        siglongjmp(jmpbuf,1);
}

char pathToFile[100];
int sendWindow;
int lastCreatedChild =0;
static int rttfirst = 0;
struct clireqlist *reqlist;
static void sighandler(int sigNum){
	//if (lastCreatedChild == 0)
		return;
  //if any thing wrong goes delete the last connection child
	//deleteClientProcess(lastCreatedChild);
	//lastCreatedChild = 0;
}

int main(int argc, char *argv[]){
  FILE *file;
  char *val,*path;
  size_t length;
  socklen_t len;
  int *unifd,nready,maxfdp1,servport,numOfIPs=0,i;
  char msg[MAXLINE];
  pid_t childpid;
  fd_set rset;
  int clilen;
  const int on =1;
  struct sockaddr_in *sa,servaddr;
  struct ifi_info *ifi, *ifireqlist;

  file = fopen("server.in","r");
  val = malloc(256);
  path = malloc(MAXLINE);
  fgets(val,256,file);
  servport = atoi(val);
  *val = '\0';
  fgets(val,256,file);
  sendWindow = atoi(val);
  *val = '\0';
  fgets(path,MAXLINE,file);
  
  fclose(file);
  strcpy(pathToFile,path);
  ifireqlist = get_ifi_info(AF_INET,1);
  
  numOfIPs = get_interfaces(ifireqlist);
  unifd = (int*)Malloc(numOfIPs*sizeof(int));
  for(i=0,ifi=ifireqlist;ifi!=NULL;ifi=ifi->ifi_next,i++){ 
    bzero(&servaddr,sizeof(servaddr));
    unifd[i] = Socket(AF_INET,SOCK_DGRAM,0);
    printf("Socket fd %d\n",unifd[i]);
    Setsockopt(unifd[i],SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
    servaddr.sin_family = AF_INET;
    servaddr = *(struct sockaddr_in *)ifi->ifi_addr;
    servaddr.sin_port = htons(servport);
    if(bind(unifd[i], (struct sockaddr *)&servaddr, sizeof(servaddr))<0){
      printf("Server socket bind error\n");
      perror("bind");
    }
    sa = (struct sockaddr_in *)ifi->ifi_addr;
    printf("IP : %s\n",Sock_ntop_host((struct sockaddr *)sa, sizeof(*sa)));
	}

  listenconn(unifd,ifireqlist,numOfIPs);
} 

int clientPresent(struct sockaddr_in *current, struct sockaddr_in*newcli){
  if((current->sin_addr.s_addr == newcli->sin_addr.s_addr) || (current->sin_port == newcli->sin_port) || (current->sin_family == newcli->sin_family)){
  printf("Client is already connected with the server!");
  return 1;
  }
return 0;
}

int checkClient(struct sockaddr_in cliaddr){
  if(reqlist == NULL){
    return 1;
  }
  clireqlist *p = checkClient;
  while(p->nextcli != NULL){
    if(clientPresent(&(p->cliaddr),&cliaddr)==1){
      return 0;
    }
    p=p->nextcli;
  }
  return 1;
}

FILE* getFile(){
  int fd = open("dirlist.txt",O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  FILE *file = fdopen(fd,"w");
  DIR* dir;
  struct dirent *dp;
  char cwd[1024];
  //printf("Path to file %s",pathToFile);
  getcwd(cwd,sizeof(cwd));
  strcat(cwd,pathToFile);
  if((dir = opendir(cwd)) != NULL){
    while((dp = readdir(dir)) != NULL){
      fputs(dp->d_name,file);
      fputs("\n",file);
    }
    closedir(dir);
  }else{
    perror("Something went wrong\n");
    return NULL;
  }
  return file;
}

void sendFile(int *sock,int fd){
  uint32_t seqno=1,ackno=1,winsize,flag,time;
  int sockfd = *sock;
  udp_pkt_s datapkt;
 	bzero(&datapkt,sizeof(datapkt));
  char data[PAYLOAD];
  void *rcvbuf[PACKETSIZE]; //for receiving ack
  
  ssize_t bytes_read;
  do{
    bytes_read = read(fd,(void*)data,PAYLOAD);
    datapkt.seqno = ++seqno;
    datapkt.ackno = ++ackno;
    datapkt.flag = DATA;
    memset(datapkt.buf,'\0',PAYLOAD);
    strcpy(datapkt.buf,(char*)data);
    printf("Sending packet\n");
    write(sockfd,&datapkt,PAYLOAD);
  } while(bytes_read != 0);
  close(fd);
  printf("Done sending file\n");
}

int connectClient(int sockfd,struct sockaddr_in cliaddr,int cliWindow,udp_pkt_s *rcvdata,char *cmd,int isLocal){
  pid_t pid;
  uint32_t seqno,ackno,winsize,flag,time;
  udp_pkt_s *pkt;
  char command[50];
  static congestion serv; //congestion object
  void *rcvbuf[PACKETSIZE];
  pkt = rcvdata;
  struct sockaddr_in servaddr;
  sigset_t alarm_sig;
  socklen_t len;
  struct itimerval *timer;
  int retrans=0,sent_bytes=0,rcv_bytes=0;
  Signal(SIGALRM,signal_function);
  sigemptyset(&alarm_sig);
  sigaddset(&alarm_sig,SIGALRM);
  cliaddr.sin_family = AF_INET;
  timer = (struct itimerval *)Malloc(sizeof(struct itimerval));
  long int start = TIMEND;
  timer->it_interval.tv_sec = 0;
  timer->it_interval.tv_usec = 0;
  timer->it_value.tv_sec = start/1000;
  timer->it_value.tv_usec = (start % 1000)*1000;
   
rcv_conn_request:
 seqno = rcvdata->seqno;
 ackno = rcvdata->ackno;
 flag = rcvdata->flag;
 winsize = rcvdata-> window;
 time = rcvdata->ts;
 strcpy(command,rcvdata->buf);
 printf("Received connection request packet\n");
 
ack_first_request:
  flag = SYN_ACK;
  seqno =1; ackno = 2;
  char newport[50];
  socklen_t servlen = sizeof(servaddr);
  len = sizeof(struct sockaddr_in);
  Getsockname(sockfd,(struct sockaddr*) &servaddr,&len);
  int newsockfd = Socket(AF_INET,SOCK_DGRAM,0);
  if(isLocal == 1){
    int option =1;
    Setsockopt(newsockfd,SOL_SOCKET,SO_DONTROUTE,&option,sizeof(option));
  }
  servaddr.sin_port =0;
  Bind(newsockfd,(struct sockaddr*)&servaddr,sizeof(servaddr));
  Getsockname(newsockfd,(struct sockaddr*)&servaddr,&servlen);
  int temp = ntohs(servaddr.sin_port);
  printf("New port for client-server communication : %d\n",temp);
  sprintf(newport,"%d",temp);
  initTimer(timer,start);
  pkt->seqno = seqno;
  pkt->flag = flag;
  pkt->ackno = ackno;
  memset(pkt->buf,'\0',PAYLOAD);
  strcpy(pkt->buf,newport);
  Sendto(sockfd,pkt,sizeof(udp_pkt_s),0,(struct sockaddr*)&cliaddr,sizeof(cliaddr));
   if (sigsetjmp(jmpbuf, 1) != 0) {
        retrans++;
        printf("Send to timed out");
        if(retrans >= MAX_RETRANS){
          printf("Client is not responding. Returning\n");
          return 0;
        }
        goto ack_first_request;
  }

read_command_data:
  len = sizeof(cliaddr);
  sigprocmask(SIG_UNBLOCK,&alarm_sig,NULL);
  rcv_bytes = recvfrom(newsockfd,rcvbuf,PACKETSIZE,0,(struct sockaddr *)&cliaddr,&len);
  udp_pkt_s *packet = (udp_pkt_s *)rcvbuf;
  seqno = packet->seqno;
  ackno = packet->ackno;
  flag = packet->flag;
  winsize = packet->window;
  time = packet->ts;
  strcpy(command,packet->buf);
  sigprocmask(SIG_BLOCK,&alarm_sig,NULL);
  Connect(newsockfd,(struct sockaddr*)&cliaddr,sizeof(cliaddr));
  Close(sockfd);
  initTimer(timer,0);
  sigprocmask(SIG_UNBLOCK,&alarm_sig,NULL);
  if(strcmp(command,"list\n")==0){
    printf("Client connected with new port! Executing command : %s",command);
    FILE* file = getFile();
    int fd = fileno(file);
    sendFile(&newsockfd,fd);
    return 1;
  }else{
    printf("Client connected with new port! Downloading file : %s\n",command);
    command[strlen(command)-1] = 0;
    char cwd[1024];
    getcwd(cwd,sizeof(cwd));
    strcat(cwd,pathToFile);
    strcat(cwd,command);
    FILE* file = fopen(cwd, "r");
    if(file == NULL){
      printf("File does not exists\n");
      return 0;
    }
    int fd = fileno(file);
    sendFile(&newsockfd,fd);
    return 1;
  }
}

void listenconn(int *sockfd, struct ifi_info *ifireqlist, int ips){
  int maxfdp1,n,r,i;
  int isLocal;
  struct sockaddr_in cliaddr;
  fd_set readfd;
  void *rcvbuf[PACKETSIZE];
  char data[200];
  uint32_t cliWindow = 200;
  char *cmd;
  maxfdp1 = -1;;
  FD_ZERO(&readfd);
  
  for(i=0;i<ips;i++){
    FD_SET(sockfd[i],&readfd);
    if(sockfd[i] > maxfdp1){
      maxfdp1 = sockfd[i];
    }
  }
  
  maxfdp1 +=1;
  signal(SIGCHLD,sighandler);
  printf("Server is listening for connection\n");
  for(;;){
    while(1){
      if(r = select(maxfdp1,&readfd,NULL,NULL,NULL))
      if(r < 0){
      if(errno == EINTR){
	      continue;
      }else{
	      printf("select error");
      }
    }else if(r == 0){
      printf("still waiting...");
      fflush(stdout);
    }else{
      break;
    }
  }
    for(i=0;i<ips;i++){
      if(FD_ISSET(sockfd[i],&readfd)){
	      struct sockaddr_in servaddr,cliaddr;
	      socklen_t len = sizeof(cliaddr);
	      memset(rcvbuf,'\0',PACKETSIZE);
	      n = recvfrom(sockfd[i],rcvbuf,PACKETSIZE,0,(struct sockaddr *)&cliaddr,&len);
	    if(n <0){
	      break;
	    }
       udp_pkt_s *packet = (udp_pkt_s *)rcvbuf;
	    if(packet->flag != SYN){ 
	      break;
	    }
	    socklen_t servlen = sizeof(servaddr);
       printf("Address sending the new connection request : %s and port %d\n", inet_ntoa(cliaddr.sin_addr),ntohs(cliaddr.sin_port));
	    pid_t pid;
     if(checkClient(cliaddr) == 0){
       continue;
     }
	    if((pid = fork())==0){
         Getsockname(sockfd[i],(struct sockaddr *)&servaddr,&servlen);
         if(strcmp(inet_ntoa(cliaddr.sin_addr),"127.0.0.1")==0){
           printf("Am i?");
           isLocal = ifLocal(sockfd,&servaddr,&cliaddr);
         }
         connectClient(sockfd[i],cliaddr,cliWindow,(udp_pkt_s*)rcvbuf,cmd,isLocal);
         printf("Server done.\n");
         exit(0);
	    }else{
         addClientRequest(cliaddr,pid);
         lastCreatedChild = pid;
	    }
      }
    }
}
}

int deleteClientProcess(int pid){
  clireqlist *curr = reqlist;
  clireqlist *prev = reqlist;
  
  if(reqlist == NULL){
    return 0;
  }
  while(curr != NULL){
    if(curr->pid == pid){
      if(curr == reqlist){
        reqlist = reqlist->nextcli;
      }else{
        prev->nextcli = curr->nextcli;
      }
      printf("Deleted Child process %d\n",pid);
      return 1;
    }
  prev = curr;
  curr = curr->nextcli;
  if(curr == NULL){
    break;
  }
  }
  return 0;
}

int addClientRequest(struct sockaddr_in cliaddr,pid_t pid){
  if(reqlist == NULL){
    clireqlist *newlist = (clireqlist *)Malloc(sizeof(clireqlist));
    newlist->cliaddr = cliaddr;
    newlist->nextcli = NULL;
    newlist->pid = pid;
    reqlist = newlist;
    return 1;
  }
  clireqlist *list = reqlist;
  while(list->nextcli != NULL){
    if(clientPresent(&(list->cliaddr),&cliaddr)==1){
      return 0;
    }
    list = list->nextcli;
  }
  
  clireqlist *newcli = (clireqlist *)Malloc(sizeof(clireqlist));
  newcli->cliaddr = cliaddr;
  newcli->nextcli = NULL;
  newcli->pid = pid;
  list->nextcli = newcli;
  return 1;
}




















  









    





















