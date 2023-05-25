#include <stdio.h>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <bits/stdc++.h>
#include <signal.h>
#include <sys/file.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/select.h>
#include <signal.h>
#include <ctime>
using namespace std;

struct sockaddr_in dest;
int sock;

void signal_callback_handler(int signum) {
	string sendMsg = "/quit";
	sendto(sock, (char*)&sendMsg[0], sendMsg.length(), 0, (struct sockaddr*)&dest, sizeof(dest));
	exit(1);
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Saurabh Mahesh Raut (sraut)\n");
    exit(1);
  }
  signal(SIGINT, signal_callback_handler);

  /* Your code here */
  int c = 0;
  int vFlag = 0;
  string arg;
  string IP;
  string port;
  while ((c = getopt (argc, argv, "v")) != -1) {
		if (c == 'v') {
		   vFlag = 1;
		}
  }
  arg = string(argv[optind]);
  IP = arg.substr(0, arg.find(':'));
  port = arg.substr(arg.find(':')+1, arg.length());

  sock = socket(PF_INET, SOCK_DGRAM, 0);
  if (sock < 0) {
	  fprintf(stderr, "Cannot open socket (%s)\n", strerror(errno));
	  exit(1);
  }

  bzero(&dest, sizeof(dest));
  dest.sin_family = AF_INET;
  int x = atoi((char*)&port[0]);
  dest.sin_port = htons(x);
  inet_pton(AF_INET, (char*)(&IP[0]), &(dest.sin_addr));


  char bufStdIn[1000];
  char bufClient[1000];
  struct sockaddr_in src;
  socklen_t srcSize = sizeof(src);

  fd_set desc;
  int max = STDIN_FILENO;
  if (sock > max)
	  max = sock;
  while(true) {
	  FD_ZERO(&desc);
	  FD_SET(sock, &desc);
	  FD_SET(STDIN_FILENO, &desc);
	  select(max+1, &desc, NULL, NULL, NULL);
	  if (FD_ISSET(STDIN_FILENO, &desc)) {  // Input from STD IN
		  int rlen = read(STDIN_FILENO, bufStdIn, 1000);
		  string sendMsg = "";
		  for (int i=0;bufStdIn[i]!='\r' && bufStdIn[i]!='\n' && bufStdIn[i]!='\0';i++)
			  sendMsg+=bufStdIn[i];
		  sendMsg = sendMsg.substr(0,rlen);
		  sendto(sock, (char*)&sendMsg[0], sendMsg.length(), 0, (struct sockaddr*)&dest, sizeof(dest));
		  if (sendMsg.compare("/quit")==0) {
			  return 0;
		  }
		  bzero(bufStdIn, 1000);
	  }
	  if (FD_ISSET(sock, &desc)) {  // Input from server
		  //cout<<"Input from server"<<endl;
		  int rlen = recvfrom(sock, bufClient, sizeof(bufClient), 0, (struct sockaddr*)&src, &srcSize);
		  string s = string(bufClient);
		  if (vFlag == 1){
			  	string p = s+"\r\n";
		  		fprintf(stderr, "%s",(char*)&(p[0]));
		  }
		  bzero(bufClient, sizeof(bufClient));
	  }
  }

  close(sock);

  return 0;
}  
