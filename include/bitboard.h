#ifndef BITBOARD
#define BITBOARD

#include <stdio.h>
#include "init.h"

// Named constants for clarity
#define KINGSIDE_CASTLE_SIZE 5
#define QUEENSIDE_CASTLE_SIZE 5
#define CHECK_SYMBOL '+'
#define MAX_FEN_LENGTH 72

// Board representation
#define RANK_OFFSET 56
#define RANKS 8
#define FILES 8

// Function to parse a single FEN character
int parseFenRec(Board board, char *fen);

// Function to apply a move to the board(without deleting the trail)
void move_piece(unsigned long long bitboards[12], char piece, char file, char rank, char file_target, char rank_target);

// Function to update the bitboard with a single movee
void UpdateBitboards(Board board, char *move);

// Handle castling moves 
void handleKingsideCastling(Board board);
void handleQueensideCastling(Board board);

// Square management
int calculateSquareIndex(char file, char rank);
void emptySquare(unsigned long long *bitboards, char file, char rank);
void updateMove(int piece, unsigned long long *bitboards, char file, char rank);

// Functions to aid visualization and debugging
void printBitboard(unsigned long long *bitboards, int piece);
void fprintBitToFen(FILE *stream, Board board);
void printBoard(Board board);

// Function to delete a single moves trail
void DeletePrevious(int piece, unsigned long long *bitboards, char sFile, char sRank, char dFile, char dRank);

// Finds the piece to move based on the source file and rank
int possiblePiece(unsigned long long *bitboards, char sFile, char sRank, int sSquare, int piece);

// Finds the index of the piece at a given square
int whatPieceBit(unsigned long long bitboards[12], int sqr);
int pieceIndex(char p);

#endif
