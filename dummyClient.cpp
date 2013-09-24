
#include <fstream>
#include "client.h"
using namespace std;

class dummyClient : public client {

public:
   dummyClient(const char *name) : client(name) {};
   ~dummyClient() {};

protected:
   void placeBet();
};

void dummyClient::placeBet() {
   fprintf(stdout, "Your turn\n");
   fold();
}


int main() {

   dummyClient skazz("skazz");
   skazz.connectToServer("127.0.0.1", "3031");

}
