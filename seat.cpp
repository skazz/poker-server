#include "seat.h"

seat::seat() {};

seat::seat(int8_t n, int chips) {
   seatNumber = n;
   remainingChips = chips;
}

void seat::dealHoleCard(int n, int8_t card) {
   holeCard[n] = card;
}

void seat::bets(int n) {
   remainingChips -= n;
   currentBet += n;
}

void seat:: wins(int n) {
   remainingChips += n;
}

void seat::folds() {
   folded = true;
}

void seat::allin() {
   bets(remainingChips);
   allined = true;
}

bool seat::hasFolded() {
   return folded;
}

void seat::newRound() {
   folded = false;
   allined = false;
   currentBet = 0;
}

seat::~seat() {}
