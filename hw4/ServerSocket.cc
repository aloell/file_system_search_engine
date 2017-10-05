/*
 * Copyright 2012 Steven Gribble
 *
 *  This file is part of the UW CSE 333 course project sequence
 *  (333proj).
 *
 *  333proj is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  333proj is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with 333proj.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>       // for snprintf()
#include <unistd.h>      // for close(), fcntl()
#include <sys/types.h>   // for socket(), getaddrinfo(), etc.
#include <sys/socket.h>  // for socket(), getaddrinfo(), etc.
#include <arpa/inet.h>   // for inet_ntop()
#include <netdb.h>       // for getaddrinfo()
#include <errno.h>       // for errno, used by strerror()
#include <string.h>      // for memset, strerror()
#include <iostream>      // for std::cerr, etc.

#include <pthread.h>

#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>

#include "./ServerSocket.h"

extern "C" {
  #include "libhw1/CSE333.h"
}

namespace hw4 {

ServerSocket::ServerSocket(uint16_t port) {
  port_ = port;
  listen_sock_fd_ = -1;
}

ServerSocket::~ServerSocket() {
  // Close the listening socket if it's not zero.  The rest of this
  // class will make sure to zero out the socket if it is closed
  // elsewhere.
  if (listen_sock_fd_ != -1)
    close(listen_sock_fd_);
  listen_sock_fd_ = -1;
}

bool ServerSocket::BindAndListen(int ai_family, int *listen_fd) {
  // Use "getaddrinfo," "socket," "bind," and "listen" to
  // create a listening socket on port port_.  Return the
  // listening socket through the output parameter "listen_fd".

  // MISSING:
  std::string port_str=std::to_string(port_);

  struct addrinfo hints;
  hints.ai_family = ai_family;    /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
  hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
  hints.ai_protocol = 0;          /* Any protocol */
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;
  struct addrinfo* result;
  int s=getaddrinfo(NULL,port_str.c_str(),&hints,&result);
  if (s != 0) {
     fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
     exit(EXIT_FAILURE);
  }
  /* getaddrinfo() returns a list of address structures.
     Try each address until we successfully connect(2).
     If socket(2) (or connect(2)) fails, we (close the socket
     and) try the next address. */
  int sfd;
  struct addrinfo* rp;
  for (rp= result; rp != NULL; rp = rp->ai_next) {
       sfd = socket(rp->ai_family, rp->ai_socktype,rp->ai_protocol);
       if (sfd == -1){
	   printf("sfd -1\n");
           continue;
       }
       //struct sockaddr_in* server_addr=(struct sockaddr_in*)rp->ai_addr;
       //server_addr->sin_port=htons(port_);
       int bvalue=bind(sfd, rp->ai_addr, rp->ai_addrlen);
       if ( bvalue== 0){ 
	   struct sockaddr_in* server_addr=(struct sockaddr_in*)rp->ai_addr;
           printf("bind address and port: %s : %d \n",inet_ntoa(server_addr->sin_addr),ntohs(server_addr->sin_port));
           break;                  /* Success */
       }
       fprintf(stderr, "bind error:%s\n",strerror(errno));
       close(sfd);
  }
  if (rp == NULL) {               /* No address succeeded */
       fprintf(stderr, "Could not connect\n");
       exit(EXIT_FAILURE);
  }
  freeaddrinfo(result);
  if(listen(sfd,10)!=0){
     fprintf(stderr, "%s\n",strerror(errno));
     exit(errno);
  }
  listen_sock_fd_=sfd;
  *listen_fd=sfd;
  sock_family_=rp->ai_family;   
  return true;
}

bool ServerSocket::Accept(int *accepted_fd,
                          std::string *client_addr,
                          uint16_t *client_port,
                          std::string *client_dnsname,
                          std::string *server_addr,
                          std::string *server_dnsname) {
  // Accept a new connection on the listening socket listen_sock_fd_.
  // (Block until a new connection arrives.)  Return the newly accepted
  // socket, as well as information about both ends of the new connection,
  // through the various output parameters.

  // MISSING:
  int cfd;
  struct sockaddr_in client;
  socklen_t client_addr_len=sizeof(struct sockaddr_in);
  //char buf[1000];
  cfd=accept(listen_sock_fd_,(struct sockaddr*)&client, &client_addr_len);
  std::cout<<"accpeted fd:"<<cfd<<" thread id:"<<pthread_self()<<std::endl;
  if(cfd==-1){
      fprintf(stderr, "%s\n",strerror(errno));
      exit(errno);
  }
  *accepted_fd=cfd;
  *client_port=client.sin_port;
  char* c_client_addr=inet_ntoa(client.sin_addr);
  std::string s_client_addr(c_client_addr);
  *client_addr=s_client_addr;
  *client_dnsname="";
  *server_addr="";
  *server_dnsname="";
  std::cout<<"accepted client address:"<<s_client_addr<<" client port:"<<client.sin_port<<std::endl;
  return true;
}

}  // namespace hw4
