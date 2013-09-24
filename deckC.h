#include <inttypes.h>
#include <cstdlib>
#include <time.h>
#include <string.h>

class deckC {

public:
   deckC();

   int8_t getCard();
   
   int shuffle();

private:
   int8_t cards[52];

   int cardsLeft;

};
