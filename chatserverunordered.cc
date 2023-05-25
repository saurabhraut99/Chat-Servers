#include <stdlib.h>
#include <openssl/md5.h>
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdio.h>
#include <arpa/inet.h>
#include<bits/stdc++.h>
#include <ctype.h>
#include <sys/file.h>
#include <stdlib.h>
using namespace std;

int vFlag=0;

string extractPort(string s) {
	return s.substr(s.find(':')+1, s.length());
}

string extractIP(string s) {
	return s.substr(0, s.find(':'));
}

bool isServer(string msgSrcIPAddr, string msgSrcPort, string forwardingIP[], string forwardingPort[], int serverCount) {
	//cout<<"msgSrcIPAddr="<<msgSrcIPAddr<<endl;
	//cout<<"msgSrcIPAddr.len"<<msgSrcIPAddr.length()<<endl;
	//cout<<"msgSrcPort="<<msgSrcPort<<endl;
	//cout<<"msgSrcPort.len"<<msgSrcPort.length()<<endl;
	for (int i = 0 ; i < serverCount ; i++) {
//		cout<<"forwardingIP[i]="<<forwardingIP[i]<<endl;
//		cout<<"forwardingIP[i].len="<<forwardingIP[i].length()<<endl;
//		cout<<"forwardingPort[i]="<<forwardingPort[i]<<endl;
//		cout<<"forwardingPort[i].len="<<forwardingPort[i].length()<<endl;
		if (forwardingIP[i].compare(msgSrcIPAddr) == 0  &&  forwardingPort[i].compare(msgSrcPort) == 0) {
			//cout<<"IS SERVER"<<endl;
			return true;
		}
	}

	return false;
}

void executeJoin(unordered_map<string, int> &adrGroup, string msgSrcIPAddr, string msgSrcPort, string msg, int sock, struct sockaddr_in src) {
	//cout<<"Executing join"<<endl;
	string s = "";
	for(int i=0;i<msg.length() && msg[i]!='\0' && msg[i]!='\r' && msg[i]!='\n';i++) {
		s+=msg[i];
	}
	//cout<<"s="<<s<<endl;
	if (adrGroup[msgSrcIPAddr+":"+msgSrcPort] == -1) { // Src not present in any chat room
		//cout<<"Src not present in any chat room"<<endl;
		adrGroup[msgSrcIPAddr+":"+msgSrcPort] = stoi(s.substr(6, s.length()));
		//cout<<"Group number is "<<adrGroup[msgSrcIPAddr+":"+msgSrcPort]<<endl;
		string t = "+OK You are now in chat room #" + to_string(adrGroup[msgSrcIPAddr+":"+msgSrcPort]);
		sendto(sock, (char*)&t[0], t.length(), 0, (struct sockaddr*)&src, sizeof(src));

	}
	else { // Src already present in some chat room
		int room = adrGroup[msgSrcIPAddr+":"+msgSrcPort];
		//cout<<"Room="<<room<<endl;
		string x = "-ERR You are already in room " + to_string(room);
		//cout<<"Send this to the client: "<<x<<endl;
		sendto(sock, (char*)&x[0], x.length(), 0, (struct sockaddr*)&src, sizeof(src));
	}
}

void executePart(unordered_map<string, int> &adrGroup, string msgSrcIPAddr, string msgSrcPort, string msg, int sock, struct sockaddr_in src) {
	string s = "";
	for(int i=0;i<msg.length() && msg[i]!='\0' && msg[i]!='\r' && msg[i]!='\n';i++) {
		s+=msg[i];
	}
	if (adrGroup[msgSrcIPAddr+":"+msgSrcPort] == -1) { // Src not present in any chat room
		string t = "-ERR You are not in any room";
		sendto(sock, (char*)&t[0], t.length(), 0, (struct sockaddr*)&src, sizeof(src));
	}
	else { // Src present in some chat room
		string t = "+OK You have left chat room #"+to_string(adrGroup[msgSrcIPAddr+":"+msgSrcPort]);
		adrGroup[msgSrcIPAddr+":"+msgSrcPort] = -1;
		sendto(sock, (char*)&t[0], t.length(), 0, (struct sockaddr*)&src, sizeof(src));
	}
}

void executeNick(unordered_map<string, string> &adrNick, string msgSrcIPAddr, string msgSrcPort, string msg, int sock, struct sockaddr_in src) {
	string s = "";
	for(int i=0;i<msg.length() && msg[i]!='\0' && msg[i]!='\r' && msg[i]!='\n';i++) {
		s+=msg[i];
	}
	adrNick[msgSrcIPAddr+":"+msgSrcPort] = s.substr(6, s.length());
	string t = "+OK Nickname set to "+ s.substr(6, s.length());
	sendto(sock, (char*)&t[0], t.length(), 0, (struct sockaddr*)&src, sizeof(src));
}
void executeQuit(unordered_map<string, int> &adrGroup, unordered_map<string, string> &adrNick, string msgSrcIPAddr, string msgSrcPort, string msg, int sock, struct sockaddr_in src) {
	adrGroup.erase(msgSrcIPAddr+":"+msgSrcPort);
	adrNick.erase(msgSrcIPAddr+":"+msgSrcPort);
	//cout<<"QUIT"<<endl;
}

