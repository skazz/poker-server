#include "seat.h"

seat::seat() {};

seat::seat(int8_t n, int chips) {
   seatNumber = n;
   remainingChips = chips;
}

int seat::dealHoleCard(int n, int8_t card) {
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

int seat::allin() {
   bets(remainingChips);
   allined = true;
}

bool seat::hasFolded() {
   return folded;
}

int seat::newRound() {
   folded = false;
   allined = false;
   currentBet = 0;
}

seat::~seat() {}
