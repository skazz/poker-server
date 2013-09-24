#include <inttypes.h>

class seat {

public:
   seat();

   seat(int chips);

   int newRound();

   int dealHoleCard(int n, int16_t card);

   int16_t getHoleCard(int n) { return holeCard[n]; };

   int bets(int n);

   int wins(int n);

   int folds();

   bool hasFolded();

   ~seat();

private:
   int16_t holeCard[2];

   int currentBet;

   int remainingChips;

   bool folded;

};
