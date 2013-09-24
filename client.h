#ifndef CLIENT_H
#define CLIENT_H

#include "pack.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <fstream>
 
class client {
 
public:
 
   client(const char *name);

   int connectToServer(const char *remoteHost, const char *port);

   ~client();

protected:

   // communication to server
   int fold();

   int raise(int16_t amount);

   int call();

   int check();

   int allin();

   int bail();

   // communication from server
   virtual void placeBet() = 0; // <- needed!


private:

   const char *name;

   int sockfd;

   int gameLoop();

   int read(unsigned char *buf);

   int sendAll(int fd, unsigned char *msg, int msg_len);
};

#endif
