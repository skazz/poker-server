#include "handEvaluator.h"


using namespace std;

handEvaluator::handEvaluator() {

   for(int i = 0; i < 4; i++)
      suit[i] = 0;

   for(int i = 0; i < 13; i++)
      value[i] = 0;

   HOCH2 = 13*13;
   HOCH3 = HOCH2*13;
   HOCH4 = HOCH3*13;

}

int handEvaluator::setBoard(int8_t *_board) {
   if(sizeof board < 5)
      return -1;

   board = _board;

   for(int i = 0; i < 4; i++)
      suit[i] = 0;

   for(int i = 0; i < 13; i++)
      value[i] = 0;

   for(int i = 0; i < 5; i++)
      kicker[i] = -1;


   for(int i = 0; i < 5; i++) {
      suit[board[i] % 4]++;
      value[board[i] / 4]++;
   }

   return 0;
}

int handEvaluator::getHandrank(int8_t *_holeCard) {
   if(sizeof holeCard < 2)
      return -1;

   int handrank;
   holeCard = _holeCard;
   
   for(int i = 0; i < 2; i++) {
      suit[holeCard[i] % 4]++;
      value[holeCard[i] / 4]++;
   }

   handrank = evaluate();

   for(int i = 0; i < 2; i++) {
      suit[holeCard[i] % 4]--;
      value[holeCard[i] / 4]--;
   }

   return handrank;
}

int handEvaluator::evaluate() {
   int fours = -1;
   int threes = -1;
   int highPair = -1;
   int lowPair = -1;
   int straight = -1;

   int flush[5];
   bool gotFlush = false;


   // flush
   for(int i = 0; i < 4; i++)
      if(suit[i] >= 5) {

         gotFlush = true;

         int tmp[13];
         for(int j = 0; j < 13; j++)
            tmp[j] = 0;

         for(int j = 0; j < 5; j++)
            if(board[j] % 4 == i)
               tmp[board[j] / 4] = 1;

         for(int j = 0; j < 2; j++)
            if(holeCard[j] % 4 == i)
               tmp[holeCard[j] / 4] = 1;

         int count = 0;
         int flushIndex = 0;
         for(int j = 12; j >= 0; j--) {

            
            if(tmp[j] == 1) {
               if(flushIndex < 5) {
                  flush[flushIndex] = j;
                  flushIndex++;
               }
               count++;
            }
            else
               count = 0;

            // straight flush
            if(count == 5)
               return 840541 + j + 4;

         }
         // ace - 5
         if(count == 4 && tmp[12] == 1)
            return 840541 + 3;
      }

   // scanning values
   int kickerIndex = 0;
   int count = 0;
   for(int i = 12; i >= 0; i--) {

      
      if(value[i] > 0) {
         if(kickerIndex < 5) {
            kicker[kickerIndex] = i;
            kickerIndex++;
         }
         count++;
      }
      else
         count = 0;

      if(straight == -1 && count == 5)
         straight = i+4;

      if(fours == -1 && value[i] == 4) {
         fours = i;

      } else if(value[i] == 3) {
         // two trips make a full house...
         if(threes == -1)
            threes = i;
         else if(highPair == -1)
            highPair = i;
         else if(lowPair == -1)
            lowPair = i;
         
      } else if(value[i] == 2) {
         if(highPair == -1) {
            highPair = i;
         } else if (lowPair == -1) {
            lowPair = i;
         }
      }
   }

   // straight ace - 5
   if(count == 4 && value[12] > 0)
      straight = 3;

   
   // return highest hand(straight flush is above)
   if(fours != -1) {
      removeFromKicker(fours);
      return 840359 + fours*13 + kicker[0];
   }
   else if(threes != -1 && highPair != -1) {
      removeFromKicker(threes);
      removeFromKicker(highPair);
      return 840177 + threes*13 + highPair;
   }
   else if(gotFlush)
      return 437944 + flush[0]*HOCH4 + flush[1]*HOCH3 + flush[2]*HOCH2 + flush[3]*13 + flush[4];
   else if(straight != -1)
      return 437931 + straight;
   else if(threes != -1) {
      removeFromKicker(threes);
      return 435552 + threes*HOCH2 + kicker[0]*13 + kicker[1];
   }
   else if(highPair != -1 && lowPair != -1) {
      removeFromKicker(highPair);
      removeFromKicker(lowPair);
      return 433173 + highPair*HOCH2 + lowPair*13 + kicker[0];
   }
   else if(highPair != -1) {
      removeFromKicker(highPair);
      return 402233 + highPair*HOCH3 + kicker[0]*HOCH2 + kicker[1]*13 + kicker[2];
   }
   else {
      return kicker[0]*HOCH4 + kicker[1]*HOCH3 + kicker[2]*HOCH2 + kicker[3]*13 + kicker[4];
   }

}


int handEvaluator::removeFromKicker(int a) {
   for(int i = 0; i < 5; i++) {
      if(kicker[i] == a) {
         for(int j = i; j < 4; j++) {
            kicker[j] = kicker[j+1];
         }
      }
   }
   return 0;
}
