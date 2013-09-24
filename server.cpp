#include "server.h"

using namespace std;

server::server(const char *port, int playerCount) {

   this->port = port;
   this->playerCount = playerCount;
   playersLeft = playerCount;

   startingChips = 20000;
   smallBlind = 400;
   bigBlind = 800;

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

   int dealer = 0; // rng
   bool blindsChanged = true;

   deckC deck;
   int8_t flop[3];
   int8_t river, turn;


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

      msg_len = pack(msg, "bb", 22, player[(dealer + 1) % playersLeft].getNumber());
      if(broadcast(msg, msg_len) == -1)
         fprintf(stderr, "error broadcasting small blind");

      msg_len = pack(msg, "bb", 23, player[(dealer + 2) % playersLeft].getNumber());
      if(broadcast(msg, msg_len) == -1)
         fprintf(stderr, "error broadcasting big blind");



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

      // flop
      deck.getCard();
      for(int i = 0; i < 3; i++)
         flop[i] = deck.getCard();

      msg_len = pack(msg, "bbbb", 31, flop[0], flop[1], flop[2]);
      if(broadcast(msg, msg_len) == -1)
         fprintf(stderr, "error broadcasting flop\n");

      // betting round 2

      // river
      deck.getCard();
      river = deck.getCard();
      msg_len = pack(msg, "bb", 32, river);
      if(broadcast(msg, msg_len) == -1)
         fprintf(stderr, "error broadcasting river\n");

      // betting round 3

      // turn
      deck.getCard();
      turn = deck.getCard();
      msg_len = pack(msg, "bb", 33, turn);
      if(broadcast(msg, msg_len) == -1)
         fprintf(stderr, "error broadcasting turn\n");


      // betting round 4

      // search winning hand(s)

      // distribute pot

      // eliminate players (move eliminated players to the end)
      msg_len = pack(msg, "bb", 13, player[1].getNumber());
      if(broadcast(msg, msg_len) == -1)
         fprintf(stderr, "error broadcasting eliminated player");
      eliminate(1);
      playersLeft--; // till its ready


      // move dealer
      dealer = (dealer + 1) % playersLeft;
   }

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
