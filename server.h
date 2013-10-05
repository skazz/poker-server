#include "pack.h"
#include "deckC.h"
#include "socketHandler.cpp"
#include "seat.h"
#include "handEvaluator.h"
#include "log.h"

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

   void eliminate(int8_t n);

   void removePlayer(int8_t n);

   int playerWon(int8_t n, int16_t amount);

   int playerFolded(int8_t n);

   int playerRaised(int8_t n, int16_t amount);

   int playerCalled(int8_t n);

   int playerChecked(int8_t n);

   int playerAllin(int8_t n);

   int playerBailed(int8_t n);

   int requestAction(int8_t n);

   int readNext(int8_t n);

   int clear(int8_t n);

   int gameLoop();

   int bettingRound();

   int getPot();

   int getPlayersInHand();

   int getPlayersAllin();

   int showdown();

   CLog log;

   const char *port;

   seat player[8];

   int startingChips, smallBlind, bigBlind, playersLeft, playerCount;

   int pot, minimumBet, toCall, lastPlayerRaised, turn;

};
