#ifndef SEARCH
#define SEARCH

#include "bitboard.h"
#include "evaluate.h"
#include <stdbool.h>

// This function performs the minimax algorithm with alpha-beta pruning.
double minimax(Board board, int depth, double alpha, double beta, bool maximizingPlayer);

// Minmax extension for quiescence search
double quiescence(Board board, double alpha, double beta);

#endif
