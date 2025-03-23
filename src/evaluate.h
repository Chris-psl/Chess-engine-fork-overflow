#ifndef EVALUATE
#define EVALUATE

#include "init.h"

// here we will define values for all pieces that can be changed depending on our strategy
#define P_VALUE 10 // pawn
#define N_VALUE 30 // knight
#define B_VALUE 35 // bishop
#define R_VALUE 50 // rook
#define Q_VALUE 90 // queen
#define K_VALUE 500 // king (huge value to protect at all costs)
#define BONUS_K_VALUE 200 // bonus points for king in special scenarios like when ruining castling

//Global attacked square penalty
#define ATTACKED_SQUARE_PENALTY 30

// A Number that if there are less pieces left for any player, endgame strategy activates
#define PIECES_ENDGAME 17

// Function to evaluate a single move
int evaluateBitboard(Board board);

#endif