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

            // TODO: while no success..., timeout
            if(tmp_offset >= size + 1) {

               p = tmp_buf + 1;

               action = *p++;

               switch(action) {
               case 0:
                  unpack(p, "s", &s);
                  player[n].setName(s);
                  msg_len = pack(msg, "bbs", 11, player[n].getNumber(), s);
                  if(broadcast(msg, msg_len) == -1)
                     fprintf(stderr, "error broadcasting name\n");
                  
                  fprintf(stdout, "%s joined\n", player[n].getName());
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
      fprintf(stderr, "socket error\n");
      return -1;
   }

   for(int i = 0; i < playerCount; i++)
      player[i].setFD(client[i]);

   
   // get names
   for(int i = 0; i < playerCount; i++) {
      if(readNext(i) == -1)
         fprintf(stderr, "error reading names\n");
   }


   // broadcast playerCount
   unsigned char msg[64];
   int msg_len;
   msg_len = pack(msg, "bb", 10, playerCount);
   if(broadcast(msg, msg_len) == -1)
      fprintf(stderr, "error broadcasting playercount\n");


   
   

   // assign players their seat and tell others
   for(int i = 0; i < playerCount; i++) {
      msg_len = pack(msg, "bb", 12, player[i].getNumber());
      if(sendAll(i, msg, msg_len) == -1)
         fprintf(stderr, "error assigning seat to %d\n", i);

   }

   // broadcast starting chips
   msg_len = pack(msg, "bh", 15, startingChips);
   if(broadcast(msg, msg_len) == -1)
      fprintf(stderr, "error broadcasting starting chips\n");

   // commence
   return gameLoop();
   
}

