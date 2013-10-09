#ifndef CLIENT_H
#define CLIENT_H

#include "pack.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
//#include <fstream>
 
class client {
 
public:
 
   client(const char *name);

   int connectToServer(const char *remoteHost, const char *port);

   ~client();

protected:

   // communication to server
   int ready();

   int fold();

   int raise(int16_t amount);

   int call();

   int check();

   int allin();

   int bail();

   /* 
      communication from server
      gets called when server requests your action
      answer with check() call() raise()...
   */
   virtual void placeBet() = 0; // <- needed!

   virtual void nextRound() { ready(); }; // tell server when you are ready 45s timeout
   // hooks for notifications from server

   virtual void setHolecards(int8_t c1, int8_t c2) {};

   virtual void setFlop(int8_t c1, int8_t c2, int8_t c3) {};

   virtual void setTurn(int8_t c1) {};

   virtual void setRiver(int8_t c1) {};

   virtual void playerFolded(int8_t n) {};

   virtual void playerRaised(int8_t n, int16_t amount) {};

   virtual void playerCalled(int8_t n) {};

   virtual void playerChecked(int8_t n) {};

   virtual void playerAllin(int8_t n, int16_t amount) {};

   virtual void playerHolecards(int8_t n, int8_t c1, int8_t c2) {};

   virtual void playerIsDealer(int8_t n) {};

   virtual void playerIsSmallBlind(int8_t n) {};

   virtual void playerIsBigBlind(int8_t n) {};

   virtual void setBlinds(int16_t small, int16_t big) {};

   virtual void playerWon(int8_t n, int16_t amount) {};

   virtual void tie() {};

   virtual void setPlayerName(int8_t n, unsigned char *name) {};

   virtual void setPlayerCount(int8_t n) {};

   virtual void setSeatNumber(int8_t n) {};

   virtual void setStartingChips(int16_t amount) {};

   virtual void playerEliminated(int8_t n) {};

   virtual void playerLeft(int8_t n) {};

   virtual void actionAccepted() {};

   virtual void actionUnknown() {};

   virtual void actionNotAllowed() {};

   virtual void betTooLow() {};


private:

   const char *name;

   int sockfd;

   int gameLoop();

   int read(unsigned char *buf);

   int sendAll(int fd, unsigned char *msg, int msg_len);
};

#endif
