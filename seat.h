#include <inttypes.h>

class seat {

public:
   seat();

   seat(int8_t n, int chips);

   void newRound();

   void dealHoleCard(int n, int8_t card);

   int8_t getHoleCard(int n) { return holeCard[n]; };

   int8_t getNumber() { return seatNumber; };

   int getRemainingChips() { return remainingChips; };

   int getCurrentBet() { return currentBet; };

   void bets(int n);

   void wins(int n);

   void folds();

   void allin();

   bool hasFolded();

   bool isAllin() { return allined; };

   void setFD(int n) { fd = n; };

   int getFD() { return fd; };

   void setName(unsigned char *_name) { name = _name; };

   unsigned char* getName() { return name; };

   ~seat();

private:
   unsigned char *name;

   int8_t seatNumber;

   int fd;

   int8_t holeCard[2];

   int currentBet;

   int remainingChips;

   bool folded, allined;

};
