#include "server.h"

using namespace std;

server::server(const char *port, int playerCount) {

   this->port = port;
   this->playerCount = playerCount;
   playersLeft = playerCount;

   startingChips = 20000;
   smallBlind = 400;
   bigBlind = 800;

   for(int i = 0; i < playerCount; i++)
      player[playerCount] = seat(startingChips);

}

server::~server() {
   cleanup(client);
}

int server::sendAll(int fd, unsigned char *msg, int msg_len) {
   int bytes_left = msg_len;
   int bytes_send = 0;
   while(bytes_left > 0) {
      if((bytes_send = send(fd, msg + msg_len - bytes_left, bytes_left, 0)) == -1)
         return -1;

      bytes_left = bytes_left - bytes_send;
   }
   return 0;
}

int server::broadcast(int *fd, int count, unsigned char *msg, int msg_len) {
   for(int i = 0; i < count; i++) {
      if(sendAll(fd[i], msg, msg_len) == -1) {
         return -1;
      }
   }
   return 0;
}

int server::startGame() {

   if((setup(client, playerCount, port)) == -1) {
      fprintf(stderr, "socket error\n");
      return -1;
   }

   cin.get();

   // broadcast playerCount
   unsigned char msg[64];
   int msg_len;
   msg_len = pack(msg, "bb", 10, playerCount);
   if(broadcast(client, playerCount, msg, msg_len) == -1)
      fprintf(stderr, "error broadcasting playercount\n");


   // get names
   

   // assign players their seat and tell others
   for(int i = 0; i < playerCount; i++) {
      msg_len = pack(msg, "bb", 12, i);
      if(sendAll(client[i], msg, msg_len) == -1)
         fprintf(stderr, "error assigning seat to %d\n", i);
   }

   // broadcast starting chips
   msg_len = pack(msg, "bh", 15, startingChips);
   if(broadcast(client, playerCount, msg, msg_len) == -1)
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


   while(playersLeft > 1) {

      // shuffle deck
      deck.shuffle();


      // reset seats
      for(int i = 0; i < playersLeft; i++)
         player[i].newRound();


      // broadcast new blinds
      if(blindsChanged) {
         msg_len = pack(msg, "bhh", 20, smallBlind, bigBlind);
         if(broadcast(client, playerCount, msg, msg_len) == -1)
            fprintf(stderr, "error broadcasting blinds\n");
         blindsChanged = false;
      }


      // tell dealer, small, big blinds (broadcast?)


      // request action
      msg_len = pack(msg, "b", 40);
      if(sendAll(client[dealer], msg, msg_len) == -1)
         fprintf(stderr, "error sending request to 0\n");


      // deal cards
      for(int i = 0; i < 2; i++) {
         for(int j = 0; j < playersLeft; j++) {
            player[j].dealHoleCard(i, deck.getCard());
         }
      }

      for(int i = 0; i < playersLeft; i++) {
         msg_len = pack(msg, "bbb", 30, player[i].getHoleCard(0), player[i].getHoleCard(1));
         if(sendAll(client[i], msg, msg_len) == -1)
            fprintf(stderr, "error sending cards to %d\n", i);
      }


      // betting round 1

      // flop

      // betting round 2

      // river

      // betting round 3

      // turn

      // betting round 4

      // search winning hand(s)

      // distribute pot

      // eliminate players
      
      playersLeft--;
      dealer = (dealer + 1) % playersLeft;
   }

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
