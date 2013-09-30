#include <fstream>
#include <cstring>

#include "client.h"

using namespace std;

class dummyClient : public client {

public:
   dummyClient(const char *name) : client(name) {};
   ~dummyClient() {};

protected:
   void placeBet();

   void setHolecards(int8_t c1, int8_t c2);

   void setFlop(int8_t c1, int8_t c2, int8_t c3);

   void setTurn(int8_t c1);

   void setRiver(int8_t c1);

   void playerIsDealer(int8_t n);

   void playerWon(int8_t n, int16_t amount);

   void playerHolecards(int8_t n, int8_t c1, int8_t c2);

private:
   string getCardName(int8_t c);
};


void dummyClient::placeBet() {
   check();
}

void dummyClient::setHolecards(int8_t c1, int8_t c2) {
   fprintf(stdout, "Hand : %s | %s\n", getCardName(c1).c_str(), getCardName(c2).c_str());
}

void dummyClient::playerHolecards(int8_t n, int8_t c1, int8_t c2) {
   fprintf(stdout, "Player %d has %s | %s\n", n, getCardName(c1).c_str(), getCardName(c2).c_str());
}

void dummyClient::setFlop(int8_t c1, int8_t c2, int8_t c3) {
   fprintf(stdout, "Flop : %s | %s | %s\n", getCardName(c1).c_str(), getCardName(c2).c_str(), getCardName(c3).c_str());
}

void dummyClient::setTurn(int8_t c1) {
   fprintf(stdout, "Turn : %s\n", getCardName(c1).c_str());
}

void dummyClient::setRiver(int8_t c1) {
   fprintf(stdout, "River: %s\n", getCardName(c1).c_str());
}

void dummyClient::playerIsDealer(int8_t n) {
   fprintf(stdout, " Player %d is dealer\n", n);
}

void dummyClient::playerWon(int8_t n, int16_t amount) {
   fprintf(stdout, " Player %d won %d\n", n, amount);
}

string dummyClient::getCardName(int8_t c) {
   string suit[4] = { string("Spades"), string("Clubs"), string("Diamonds"), string("Hearts") };
   string value[13] = { string("Two"), string("Three"), string("Four"), string("Five"), string("Six"), string("Seven"), string("Eight"), string("Nine"), string("Ten"), string("Jack"), string("Queen"), string("King"), string("Ace") };

   return value[c % 13] + string(" of ") + suit[c / 13];
}

int main(int argc, char *argv[]) {
   const char *port = "3031";
   const char *ip = "127.0.0.1";

   if(argc > 1)
      ip = argv[1];
   if(argc > 2)
      port = argv[2];

   fprintf(stdout, "Connection to %s on %s\n", ip, port);

   dummyClient skazz("skazz");

   skazz.connectToServer(ip, port);

}
