#include <cstdlib>
#include <cstring>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/wait.h>

#define BACKLOG 10

using namespace std;

void sigchld_handler(int s) {
   while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, ipv4 ipv6
void *get_in_addr(struct sockaddr *sa) {
   if(sa->sa_family == AF_INET) {
      return &(((struct sockaddr_in*)sa)->sin_addr);
   }
   return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int waitForPlayers(int sockfd, int *client, int playerCount) {
   
   socklen_t sin_size;
   char s[INET6_ADDRSTRLEN];
   struct sigaction sa;
   struct sockaddr_storage their_addr; // connector's address information
   if(listen(sockfd, BACKLOG) == -1) {
      perror("listen");
      return -1;;
   }

   sa.sa_handler = sigchld_handler; //reap all dead processes (?)
   sigemptyset(&sa.sa_mask);
   sa.sa_flags = SA_RESTART;
   if(sigaction(SIGCHLD, &sa, NULL) == -1) {
      perror("sigaction");
      return -1;
   }

   printf("server: waiting for connections...\n");

   int8_t i = 0;
   while(i < playerCount) { // main accept() loop
      sin_size = sizeof their_addr;
      client[i] = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
      if(client[i] == -1) {
         perror("accept");
         continue;
      }

      inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr*)&their_addr), s, sizeof s);
      printf("server: got connection from %s\n", s);

      i++;
   }

   close(sockfd); //all done
}

int setup(int *client, int playerCount, const char *port) {

   int sockfd;
   struct addrinfo hints, *servinfo, *p;
   int yes=1;
   int rv;

   memset(&hints, 0, sizeof hints);
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_flags = AI_PASSIVE;

   if((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
      return -1;
   }

   // loop through all the results and bind to the first we can
   for(p = servinfo; p != NULL; p = p->ai_next) {
      if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
         perror("server: socket");
         continue;
      }

      if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
         perror("setsockopt");
         return -1;
      }

      if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
         close(sockfd);
         perror("server: bind");
         continue;
      }

      break;
   }

   if(p == NULL) {
      fprintf(stderr, "server: failed to bind\n");
      return -1;
   }

   freeaddrinfo(servinfo);

   return waitForPlayers(sockfd, client, playerCount);
}

int cleanup(int *fd) {
   for(; *fd != '\0'; fd++)
      close(*fd);
}