int server::gameLoop() {
   unsigned char msg[16];
   int msg_len;

   int dealer = rand() % playerCount; // rng
   bool blindsChanged = true;
   bool cardsOnTable = false;

   bool sorted;
   int tmp, tmppot, minBet, count, sidepot, distributedpot;

   deckC deck;
   int8_t flop[3];
   int8_t river, turn;
   int8_t board[5];
   int8_t tmpHole[2];
   int winner[playerCount];
   int handrank[playerCount];

   handEvaluator eval;


   while(playersLeft > 1) {

      // shuffle deck
      deck.shuffle();


      // reset seats
      for(int i = 0; i < playersLeft; i++)
         player[i].newRound();


      // broadcast new blinds
      if(blindsChanged) {
         msg_len = pack(msg, "bhh", 20, smallBlind, bigBlind);
         if(broadcast(msg, msg_len) == -1)
            fprintf(stderr, "error broadcasting blinds\n");
         blindsChanged = false;
      }


      // tell dealer, small, big blinds (broadcast?)
      msg_len = pack(msg, "bb", 21, player[dealer].getNumber());
      if(broadcast(msg, msg_len) == -1)
         fprintf(stderr, "error broadcasting dealer");


      // small blind(check if enough chips)
      msg_len = pack(msg, "bb", 22, player[(dealer + 1) % playersLeft].getNumber());
      if(broadcast(msg, msg_len) == -1)
         fprintf(stderr, "error broadcasting small blind");

      if(player[(dealer + 1) % playersLeft].getRemainingChips() <= smallBlind) {
         playerAllin((dealer + 1) % playersLeft);
      } else {
         player[(dealer + 1) % playersLeft].bets(smallBlind);
      }

      // big blind(check if enough chips)
      msg_len = pack(msg, "bb", 23, player[(dealer + 2) % playersLeft].getNumber());
      if(broadcast(msg, msg_len) == -1)
            fprintf(stderr, "error broadcasting big blind");

      if(player[(dealer + 2) % playersLeft].getRemainingChips() <= bigBlind) {
         playerAllin((dealer + 2) % playersLeft);
      } else {
         player[(dealer + 2) % playersLeft].bets(bigBlind);
      }


      minimumBet = bigBlind;
      toCall = bigBlind;
      cardsOnTable = false;
      lastPlayerRaised = (dealer + 2) % playersLeft; // still weird reset counter in betting round


      // deal cards
      for(int i = 0; i < 2; i++) {
         for(int j = 0; j < playersLeft; j++) {
            player[j].dealHoleCard(i, deck.getCard());
         }
      }

      for(int i = 0; i < playersLeft; i++) {
         msg_len = pack(msg, "bbb", 30, player[i].getHoleCard(0), player[i].getHoleCard(1));
         if(sendAll(i, msg, msg_len) == -1)
            fprintf(stderr, "error sending cards to %d\n", i);
      }

      // betting round 1
      turn = (dealer + 3) % playersLeft;
      bettingRound();

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
         flop[i] = deck.getCard();
         board[i] = flop[i];
      }

      msg_len = pack(msg, "bbbb", 31, flop[0], flop[1], flop[2]);
      if(broadcast(msg, msg_len) == -1)
         fprintf(stderr, "error broadcasting flop\n");

      // betting round 2
      if(!cardsOnTable)
         bettingRound();

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
      turn = deck.getCard();
      board[3] = turn;
      msg_len = pack(msg, "bb", 32, turn);
      if(broadcast(msg, msg_len) == -1)
         fprintf(stderr, "error broadcasting turn\n");

      // betting round 3
      if(!cardsOnTable)
         bettingRound();

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
      river = deck.getCard();
      board[4] = river;
      msg_len = pack(msg, "bb", 33, river);
      if(broadcast(msg, msg_len) == -1)
         fprintf(stderr, "error broadcasting river\n");


      // betting round 4
      if(!cardsOnTable)
         bettingRound();

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
         fprintf(stdout, "Player %d has %d\n", player[winner[i]].getNumber(), handrank[winner[i]]);


      // distribute pot
      pot = getPot();

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

         fprintf(stdout, "best hand in pot %d %d times\n", handrank[winner[first]], count);
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
         fprintf(stdout, "Player %"PRId8" has %"PRId16" chips\n", player[i].getNumber(), player[i].getRemainingChips());
         if(player[i].getRemainingChips() == 0) {
            msg_len = pack(msg, "bb", 13, player[i].getNumber());
            if(broadcast(msg, msg_len) == -1)
               fprintf(stderr, "error broadcasting eliminated player");
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

int server::bettingRound() {
   int count = 0;
   int success = -1;

   while(getPlayersInHand() > 1 && (lastPlayerRaised != turn || count < playersLeft)) {
      if(!player[turn].hasFolded() && !player[turn].isAllin()) {
         // empty socket, only latest action counts
         clear(turn);
         requestAction(turn);
         if((success = readNext(turn)) != 0)
            playerFolded(turn);

      }

      turn = (turn + 1) % playersLeft;
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
      fprintf(stderr, "error sending request to %"PRId8"\n", n);
      return -1;
   }
   return 0;
}

int server::playerFolded(int8_t n) {
   fprintf(stdout, "Player %"PRId8" folded\n", player[n].getNumber());

   player[n].folds();

   unsigned char msg[3];
   int msg_len = pack(msg, "bb", 41, player[n].getNumber());
   if(broadcast(msg, msg_len) == -1)
      fprintf(stderr, "error broadcasting fold\n");
   return 0;
}

int server::playerRaised(int8_t n, int16_t a) {
   fprintf(stdout, "Player %"PRId8" tried to raise by %"PRId16"\n", player[n].getNumber(), a);

   unsigned char msg[5];
   int msg_len;
   if(a < minimumBet) {
      msg_len = pack(msg, "b", 3);
      if(sendAll(n, msg, msg_len) == -1)
         fprintf(stderr, "error sending \"bet too low\" to %"PRId8"\n", player[n].getNumber());
      return -1;
   } else if((toCall - player[n].getCurrentBet() + a) > player[n].getRemainingChips()) {
      return playerAllin(n);
   }


   fprintf(stdout, "Player %"PRId8" raised by %"PRId16"\n", player[n].getNumber(), a);

   minimumBet = a;
   toCall += a;
   player[n].bets(toCall - player[n].getCurrentBet());
   lastPlayerRaised = n;

   msg_len = pack(msg, "bbh", 42, player[n].getNumber(), a);
   if(broadcast(msg, msg_len) == -1)
      fprintf(stderr, "error broadcasting raise\n");
   return 0;
}

int server::playerCalled(int8_t n) {
   fprintf(stdout, "Player %"PRId8" tried to call\n", player[n].getNumber());

   if(toCall - player[n].getCurrentBet() >= player[n].getRemainingChips())
      return playerAllin(n);


   fprintf(stdout, "Player %"PRId8" called\n", player[n].getNumber());

   player[n].bets(toCall - player[n].getCurrentBet());

   unsigned char msg[3];
   int msg_len = pack(msg, "bb", 43, player[n].getNumber());
   if(broadcast(msg, msg_len) == -1)
      fprintf(stderr, "error broadcasting call\n");
   return 0;
}

int server::playerChecked(int8_t n) {
   if(player[n].getCurrentBet() < toCall)
      return playerCalled(n);

   fprintf(stdout, "Player %"PRId8" checked\n", player[n].getNumber());

   unsigned char msg[3];
   int msg_len = pack(msg, "bb", 44, player[n].getNumber());
   if(broadcast(msg, msg_len) == -1)
      fprintf(stderr, "error broadcasting check\n");
   return 0;
}

int server::playerAllin(int8_t n) {
   fprintf(stdout, "Player %"PRId8" is allin\n", player[n].getNumber());

   player[n].allin();

   unsigned char msg[5];
   int msg_len = pack(msg, "bbh", 45, player[n].getNumber(), player[n].getCurrentBet());
   if(broadcast(msg, msg_len) == -1)
      fprintf(stderr, "error broadcasting allin\n");
   return 0;
}

int server::playerBailed(int8_t n) {
   fprintf(stdout, "Player %"PRId8" bailed\n", player[n].getNumber());

   removePlayer(n);

   unsigned char msg[3];
   int msg_len = pack(msg, "bb", 14, player[n].getNumber());
   if(broadcast(msg, msg_len) == -1)
      fprintf(stderr, "error broadcasting bail\n");
   return 0;
}

int server::playerWon(int8_t n, int16_t amount) {
   fprintf(stdout, "player %d bet %d wins %d\n", player[n].getNumber(), player[n].getCurrentBet(), amount);
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
      } else {
         fprintf(stderr, "unrecognized option %s\n", argv[i]);
      }
   }

   server myServer(port, playerCount);

   myServer.startGame();
   
}
