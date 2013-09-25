#include "pack.h"
#include "deckC.h"
#include "socketHandler.cpp"
#include "seat.h"

#include <fstream>
#include <iostream>
#include <cstdlib>
#include <time.h>

class server {

public:

   server(const char *port, int playerCount);

   int startGame();

   ~server();

private:

   int sendAll(int fd, unsigned char *msg, int msg_len);

   int broadcast(unsigned char *msg, int msg_len);

   int eliminate(int8_t n);

   int removePlayer(int8_t n);

   int playerFolded(int8_t n);

   int playerRaised(int8_t n, int16_t amount);

   int playerCalled(int8_t n);

   int playerChecked(int8_t n);

   int playerAllin(int8_t n);

   int playerBailed(int8_t n);

   int requestAction(int8_t n);

   int readNext(int8_t n);

   int gameLoop();

   int bettingRound();

   const char *port;

   seat player[8];

   int startingChips, smallBlind, bigBlind, playersLeft, playerCount;

   int pot, minimumBet, toCall, playersInHand, lastPlayerRaised, turn;
};
