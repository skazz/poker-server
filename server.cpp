#include "server.h"

using namespace std;

server::server(const char *port, int playerCount) {
   time_t seconds;
   time(&seconds);
   srand((unsigned int) seconds);

   this->port = port;
   this->playerCount = playerCount;
   playersLeft = playerCount;

   startingChips = 20000;
   smallBlind = 1000;
   bigBlind = 2000;

   for(int8_t i = 0; i < playerCount; i++)
      player[i] = seat(i, startingChips);

   log.setLogLevel(VERBOSE);
   log.setDisplayMessages(true);

}

server::~server() {
   int client[playerCount];
   for(int i = 0; i < playerCount; i++)
      client[i] = player[i].getFD();
   cleanup(client);
}

int server::sendAll(int n, unsigned char *msg, int msg_len) {
   int fd = player[n].getFD();
   int bytes_left = msg_len;
   int bytes_send = 0;
   while(bytes_left > 0) {
      if((bytes_send = send(fd, msg + msg_len - bytes_left, bytes_left, 0)) == -1)
         return -1;

      bytes_left = bytes_left - bytes_send;
   }
   return 0;
}

int server::broadcast(unsigned char *msg, int msg_len) {
   for(int i = 0; i < playerCount; i++) {
      if(sendAll(i, msg, msg_len) == -1) {
         return -1;
      }
   }
   return 0;
}

int server::waitForNextRound() {
   struct timeval tv;
   fd_set readfds, master;

   time_t start, now;

   unsigned char buf[32], tmp_buf[64], *p;
   int buf_len = sizeof buf;
   int tmp_offset = 0;
   int bytes_received = 0;
   int8_t size, action;

   unsigned char msg[16];
   int msg_len, maxFD;

   int playersReady = 0;

   // ask players
   log.log(VERBOSE, "Waiting for players");
   msg_len = pack(msg, "b", 19);
   if(broadcast(msg, msg_len) == -1)
      log.log(ERROR, "ERROR: broadcasting ready?");


   FD_ZERO(&readfds);
   FD_ZERO(&master);
   maxFD = -1;

   for(int i = 0; i < playersLeft; i++) {
      FD_SET(player[i].getFD(), &master);
      if(player[i].getFD() > maxFD)
         maxFD = player[i].getFD();
   }

   time(&start);
   time(&now);

   while(difftime(now, start) < 45 && playersReady < playersLeft) {
      tv.tv_sec = 45 - difftime(start, now);
      tv.tv_usec = 0;

      readfds = master;

      if(select(maxFD + 1, &readfds, NULL, NULL, &tv) == -1)
         log.log(ERROR, "ERROR: selecting");


      for(int i = 0; i < maxFD + 1; i++) {
         if(FD_ISSET(i, &readfds)) {
            if((bytes_received = recv(i, buf, buf_len, 0)) > 0) {

               memcpy(tmp_buf + tmp_offset, buf, bytes_received);
               tmp_offset += bytes_received;
               if(bytes_received < 1)
                  continue;

               size = *tmp_buf;

               if(tmp_offset >= size + 1) {

                  p = tmp_buf + 1;

                  action = *p++;

                  switch(action) {
                  case 10:
                     FD_CLR(i, &master);
                     playersReady++;
                     log.log(VERBOSE, "socket %d ready", i);
                     break;
                  }

                  tmp_offset -= (size + 1);
                  memmove(tmp_buf, tmp_buf + size + 1, tmp_offset);

                  if(tmp_offset > 0)
                     size = *tmp_buf;
               }
            }
         }
      }
      time(&now);
   }

   return -1;
}

