#ifndef EVALUATE
#define EVALUATE

// here we will define values for all pieces that can be changed depending on our strategy
#define P_VALUE 10 // pawn
#define N_VALUE 30 // knight
#define B_VALUE 30 // bishop
#define R_VALUE 50 // rook
#define Q_VALUE 90 // queen
#define K_VALUE 500 // king (huge value to protect at all costs)
#define BONUS_K_VALUE 200 // bonus points for king in special scenarios like when ruining castling

// defining relative value aditions for positional advantage
#define VAL_GR_CENTER 20
#define VAL_GO_CENTER 5
#define VAL_OUT_CENTER 2
#define KING_ACTIVITY_BONUS 15       // Bonus per square closer to the center in the endgame
#define KING_SAFETY_PENALTY 70       // Penalty if the king is under attack

// defining the value of stopping the possibility of castling
#define NO_CASTLING 50

// defining the multiplier of defensive and offensive evaluation in order to properly compare risk/reward
// the relative size of these two causes the bot to be more or less defensive and agressive
#define POS_MULT 100
#define NEG_MULT 100

//Global attacked square penalty
#define ATTACKED_SQUARE_PENALTY 30

// A Number that if there are less pieces left for any player, endgame strategy activates
#define PIECES_ENDGAME 8

// Function to evaluate a single piece
int addValue(unsigned long long bitboards[12], int piece, int sqr, char usPlayer, int movingPiece, int StartSqr);

// Function to evaluate a single move
int evaluateBitboard(unsigned long long bitboards[12], char usPlayer, unsigned short int fullmove);

// Helper function for the evaluation function
int checkCross(unsigned long long bitboards[12], short int square, char usPlayer);
int checkDiagonal(unsigned long long bitboards[12], short int square, char usPlayer);
int checkKnight(unsigned long long bitboards[12], short int square, char usPlayer);
int checkPawn(unsigned long long bitboards[12], short int square, char usPlayer);
int checkKing(unsigned long long bitboards[12], short int square, char usPlayer);

// Helper functions for the evaluation functions
int squareValue(short int square);
int whatPiece(unsigned long long bitboards[12], short int sqr);
int countPieces(char player, unsigned long long * bitboards);

#endif