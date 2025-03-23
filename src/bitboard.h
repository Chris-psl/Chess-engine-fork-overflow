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

// Helper functions for the move function  
void handleKingsideCastling(Board board);
void handleQueensideCastling(Board board);
int calculateSquareIndex(char file, char rank);
void emptySquare(unsigned long long *bitboards, char file, char rank);
void updateMove(int piece, unsigned long long *bitboards, char file, char rank);

// Helper function for bitboard visualization
void printBitboard(unsigned long long *bitboards, int piece);
void fprintBitToFen(FILE *stream, Board board);
void printBoard(Board board);

// Function to delete a single moves trail
void DeletePrevious(int piece, unsigned long long *bitboards, char sFile, char sRank, char dFile, char dRank);
int possiblePiece(unsigned long long *bitboards, char sFile, char sRank, int sSquare, int piece);
int whatPieceBit(unsigned long long bitboards[12], int sqr);
int pieceIndex(char p);

#endif