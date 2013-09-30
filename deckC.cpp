#include "deckC.h"

deckC::deckC() {
   // init rng
   time_t seconds;
   time(&seconds);
   srand((unsigned int) seconds);
}

int deckC::shuffle() {
   cardsLeft = 52;
   for(int8_t i = 0; i < 52; i++)
      cards[i] = i;

   return 0;
}

int8_t deckC::getCard() {
   int index = rand() % cardsLeft; // rng
   int8_t card = cards[index];
   memmove(cards + index, cards + index + 1, cardsLeft - index - 1);
   cardsLeft--;
   return card;
}