int main(int argc, char *argv[]) {
  int ordering = 0;
  string fileName;
  int serverNumber;
  if (argc < 2) {
    fprintf(stderr, "*** Author: Saurabh Mahesh Raut (sraut)\n");
    exit(1);
  }
  /* Your code here */

  for(int i=0;i<argc;i++) {
	  string s = string(argv[i]);
	  if (s.compare("-v")==0) {
		  vFlag=1;
	  }
	  else if (s.compare("-o")==0) {
		  if (string(argv[i+1]).compare("fifo")==0) {
			  ordering=1;
		  } else if (string(argv[i+1]).compare("total")==0) {
			  ordering=2;
		  }
	  }
	  else if (isdigit(s[0])) {
		  serverNumber = stoi(s);
	  }
	  else if (s.compare("fifo")!=0  && s.compare("total")!=0 && s.compare("unordered")!=0) {
		  fileName = s;
	  }
  }
  // Count the number of servers in the file
  FILE *fileptr;
  //cout<<"Filename="<<fileName<<endl;
  fileptr = fopen((char*)&fileName[0], "r");
  char arr[1000];
  int serverCount = 0;
  unordered_map<string, int> adrGroup;
  unordered_map<string, string> adrNick;
  while(fgets (arr , 1000, fileptr) != NULL) {
	  serverCount++;
  }
  fseek(fileptr, SEEK_SET, SEEK_SET);
  // Store required forwarding and Bind addresses
  bool hasDifferent[serverCount];
  string forwardingIP[serverCount];
  string forwardingPort[serverCount];
  string myBindIPAddress;
  string myBindPort;
  int k = 0;
  while(fgets (arr , 1000, fileptr) != NULL) {
  	  string s = string(arr);
  	  if (s.find(',') ==  string::npos) {  // Same IP and binding address
  		  forwardingIP[k] = s.substr(0, s.find(':'));
  		  string h = "";
  		  for(int i=s.find(':')+1 ; s[i]!='\n' && s[i]!='\r' && s[i]!='\0';i++)
  			  h+=s[i];
  		  forwardingPort[k] = h;
  		  hasDifferent[k] = false;
  		  if (k+1 == serverNumber) {
  			  myBindIPAddress = forwardingIP[k];
  			  myBindPort = forwardingPort[k];
  		  }
  	  } else { // Different IP and binding address
  		  forwardingIP[k] = s.substr(s.find(',')+1, s.find(':', s.find(',')) - s.find(',') - 1);
  		  string h = "";
  		  for(int i=s.find(':', s.find(','))+1 ; s[i]!='\n' && s[i]!='\r' && s[i]!='\0';i++)
  			  h+=s[i];
  		  forwardingPort[k] = s.substr(s.find(':')+1, s.find(',') - s.find(':') - 1);;
  		  hasDifferent[k] = true;
  		  if (k+1 == serverNumber) {
  			  myBindIPAddress = s.substr(s.find(',')+1, s.find(':', s.find(',')) - s.find(',') - 1);
  			  string h = "";
  			  for(int i=s.find(':', s.find(','))+1 ; s[i]!='\n' && s[i]!='\r' && s[i]!='\0';i++)
  				  h+=s[i];
  			  myBindPort = h;
  		  }
  	  }
  	  bzero(arr, s.length());
  	  k++;
    }
  fclose(fileptr);

  //cout<<"My IP address:"<<myBindIPAddress<<endl;
  //cout<<"My bind port:"<<myBindPort<<endl;

  // Open socket and listen on Bind address
  int sock = socket(PF_INET, SOCK_DGRAM, 0);
  struct sockaddr_in servaddr;
  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  inet_pton(AF_INET, (char*)&myBindIPAddress[0], &(servaddr.sin_addr));
  servaddr.sin_port = htons(atoi(&myBindPort[0]));
  bind(sock, (struct sockaddr*)&servaddr, sizeof(servaddr));

  int u=0;
  while (true) {
	  struct sockaddr_in src;
	  socklen_t srclen = sizeof(src);
	  char buf[1000];
	  int rlen = recvfrom(sock, buf, 1000, 0, (struct sockaddr*)&src, &srclen);
	  string msg = string(buf);
	  //cout<<"msg.length="<<msg.length()<<endl;
	  //cout<<"recieved string="<<msg<<endl;
	  string msgSrcIPAddr = inet_ntoa(src.sin_addr);
	  string msgSrcPort = to_string(ntohs(src.sin_port));
	  //cout<<"msgSrcIPAddr="<<msgSrcIPAddr<<endl;
	  //cout<<"msgSrcPort="<<msgSrcPort<<endl;
	  //cout<<"isServer(msgSrcIPAddr, msgSrcPort, forwardingIP, forwardingPort, serverCount)"<<isServer(msgSrcIPAddr, msgSrcPort, forwardingIP, forwardingPort, serverCount)<<endl;
	  if (isServer(msgSrcIPAddr, msgSrcPort, forwardingIP, forwardingPort, serverCount)) { // Src is a server
		  /*string extractedIP = msg.substr(0, msg.find(':'));
		  string extractedPort = msg.substr(msg.find(':')+1, msg.find('#')-msg.find(':')-1);
		  msg = msg.substr(msg.find('#')+1, msg.length());*/
		  //cout<<"Src is a server with IP="<<msgSrcIPAddr<<" and port="<<msgSrcPort<<endl;
		  //cout<<"msg is "<<msg<<endl;
		  int groupNo = stoi(msg.substr(0, msg.find('#')));
		  msg = msg.substr(msg.find('#')+1, msg.length());
		  //cout<<"YOO"<<endl;
		  //cout<<"msg.len="<<msg.length()<<endl;
		  for (auto x : adrGroup) {
			  if (x.second == groupNo) {
				  struct sockaddr_in sendTo;
				  bzero(&sendTo, sizeof(sendTo));
				  sendTo.sin_family = AF_INET;
				  sendTo.sin_port = htons(stoi(extractPort(x.first)));
				  inet_pton(AF_INET, (char*)&extractIP(x.first)[0], &(sendTo.sin_addr));
				  sendto(sock, (char*)&msg[0], msg.length(), 0, (struct sockaddr*)&sendTo, sizeof(sendTo));
			  }
		  }

	  }

	  else { // Src is a client
		  //cout<<"Src is a client with IP="<<msgSrcIPAddr<<" and port="<<msgSrcPort<<endl;
		  if (adrGroup.find(msgSrcIPAddr+":"+msgSrcPort) == adrGroup.end()) {  // We have recieved msg from a new client!
			  adrGroup[msgSrcIPAddr+":"+msgSrcPort] = -1;
			  adrNick[msgSrcIPAddr+":"+msgSrcPort] = msgSrcIPAddr+":"+msgSrcPort;
		  }
		  if (msg[0] == '/') { // Msg is a command
			  string command = msg.substr(1, 4);
			  //cout<<"command="<<command<<endl;
			  if (command.compare("join")==0)
				  executeJoin(adrGroup, msgSrcIPAddr, msgSrcPort, msg, sock, src);
			  else if (command.compare("part")==0)
				  executePart(adrGroup, msgSrcIPAddr, msgSrcPort, msg, sock, src);
			  else if (command.compare("nick")==0)
				  executeNick(adrNick, msgSrcIPAddr, msgSrcPort, msg, sock, src);
			  else if (command.compare("quit")==0)
				  executeQuit(adrGroup, adrNick, msgSrcIPAddr, msgSrcPort, msg, sock, src);
			  else{
				  string t = "-ERR Invalud command";
				  sendto(sock, (char*)&t[0], t.length(), 0, (struct sockaddr*)&src, sizeof(src));
			  }

		  }
		  else { // Msg a chat message
			  //cout<<"Msg is a chat message"<<endl;
			  if (adrGroup[msgSrcIPAddr+":"+msgSrcPort] == -1) {  // User that sent the message is "not" present in any chat room
				  string t = "-ERR User not present in any chat room";
				  sendto(sock, (char*)&t[0], t.length(), 0, (struct sockaddr*)&src, sizeof(src));
			  }
			  else {  // User that sent the message is present in some chat room
				  //cout<<"msg is "<<msg<<endl;
				  //cout<<"msg.length()="<<msg.length()<<endl;
				  string sss = to_string(adrGroup[msgSrcIPAddr+":"+msgSrcPort]) + string("#<")+string(adrNick[msgSrcIPAddr+":"+msgSrcPort])+string(">")+ string(msg);
				  //cout<<"Message from client is "<<sss<<endl;
				  /*if (vFlag==1){
					  string hh = string(adrNick[msgSrcIPAddr+":"+msgSrcPort]) + " just sent a message on group "+ to_string(adrGroup[msgSrcIPAddr+":"+msgSrcPort]) + string(msg);
				  	  fprintf(stderr, "%s\n", (char*)&(hh)[0]);
				  }*/
				  //cout<<"forwarding this to all servers "<<s<<endl;
				  for (int i=0;i<serverCount;i++) {
					  //cout<<"Forwarding IP="<<forwardingIP[i]<<endl;
					  //cout<<"Forwarding Port="<<forwardingPort[i]<<endl;
					  if (hasDifferent[i]==true){
						  // TODO: invoke proxy
					  }
					  struct sockaddr_in sendTo;
					  bzero(&sendTo, sizeof(sendTo));
					  sendTo.sin_family = AF_INET;
					  //cout<<"forwardingPort[i]="<<forwardingPort[i]<<endl;
					  sendTo.sin_port = htons(stoi(forwardingPort[i]));
					  inet_pton(AF_INET, (char*)&forwardingIP[i][0], &(sendTo.sin_addr));
					  sendto(sock, (char*)&sss[0], sss.length(), 0, (struct sockaddr*)&sendTo, sizeof(sendTo));
					  //cout<<"I SENT: "<<sss<<endl;
				  }
			  }
		  }
	  }

	  bzero(buf, rlen);
  }
  return 0;

}