int server::readNext(int8_t n) {

   struct timeval tv;
   fd_set readfds;

   time_t start, now;

   int fd = player[n].getFD();
   unsigned char buf[32], tmp_buf[64], *p;
   int buf_len = sizeof buf;
   int tmp_offset = 0;
   int bytes_received = 0;
   int8_t size, action;
   int16_t d;

   unsigned char msg[16];
   unsigned char s[16];
   int msg_len;

   time(&start);
   time(&now);

   FD_ZERO(&readfds);
   FD_SET(fd, &readfds);

   while(difftime(now, start) < 45) {

      tv.tv_sec = 45 - difftime(start, now);
      tv.tv_usec = 0;
      select(fd+1, &readfds, NULL, NULL, &tv);

      if(FD_ISSET(fd, &readfds)) {
         if((bytes_received = recv(fd, buf, buf_len, 0)) > 0) {

            memcpy(tmp_buf + tmp_offset, buf, bytes_received);
            tmp_offset += bytes_received;
            if(bytes_received < 1)
               continue;

            size = *tmp_buf;

            if(tmp_offset >= size + 1) {

               p = tmp_buf + 1;

               action = *p++;

               switch(action) {
               case 0:
                  unpack(p, "s", &s);
                  player[n].setName(s);
                  msg_len = pack(msg, "bbs", 11, player[n].getNumber(), s);
                  if(broadcast(msg, msg_len) == -1)
                     log.log(ERROR, "ERROR: broadcasting name");
                  
                  log.log(SERVER, "%s joined", player[n].getName());
                  return 1;
               case 1:
                  return playerFolded(n);
               case 2:
                  unpack(p, "h", &d);
                  return playerRaised(n, d);
               case 3:
                  return playerCalled(n);
               case 4: 
                  return playerChecked(n);
               case 5:
                  return playerAllin(n);
               case 6:
                  return playerBailed(n);
               default:
                  return -1;
               }

               tmp_offset -= (size + 1);
               memmove(tmp_buf, tmp_buf + size + 1, tmp_offset);

               if(tmp_offset > 0)
                  size = *tmp_buf;
            }
         }
      }
      time(&now);
   }

   return -1;

}


int server::startGame() {
   int client[playerCount];

   if((setup(client, playerCount, port)) == -1) {
      log.log(ERROR, "ERROR: socket error");
      return -1;
   }

   for(int i = 0; i < playerCount; i++)
      player[i].setFD(client[i]);

   
   // get names
   for(int i = 0; i < playerCount; i++) {
      if(readNext(i) == -1)
         log.log(ERROR, "ERROR: reading names");
   }


   // broadcast playerCount
   unsigned char msg[64];
   int msg_len;
   msg_len = pack(msg, "bb", 10, playerCount);
   if(broadcast(msg, msg_len) == -1)
      log.log(ERROR, "ERROR: broadcasting playercount");


   
   

   // assign players their seat and tell others
   for(int i = 0; i < playerCount; i++) {
      msg_len = pack(msg, "bb", 12, player[i].getNumber());
      if(sendAll(i, msg, msg_len) == -1)
         log.log(ERROR, "ERROR: assigning seat to %d", i);

   }

   // broadcast starting chips
   msg_len = pack(msg, "bh", 15, startingChips);
   if(broadcast(msg, msg_len) == -1)
      log.log(ERROR, "ERROR: broadcasting starting chips");

   // commence
   return gameLoop();
   
}

