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

   int broadcast(unsigned char *msg, int msg_len);

   int eliminate(int8_t n);

   int gameLoop();

   const char *port;

   seat player[8];

   int startingChips, smallBlind, bigBlind, playersLeft, playerCount;
};
