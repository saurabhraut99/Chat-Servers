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
string excludeNewLineChars(string s) {
	string t = "";
	for(int i=0;i<s.length() && s[i]!='\n' && s[i]!='\r' && s[i]!='\0';i++) {
		t+=s[i];
	}
	return t;
}

bool isServer(string msgSrcIPAddr, string msgSrcPort, string forwardingIP[], string forwardingPort[], int serverCount) {
	for (int i = 0 ; i < serverCount ; i++) {
		if (forwardingIP[i].compare(msgSrcIPAddr) == 0  &&  forwardingPort[i].compare(msgSrcPort) == 0) {
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
}

void sendMsgToAddr(int sock, string msg, string addr) {
	  struct sockaddr_in sendTo;
	  bzero(&sendTo, sizeof(sendTo));
	  sendTo.sin_family = AF_INET;
	  sendTo.sin_port = htons(stoi(extractPort(addr)));
	  inet_pton(AF_INET, (char*)&extractIP(addr)[0], &(sendTo.sin_addr));
	  sendto(sock, (char*)&msg[0], msg.length(), 0, (struct sockaddr*)&sendTo, sizeof(sendTo));
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

  cout<<"My IP address:"<<myBindIPAddress<<endl;
  cout<<"My bind port:"<<myBindPort<<endl;

  //---------------------------------------------------------------------------------------------
  typedef pair<int, string> pi;
  unordered_map<string, unordered_map<int, int>> counters;
  unordered_map<string, unordered_map<int,   priority_queue<pi, vector<pi>, greater<pi>>  >> holdback;
  unordered_map<string, unordered_map<int, int>> mostRecent;

  // Open socket and listen on Bind address
  int sock = socket(PF_INET, SOCK_DGRAM, 0);
  struct sockaddr_in servaddr;
  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  inet_pton(AF_INET, (char*)&myBindIPAddress[0], &(servaddr.sin_addr));
  servaddr.sin_port = htons(atoi(&myBindPort[0]));
  bind(sock, (struct sockaddr*)&servaddr, sizeof(servaddr));

  while (true) {
	  struct sockaddr_in src;
	  socklen_t srclen = sizeof(src);
	  char buf[1000];
	  int rlen = recvfrom(sock, buf, 1000, 0, (struct sockaddr*)&src, &srclen);
	  string msg = string(buf);
	  msg = msg.substr(0, rlen);
	  msg = excludeNewLineChars(msg);
	  string msgSrcIPAddr = inet_ntoa(src.sin_addr);
	  string msgSrcPort = to_string(ntohs(src.sin_port));

	  if (isServer(msgSrcIPAddr, msgSrcPort, forwardingIP, forwardingPort, serverCount)) { // Src is a server
		  // msgSeqNumber<message source address>groupNo#<Nickname of msg source>msg
		  string clientID = msg.substr(msg.find('<')+1, msg.find('>')-msg.find('<')-1);
		  int groupNo = stoi(msg.substr(msg.find('>')+1, msg.find('#') - msg.find('>') - 1));
		  int msgSeqNumber = stoi(msg.substr(0, msg.find('<')));
		  msg = msg.substr(msg.find('#')+1, msg.length());
		  holdback[clientID][groupNo].push(make_pair(msgSeqNumber, msg));

		  //cout<<"holdback["<<clientID<<"]["<<groupNo<<"].top().first="<<holdback[clientID][groupNo].top().first<<endl;
		  //cout<<"holdback["<<clientID<<"]["<<groupNo<<"].top().second="<<holdback[clientID][groupNo].top().second<<endl;
		  //cout<<"mostRecent["<<clientID<<"]["<<groupNo<<"]="<<mostRecent[clientID][groupNo]<<endl;
		  while (holdback[clientID][groupNo].empty()!=1  &&
									holdback[clientID][groupNo].top().first == mostRecent[clientID][groupNo] + 1) {
			  string msgToBeSend = holdback[clientID][groupNo].top().second;
			  holdback[clientID][groupNo].pop();
			  //cout<<"msgToBeSend to clients is "<<msgToBeSend<<endl;
			  for(auto y : adrGroup) {
				  if (y.second == groupNo) {  // Found a client to which we need to send this msg
					  sendMsgToAddr(sock, msgToBeSend, y.first);
				  }
			  }
			  mostRecent[clientID][groupNo]++;
		  }

	  }

	  else { // Src is a client
		  //cout<<"Src is a client with IP="<<msgSrcIPAddr<<" and port="<<msgSrcPort<<endl;
		  if (adrGroup.find(msgSrcIPAddr+":"+msgSrcPort) == adrGroup.end()) {  // We have received msg from a new client!
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
			  if (adrGroup[msgSrcIPAddr+":"+msgSrcPort] == -1) {  // User that sent the message is "not" present in any chat room
				  string t = "-ERR User not present in any chat room";
				  sendto(sock, (char*)&t[0], t.length(), 0, (struct sockaddr*)&src, sizeof(src));
			  }
			  else {  // User that sent the message is present in some chat room
				  int group = adrGroup[msgSrcIPAddr+":"+msgSrcPort];
				  counters[msgSrcIPAddr+":"+msgSrcPort][group]++;
				  // msgSeqNumber<message source address>groupNo#<Nickname of msg source>msg
				  string sss = to_string(counters[msgSrcIPAddr+":"+msgSrcPort][group]) +   // message sequence number
						  "<" + string(msgSrcIPAddr) +":"+string(msgSrcPort) + ">"+     // clientID
						  to_string(adrGroup[msgSrcIPAddr+":"+msgSrcPort]) +    // Group number
						  string("#<")+
						  string(adrNick[msgSrcIPAddr+":"+msgSrcPort])+string(">")+ string(msg);
				  /*if (vFlag==1){
					  string hh = string(adrNick[msgSrcIPAddr+":"+msgSrcPort]) + " just sent a message on group "+ to_string(adrGroup[msgSrcIPAddr+":"+msgSrcPort]) + string(msg);
				  	  fprintf(stderr, "%s\n", (char*)&(hh)[0]);
				  }*/
				  //cout<<"counter["<<msgSrcIPAddr<<"]["<<group<<"]="<<counters[msgSrcIPAddr+":"+msgSrcPort][group]<<endl;
				  for (int i=0;i<serverCount;i++) {
					  sendMsgToAddr(sock, sss, string(forwardingIP[i] + ":" + forwardingPort[i]));
				  }
			  }
		  }
	  }

	  bzero(buf, rlen);
  }
  return 0;

}
