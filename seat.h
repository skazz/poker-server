#include <inttypes.h>

class seat {

public:
   seat();

   seat(int8_t n, int chips);

   int newRound();

   int dealHoleCard(int n, int8_t card);

   int8_t getHoleCard(int n) { return holeCard[n]; };

   int8_t getNumber() { return seatNumber; };

   int getRemainingChips() { return remainingChips; };

   int getCurrentBet() { return currentBet; };

   int bets(int n);

   int wins(int n);

   int folds();

   int allin();

   bool hasFolded();

   bool isAllin() { return allined; };

   int setFD(int n) { fd = n; };

   int getFD() { return fd; };

   int setName(unsigned char *_name) { name = _name; };

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