int server::gameLoop() {
   unsigned char msg[16];
   int msg_len;

   int dealer = rand() % playerCount; // rng
   int turn;
   bool blindsChanged = true;
   bool cardsOnTable = false;

   bool sorted;
   int tmp, tmppot, minBet, count, sidepot, distributedpot;

   deckC deck;
   int8_t board[5];
   int8_t tmpHole[2];
   int winner[playerCount];
   int handrank[playerCount];

   handEvaluator eval;


   while(playersLeft > 1) {

      waitForNextRound();

      // shuffle deck
      deck.shuffle();


      // reset seats
      for(int i = 0; i < playersLeft; i++)
         player[i].newRound();


      // broadcast new blinds
      if(blindsChanged) {
         msg_len = pack(msg, "bhh", 20, smallBlind, bigBlind);
         if(broadcast(msg, msg_len) == -1)
            log.log(ERROR, "ERROR: broadcasting blinds");
         blindsChanged = false;
      }


      // tell dealer, small, big blinds (broadcast?)
      msg_len = pack(msg, "bb", 21, player[dealer].getNumber());
      if(broadcast(msg, msg_len) == -1)
         log.log(ERROR, "ERROR: broadcasting dealer");


      // small blind(check if enough chips)
      msg_len = pack(msg, "bb", 22, player[(dealer + 1) % playersLeft].getNumber());
      if(broadcast(msg, msg_len) == -1)
         log.log(ERROR, "ERROR: broadcasting small blind");

      if(player[(dealer + 1) % playersLeft].getRemainingChips() <= smallBlind) {
         playerAllin((dealer + 1) % playersLeft);
      } else {
         player[(dealer + 1) % playersLeft].bets(smallBlind);
      }

      // big blind(check if enough chips)
      msg_len = pack(msg, "bb", 23, player[(dealer + 2) % playersLeft].getNumber());
      if(broadcast(msg, msg_len) == -1)
            log.log(ERROR, "ERROR: broadcasting big blind");

      if(player[(dealer + 2) % playersLeft].getRemainingChips() <= bigBlind) {
         playerAllin((dealer + 2) % playersLeft);
      } else {
         player[(dealer + 2) % playersLeft].bets(bigBlind);
      }


      minimumBet = bigBlind;
      toCall = bigBlind;
      cardsOnTable = false;

      // deal cards
      for(int i = 0; i < 2; i++) {
         for(int j = 0; j < playersLeft; j++) {
            player[j].dealHoleCard(i, deck.getCard());
         }
      }

      for(int i = 0; i < playersLeft; i++) {
         msg_len = pack(msg, "bbb", 30, player[i].getHoleCard(0), player[i].getHoleCard(1));
         if(sendAll(i, msg, msg_len) == -1)
            log.log(ERROR, "ERROR: sending cards to %d", i);

         log.log(HAND, "Player %d has %d %d", player[i].getNumber(), player[i].getHoleCard(0), player[i].getHoleCard(1));
      }

      // betting round 1
      turn = (dealer + 3) % playersLeft;
      bettingRound(turn);

      // check remaining players / allins
      if(getPlayersInHand() <= 1) {
         if(getPlayersAllin() > 0) {
            cardsOnTable = true;
            showdown();
         } else {
            for(int i = 0; i < playersLeft; i++) {
               if(!player[i].hasFolded()) {
                  playerWon(i, getPot());
               }
            }
            continue;
         }
      }


      // flop
      deck.getCard();
      for(int i = 0; i < 3; i++) {
         board[i] = deck.getCard();;
      }

      msg_len = pack(msg, "bbbb", 31, board[0], board[1], board[2]);
      if(broadcast(msg, msg_len) == -1)
         log.log(ERROR, "ERROR: broadcasting flop");

      log.log(HAND, "Flop: %d %d %d", board[0], board[1], board[2]);

      // betting round 2
      if(!cardsOnTable) {
         turn = (dealer + 1) % playersLeft;
         bettingRound(turn);
      }

      // check remaining players / allins
      if(getPlayersInHand() <= 1) {
         if(getPlayersAllin() > 0) {
            if(!cardsOnTable) {
               cardsOnTable = true;
               showdown();
            }
         } else {
            for(int i = 0; i < playersLeft; i++) {
               if(!player[i].hasFolded()) {
                  playerWon(i, getPot());
               }
            }
            continue;
         }
      }

      // turn
      deck.getCard();
      board[3] = deck.getCard();
      msg_len = pack(msg, "bb", 32, board[3]);
      if(broadcast(msg, msg_len) == -1)
         log.log(ERROR, "ERROR: broadcasting turn");

      log.log(HAND, "Turn: %d", board[3]);

      // betting round 3
      if(!cardsOnTable) {
         turn = (dealer + 1) % playersLeft;
         bettingRound(turn);
      }

       // check remaining players / allins
      if(getPlayersInHand() <= 1) {
         if(getPlayersAllin() > 0) {
            if(!cardsOnTable) {
               cardsOnTable = true;
               showdown();
            }
         } else {
            for(int i = 0; i < playersLeft; i++) {
               if(!player[i].hasFolded()) {
                  playerWon(i, getPot());
               }
            }
            continue;
         }
      }

      // river
      deck.getCard();
      board[4] = deck.getCard();
      msg_len = pack(msg, "bb", 33, board[4]);
      if(broadcast(msg, msg_len) == -1)
         log.log(ERROR, "ERROR: broadcasting river");

      log.log(HAND, "River: %d", board[4]);


      // betting round 4
      if(!cardsOnTable) {
         turn = (dealer + 1) % playersLeft;
         bettingRound(turn);
      }

       // check remaining players / allins
      if(getPlayersInHand() == 1 && getPlayersAllin() == 0) {
         for(int i = 0; i < playersLeft; i++) {
            if(!player[i].hasFolded()) {
               playerWon(i, getPot());
            }
         }
         continue;
      }

      if(!cardsOnTable)
         showdown();

      // get hand rankings
      eval.setBoard(board);
      for(int i = 0; i < playersLeft; i++) {
         if(!player[i].hasFolded()) {
            tmpHole[0] = player[i].getHoleCard(0);
            tmpHole[1] = player[i].getHoleCard(1);
            handrank[i] = eval.getHandrank(tmpHole);
         } else
            handrank[i] = 0;
      }

      // search winning hand (sort for pot distribution?)
      for(int i = 0; i < playersLeft; i++)
         winner[i] = i;

      sorted = false;
      while(!sorted) {
         sorted = true;
         for(int i = 0; i < playersLeft - 1; i++) {
            if(handrank[winner[i]] < handrank[winner[i+1]]) {
               sorted = false;
               tmp = winner[i];
               winner[i] = winner[i+1];
               winner[i+1] = tmp;
            }
         }
      }

      for(int i = 0; i < playersLeft; i++)
         log.log(HAND, "Player %d has %d", player[winner[i]].getNumber(), handrank[winner[i]]);


      // distribute pot
      pot = getPot();
      log.log(POT, "Pot: %d", pot);

      tmp = 0;
      sidepot = 0;
      distributedpot = 0;

      while(pot > 0) {
         // next sidepot
         minBet = 999999999;
         for(int i = 0; i < playersLeft; i++)
            if(!player[i].hasFolded())
               if(player[i].getCurrentBet() > tmp && player[i].getCurrentBet() < minBet)
                  minBet = player[i].getCurrentBet();


         tmp = minBet;
         tmppot = 0;
         for(int i = 0; i < playersLeft; i++) {
            if(player[i].getCurrentBet() > tmp)
               tmppot += tmp;
            else
               tmppot += player[i].getCurrentBet();
         }

         sidepot = tmppot - distributedpot;
         distributedpot = tmppot;
         
         // the best hand in pot
         int first;
         for(int i = 0; i < playersLeft; i++)
            if(player[winner[i]].getCurrentBet() >= tmp) {
               first = i;
               break;
            }

         // count equal hands in pot
         count = 0;
         int j = first;
         while(j < playersLeft) {
            if(handrank[winner[first]] == handrank[winner[j]]) {
               if(player[winner[j]].getCurrentBet() >= tmp)
                  count++;
            } else {
               break;
            }
           j++;
         }

         // distribute sidepot
         int k = 0;
         j = first;

         while(k < count) {
            if(player[winner[j]].getCurrentBet() >= tmp) {
               playerWon(winner[j], sidepot / count);
               k++;
            }
            j++;
         }

         pot -= sidepot;
      }



      // eliminate players (move eliminated players to the end)
      for(int i = 0; i < playersLeft; i++) {
         log.log(POT, "Player %d has %d chips", player[i].getNumber(), player[i].getRemainingChips());
         if(player[i].getRemainingChips() == 0) {
            msg_len = pack(msg, "bb", 13, player[i].getNumber());
            if(broadcast(msg, msg_len) == -1)
               log.log(ERROR, "ERROR: broadcasting eliminated player");
            eliminate(i);
            i--; // BAAAAAHH
            playersLeft--;
         }
      }

      // move dealer
      dealer = (dealer + 1) % playersLeft;
   }

   return 0;

}

