// handEvaluator returns handranks (enumeration in ./handranks)

#include <inttypes.h>

class handEvaluator {

public:
   handEvaluator();

   int setBoard(int8_t *_board);

   int getHandrank(int8_t *_holeCard);

private:
   int evaluate();

   int removeFromKicker(int a);

   int8_t *board;

   int8_t *holeCard;

   int suit[4];

   int value[13];

   int kicker[5];
};
