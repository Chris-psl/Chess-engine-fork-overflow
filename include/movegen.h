#ifndef MOVEGEN
#define MOVEGEN

#include "init.h"
#include "capture.h" // Library used to generate attack moves

// Function to generate non capture Piece Moves
char *generatePawnMoves(Board board);
char *generateBishopMoves(Board board);
char *generateKnightMoves(Board board);
char *generateRookMoves(Board board);
char *generateQueenMoves(Board board);
char *generateKingMoves(Board board);

// Final functions containing all moves
char *generateAllMoves(Board board);
char *generateMoveMoves(Board board, const char *moves);

// Update the en passant square based on the last move
int updateEnPassant(Board board, const char *move);

#endif