int server::bettingRound(int n) {
   int count = 0;
   int success = -1;
   int turn = n;

   while(getPlayersInHand() > 1 && count < playersLeft) {
      success = 0;
      if(!player[turn].hasFolded() && !player[turn].isAllin()) {
         // empty socket, only latest action counts
         clear(turn);
         requestAction(turn);
         if((success = readNext(turn)) == -1)
            playerFolded(turn);

      }

      turn = (turn + 1) % playersLeft;
      if(success == 2)
         count = 1;
      else
         count++;
   }
   return 0;
}

int server::clear(int8_t n) {
   int fd = player[n].getFD();
   fd_set readfds;
   struct timeval tv;
   tv.tv_sec = 0;
   tv.tv_usec = 0;

   FD_ZERO(&readfds);
   FD_SET(fd, &readfds);
   bool empty = false;

   while(!empty) {
      empty = true;
      select(fd+1, &readfds, NULL, NULL, &tv);
      if(FD_ISSET(fd, &readfds)) {
         recv(fd, NULL, 64, 0);
         empty = false;
      }
   }
   return 0;
}

int server::showdown() {
   unsigned char msg[5];
   int msg_len;
   for(int i = 0; i < playersLeft; i++) {
      if(!player[i].hasFolded()) {
         msg_len = pack(msg, "bbbb", 50, player[i].getNumber(), player[i].getHoleCard(0), player[i].getHoleCard(1));
         broadcast(msg, msg_len);
      }
   }

   return 0;
}

