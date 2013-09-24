#include "seat.h"

seat::seat() {};

seat::seat(int chips) {
   remainingChips = chips;
}

int seat::dealHoleCard(int n, int16_t card) {
   holeCard[n] = card;
}

int seat::bets(int n) {
   remainingChips -= n;
   currentBet += n;
}

int seat:: wins(int n) {
   remainingChips += n;
}

int seat::folds() {
   folded = true;
}

bool seat::hasFolded() {
   return folded;
}

int seat::newRound() {
   folded = false;
   currentBet = 0;
}

seat::~seat() {}
