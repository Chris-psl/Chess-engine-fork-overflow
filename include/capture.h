#ifndef CAPTURE
#define CAPTURE

#include "init.h"
#include <stddef.h>

//Function to check if a square is attacked
int isSquareAttacked(Board board, int square);
int KingSquare(Board board);
int isKingAttacked(Board board);

//Functions to return legal moves based on the application
int appendString(char **buffer, size_t *size, size_t *len, const char *str);

// Filters moves based on legality
char *LegalALLMoves(Board board, const char *moves);
char *LegalAttackMoves(Board board, const char *moves);
char *LegalMoves(Board board, const char *moves);

// Converts a square index to algebraic notation
void squareToAlgebraic(short int square, char *buffer);

// Removes corrupted moves from the input string
char *filter_valid_moves(const char *input);

// Checks the validity of a move
int is_valid_move(const char *move);

//Function to generate possible capture moves
char *generatePawnCaptures(Board board);
char *generateBishopCaptures(Board board);
char *generateKnightCaptures(Board board);
char *generateRookCaptures(Board board);
char *generateQueenCaptures(Board board);
char *generateKingCaptures(Board board);
char *generateAllCaptures(Board board);
char *generateLegalCaptures(Board board);
char *generateLegalMoves(Board board);

#endif