int server::getPot() {
   int p = 0;
   for(int i = 0; i < playersLeft; i++)
      p += player[i].getCurrentBet();

   return p;
}

int server::getPlayersInHand() {
   int c = 0;
   for(int i = 0; i < playersLeft; i++)
      if(!player[i].hasFolded() && !player[i].isAllin())
         c++;

   return c;
}

int server::getPlayersAllin() {
   int c = 0;
   for(int i = 0; i < playersLeft; i++)
      if(player[i].isAllin())
         c++;

   return c;
}

int server::requestAction(int8_t n) {
   unsigned char msg[2];
   int msg_len = pack(msg, "b", 40);
   if(sendAll(n, msg, msg_len) == -1) {
      log.log(ERROR, "ERROR: sending request to %d", n);
      return -1;
   }
   return 0;
}

int server::playerFolded(int8_t n) {
   log.log(VERBOSE, "Player %d folded", player[n].getNumber());

   player[n].folds();

   unsigned char msg[3];
   int msg_len = pack(msg, "bb", 41, player[n].getNumber());
   if(broadcast(msg, msg_len) == -1)
      log.log(ERROR, "ERROR: broadcasting fold");
   return 0;
}

// return 2 if raising success
int server::playerRaised(int8_t n, int16_t a) {
   unsigned char msg[5];
   int msg_len, tmp;

   if(toCall - player[n].getCurrentBet() >= player[n].getRemainingChips())
      return playerAllin(n);

   tmp = a;

   if(tmp < minimumBet) {
      if((toCall - player[n].getCurrentBet() + minimumBet) >= player[n].getRemainingChips()) {
         return playerAllin(n);
      } else {
         tmp = minimumBet;
      }
   }

   if(toCall - player[n].getCurrentBet() + tmp >= player[n].getRemainingChips())
      return playerAllin(n);


   log.log(VERBOSE, "Player %d raised by %d", player[n].getNumber(),  a);

   minimumBet = tmp;
   toCall += tmp;
   player[n].bets(toCall - player[n].getCurrentBet());

   msg_len = pack(msg, "bbh", 42, player[n].getNumber(), a);
   if(broadcast(msg, msg_len) == -1)
      log.log(ERROR, "ERROR: broadcasting raise");
   return 2;
}

