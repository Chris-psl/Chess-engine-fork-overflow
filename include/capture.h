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
char *LegalALLMoves(Board board, const char *moves);
char *LegalAttackMoves(Board board, const char *moves);
char *LegalMoves(Board board, const char *moves);
void squareToAlgebraic(short int square, char *buffer);
char *filter_valid_moves(const char *input);
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
