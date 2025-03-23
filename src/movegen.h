#ifndef MOVEGEN
#define MOVEGEN

#include "init.h"
#include "capture.h" // Library used to generate attack moves

// Function to generate Passive Piece Moves
char *generatePawnMoves(Board board);
char *generateBishopMoves(Board board);
char *generateKnightMoves(Board board);
char *generateRookMoves(Board board);
char *generateQueenMoves(Board board);
char *generateKingMoves(Board board);
char *generateAllMoves(Board board);
int updateEnPassant(Board board, const char *move);
char *generateMoveMoves(Board board, const char *moves);

#endif