
#include <fstream>
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
};

void dummyClient::placeBet() {
   check();
}

void dummyClient::setHolecards(int8_t c1, int8_t c2) {
   fprintf(stdout, "Hand : %"PRId8" and %"PRId8"\n", c1, c2);
}

void dummyClient::setFlop(int8_t c1, int8_t c2, int8_t c3) {
   fprintf(stdout, "Flop : %"PRId8" %"PRId8" %"PRId8"\n", c1, c2, c3);
}

void dummyClient::setTurn(int8_t c1) {
   fprintf(stdout, "Turn : %"PRId8"\n", c1);
}

void dummyClient::setRiver(int8_t c1) {
   fprintf(stdout, "River: %"PRId8"\n", c1);
}

void dummyClient::playerIsDealer(int8_t n) {
   fprintf(stdout, "Player %"PRId8" is dealer\n", n);
}

void dummyClient::playerWon(int8_t n, int16_t amount) {
   fprintf(stdout, "Player %"PRId8" won %"PRId16"\n", n, amount);
}

int main() {

   dummyClient skazz("skazz");
   skazz.connectToServer("127.0.0.1", "3031");

}