int server::playerCalled(int8_t n) {

   if(toCall - player[n].getCurrentBet() >= player[n].getRemainingChips())
      return playerAllin(n);


   log.log(VERBOSE, "Player %d called", player[n].getNumber());

   player[n].bets(toCall - player[n].getCurrentBet());

   unsigned char msg[3];
   int msg_len = pack(msg, "bb", 43, player[n].getNumber());
   if(broadcast(msg, msg_len) == -1)
      log.log(ERROR, "ERROR: broadcasting call");
   return 0;
}

int server::playerChecked(int8_t n) {
   if(player[n].getCurrentBet() < toCall)
      return playerCalled(n);

   log.log(VERBOSE, "Player %d checked", player[n].getNumber());

   unsigned char msg[3];
   int msg_len = pack(msg, "bb", 44, player[n].getNumber());
   if(broadcast(msg, msg_len) == -1)
      log.log(ERROR, "ERROR: broadcasting check");
   return 0;
}

int server::playerAllin(int8_t n) {
   int raised = 0;
   int tmp;

   log.log(VERBOSE, "Player %d is allin", player[n].getNumber());

   if(toCall - player[n].getCurrentBet() < player[n].getRemainingChips()) {
      raised = 2;
      // added to Pot = remainingChips - (toCall - currentBet)
      tmp = player[n].getRemainingChips() - (toCall - player[n].getCurrentBet());
      toCall += tmp;
      if(tmp > minimumBet)
         minimumBet = tmp;
   }

   player[n].allin();

   unsigned char msg[5];
   int msg_len = pack(msg, "bbh", 45, player[n].getNumber(), player[n].getCurrentBet());
   if(broadcast(msg, msg_len) == -1)
      log.log(ERROR, "ERROR: broadcasting allin");
   return raised;
}

int server::playerBailed(int8_t n) {
   log.log(VERBOSE, "Player %d bailed", player[n].getNumber());

   removePlayer(n);

   unsigned char msg[3];
   int msg_len = pack(msg, "bb", 14, player[n].getNumber());
   if(broadcast(msg, msg_len) == -1)
      log.log(ERROR, "ERROR: broadcasting bail");
   return 0;
}

int server::playerWon(int8_t n, int16_t amount) {
   log.log(POT, "Player %d wins %d", player[n].getNumber(), amount);
   player[n].wins(amount);

   unsigned char msg[5];
   int msg_len = pack(msg, "bbh", 52, player[n].getNumber(), amount);
   broadcast(msg, msg_len);

   return 0;
}

void server::removePlayer(int8_t n) {
   for(int i = n; i < playerCount - 1; i++)
      player[i] = player[i+1];

   playerCount--;
}

void server::eliminate(int8_t n) {
   log.log(POT, "Player %d has been eliminated", player[n].getNumber());
   seat tmp = player[n];
   for(int i = n; i < playerCount - 1; i++)
      player[i] = player[i+1];
      
   player[playerCount - 1] = tmp;
}

int main(int argc, char *argv[]) {

   const char *port = "3031";
   int playerCount = 3;

   // read commandline
   for(int i = 1; i < argc; i+=2) {
      if(strcmp(argv[i], "-n") == 0) {
         playerCount = atoi(argv[i+1]);
         if(playerCount > 8 || playerCount < 2) {
            playerCount = 3;
         }
      }
   }

   server myServer(port, playerCount);

   myServer.startGame();
   
}
