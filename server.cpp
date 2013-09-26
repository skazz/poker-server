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

   while((bytes_received = recv(fd, buf, buf_len, 0)) > 0) {
      //fprintf(stdout, "Received %d Bytes\n", bytes_received);

      memcpy(tmp_buf + tmp_offset, buf, bytes_received);
      tmp_offset += bytes_received;
      if(bytes_received < 1)
         continue;

      size = *tmp_buf;

      // TODO: while no success..., timeout
      if(tmp_offset >= size + 1) {
         //fprintf(stdout, "Read %d Bytes\n", size + 1);

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

   
   cin.get();



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
   gameLoop();
   
}

int server::gameLoop() {
   unsigned char msg[16];
   int msg_len;

   int dealer = rand() % playerCount; // rng
   bool blindsChanged = true;

   deckC deck;
   int8_t flop[3];
   int8_t river, turn;


   while(playersLeft > 1) {

      pot = 0;

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
      playersInHand = playersLeft;
      lastPlayerRaised = (dealer + 3) % playersLeft; // BULLSHIT


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


      // flop
      deck.getCard();
      for(int i = 0; i < 3; i++)
         flop[i] = deck.getCard();

      msg_len = pack(msg, "bbbb", 31, flop[0], flop[1], flop[2]);
      if(broadcast(msg, msg_len) == -1)
         fprintf(stderr, "error broadcasting flop\n");

      // betting round 2
      bettingRound();

      // turn
      deck.getCard();
      turn = deck.getCard();
      msg_len = pack(msg, "bb", 32, turn);
      if(broadcast(msg, msg_len) == -1)
         fprintf(stderr, "error broadcasting turn\n");

      // betting round 3
      bettingRound();

      // river
      deck.getCard();
      river = deck.getCard();
      msg_len = pack(msg, "bb", 33, river);
      if(broadcast(msg, msg_len) == -1)
         fprintf(stderr, "error broadcasting river\n");


      // betting round 4
      bettingRound();

      // search winning hand(s)

      // distribute pot
      for(int i = 0; i < playersLeft; i++)
         pot += player[i].getCurrentBet();

      player[0].wins(pot);

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

}

int server::bettingRound() {
   int count = 0;
   int tries = 0;
   int success = -1;

   while(playersInHand > 1 && (lastPlayerRaised != turn || count < playersLeft)) {
      if(!player[turn].hasFolded() && !player[turn].isAllin()) {
         tries = 0;
         do {
            requestAction(turn);
            tries++;
         }
         while((success = readNext(turn) != 0) && tries < 3);

         if(success != 0)
            playerFolded(turn);

      }

      turn = (turn + 1) % playersLeft;
      count++;
   }
   return 0;
}

int server::requestAction(int8_t n) {
   unsigned char msg[2];
   int msg_len = pack(msg, "b", 40);
   if(sendAll(n, msg, msg_len) == -1)
      fprintf(stderr, "error sending request to %"PRId8"\n", n);
}

int server::playerFolded(int8_t n) {
   fprintf(stdout, "Player %"PRId8" folded\n", player[n].getNumber());

   player[n].folds();
   playersInHand--;

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
   playersInHand--;

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

int server::removePlayer(int8_t n) {
   for(int i = n; i < playerCount - 1; i++)
      player[i] = player[i+1];

   playerCount--;
}

int server::eliminate(int8_t n) {
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
   
   cin.get();

}
