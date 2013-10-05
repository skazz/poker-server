#include <inttypes.h>
#include <cstdlib>
#include <time.h>

class deckC {

public:
   deckC();

   int8_t getCard();
   
   void shuffle();

private:
   int8_t cards[52];

   int cardsLeft;

};
