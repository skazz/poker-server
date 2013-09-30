#include "client.h"

using namespace std;

client::client(const char *_name) {
   name = _name;
}

client::~client() {
   close(sockfd);
}

int client::connectToServer(const char *remoteHost, const char *port) {
   struct addrinfo hints, *res;
   int status;

   memset(&hints, 0, sizeof hints);
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;

   if((status = getaddrinfo(remoteHost, port, &hints, &res)) != 0)
      return -1;

   if((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0)
      return -1;

   if((status = connect(sockfd, res->ai_addr, res->ai_addrlen)) != 0)
      return -1;

   return gameLoop();
}


int client::fold() {
   unsigned char msg[2];
   int msg_len;

   msg_len = pack(msg, "b", 1);
   return sendAll(sockfd, msg, msg_len);
}

int client::raise(int16_t amount) {
   unsigned char msg[4];
   int msg_len;

   msg_len = pack(msg, "bh", 2, amount);
   return sendAll(sockfd, msg, msg_len);
}

int client::call() {
   unsigned char msg[2];
   int msg_len;

   msg_len = pack(msg, "b", 3);
   return sendAll(sockfd, msg, msg_len);
}

int client::check() {
   unsigned char msg[2];
   int msg_len;
   
   msg_len = pack(msg, "b", 4);
   return sendAll(sockfd, msg, msg_len);
}

int client::allin() {
   unsigned char msg[2];
   int msg_len;

   msg_len = pack(msg, "b", 5);
   return sendAll(sockfd, msg, msg_len);
}

int client::bail() {
   unsigned char msg[2];
   int msg_len;

   msg_len = pack(msg, "b", 6);
   return sendAll(sockfd, msg, msg_len);
}

// specs in ./list_of_actions
int client::read(unsigned char *buf) {

   unsigned char *p = buf;
   int8_t a, b, c;
   int16_t d, f;
   unsigned char s[64];
   int8_t action = *p++;

   switch(action) {
   case 40:
      placeBet();
      return 0;
   case 0:
      //fprintf(stdout, "Did well\n");
      actionAccepted();
      return 0;
   case 41:
      unpack(p, "b", &a);
      //fprintf(stdout, "Player %"PRId8" folded\n", a);
      playerFolded(a);
      return 0;
   case 42:
      unpack(p, "bh", &a, &d);
      //fprintf(stdout, "Player %"PRId8" raised by %"PRId16"\n", a, d);
      playerRaised(a, d);
      return 0;
   case 43:
      unpack(p, "b", &a);
      //fprintf(stdout, "Player %"PRId8" called\n", a);
      playerCalled(a);
      return 0;
   case 44:
      unpack(p, "b", &a);
      //fprintf(stdout, "Player %"PRId8" checked\n", a);
      playerChecked(a);
      return 0;
   case 45:
      unpack(p, "bh", &a, &d);
      //fprintf(stdout, "Player %"PRId8" is Allin for %"PRId16"\n", a, d);
      playerAllin(a, d);
      return 0;
   case 50:
      unpack(p, "bbb", &a, &b, &c);
      //fprintf(stdout, "Player %"PRId8" got %"PRId8" and %"PRId8"\n", a, b, c);
      playerHolecards(a, b, c);
      return 0;
   case 30:
      unpack(p, "bb", &a, &b);
      //fprintf(stdout, "You got %" PRId8 " and %" PRId8 "\n", a, b);
      setHolecards(a, b);
      return 0;
   case 31:
      unpack(p, "bbb", &a, &b, &c);
      //fprintf(stdout, "Flop : %"PRId8" %"PRId8" %"PRId8"\n", a, b, c);
      setFlop(a, b, c);
      return 0;
   case 32:
      unpack(p, "b", &a);
      //fprintf(stdout, "Turn : %"PRId8"\n", a);
      setTurn(a);
      return 0;
   case 33:
      unpack(p, "b", &a);
      //fprintf(stdout, "River: %"PRId8"\n", a);
      setRiver(a);
      return 0;
   case 21:
      unpack(p, "b", &a);
      //fprintf(stdout, "Player %"PRId8" is dealer\n", a);
      playerIsDealer(a);
      return 0;
   case 22:
      unpack(p, "b", &a);
      //fprintf(stdout, "Player %"PRId8" is small blind\n");
      playerIsSmallBlind(a);
      return 0;
   case 23:
      unpack(p, "b", &a);
      //fprintf(stdout, "Player %"PRId8" is big blind\n");
      playerIsBigBlind(a);
      return 0;
   case 20:
      unpack(p, "hh", &d, &f);
      //fprintf(stdout, "The blinds are %" PRId16 "/%" PRId16 "\n", d, f);
      setBlinds(d, f);
      return 0;
   case 52:
      unpack(p, "bh", &a, &d);
      //fprintf(stdout, "Player %"PRId8" won %"PRId16"\n", a, d);
      playerWon(a, d);
      return 0;
   case 51:
      //fprintf(stdout, "Tie\n");
      tie();
      return 0;
   case 1:
      //fprintf(stdout, "Dont know this move\n");
      actionUnknown();
      return 0;
   case 2:
      //fprintf(stdout, "Not allowed\n");
      actionNotAllowed();
      return 0;
   case 3:
      //fprintf(stdout, "Bet too low\n");
      betTooLow();
      return 0;
   case 11:
      unpack(p, "bs", &a, &s);
      //fprintf(stdout, "%s is Player %" PRId8 "\n", s, a);
      setPlayerName(a, s);
      return 0;
   case 10:
      unpack(p, "b", &a);
      //fprintf(stdout, "There are %" PRId8 " Players ingame.\n", a);
      setPlayerCount(a);
      return 0;
   case 12:
      unpack(p, "b", &a);
      //fprintf(stdout, "You are Player %" PRId8 "\n", a);
      setSeatNumber(a);
      return 0;
   case 13:
      unpack(p, "b", &a);
      //fprintf(stdout, "Player %" PRId8 " has been eliminated\n", a);
      playerEliminated(a);
      return 0;
   case 14:
      unpack(p, "b", &a);
      //fprintf(stdout, "Player %" PRId8 " has left\n", a);
      playerLeft(a);
      return 0;
   case 15:
      unpack(p, "h", &d);
      //fprintf(stdout, "You start with %" PRId16 " chips\n", d);
      setStartingChips(d);
      return 0;
   default:
      return -1;
   }

}

int client::gameLoop() {
   unsigned char msg[64];
   int msg_len;

   msg_len = pack(msg, "bs", 0, name);
   sendAll(sockfd, msg, msg_len);


   unsigned char buf[64];
   unsigned char *p;
   unsigned char tmp_buf[130];
   int tmp_offset = 0;
   int buf_len = sizeof buf;
   int bytes_received = 0;
   int8_t size;

   while((bytes_received = recv(sockfd, buf, buf_len, 0)) > 0) {
      
      //fprintf(stdout, "Received %" PRId8 " Bytes\n", bytes_received);

      memcpy(tmp_buf + tmp_offset, buf, bytes_received);
      tmp_offset += bytes_received;

      if(bytes_received < 1)
         continue;

      size = *tmp_buf;

      while(tmp_offset >= size + 1) {
         //fprintf(stdout, "Read %" PRId8 " Bytes\n", size + 1);

         p = tmp_buf + 1;
         read(p);

         tmp_offset -= (size + 1);

         memmove(tmp_buf, tmp_buf + size + 1, tmp_offset);

         if(tmp_offset > 0)
            size = *tmp_buf;
      }
   }

   return 0;
}

int client::sendAll(int fd, unsigned char *msg, int msg_len) {
   int bytes_left = msg_len;
   int bytes_send = 0;
   while(bytes_left > 0) {
      if((bytes_send = send(fd, msg + msg_len - bytes_left, bytes_left, 0)) == -1)
         return -1;
      bytes_left = bytes_left - bytes_send;
   }
   return 0;
}
