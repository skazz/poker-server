#include "pack.h"
#include "deckC.h"
#include "socketHandler.cpp"
#include "seat.h"

#include <fstream>
#include <iostream>

class server {

public:

   server(const char *port, int playerCount);

   int startGame();

   ~server();

private:

   int sendAll(int fd, unsigned char *msg, int msg_len);

   int broadcast(int *fd, int count, unsigned char *msg, int msg_len);

   int read(unsigned char *buf);

   int gameLoop();

   const char *port;

   int playerCount;

   int client[8];

   seat player[8];

   int startingChips, smallBlind, bigBlind, playersLeft;
};
