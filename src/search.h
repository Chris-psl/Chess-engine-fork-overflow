#ifndef SEARCH
#define SEARCH

#include "bitboard.h"
#include "evaluate.h"
#include <stdbool.h>


double minimax(Board board, int depth, double alpha, double beta, bool maximizingPlayer);
double quiescence(Board board, double alpha, double beta);

#endif