#ifndef INIT
#define INIT

#define BOARD_SIZE 8
#define MAX_HISTORY 100
#define MAX_DEPTH 4

#define MAX_MOVE_LENGTH 10 // d7xe8=B++ is a valid move notation
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define ERROR_CODE -1
#define DEBUG 0 // turn 1 for debug prints

// Setting a bit (placing a piece)
#define SET_BIT(bitboard, square) ((bitboard) |= (1ULL << (square)))

// Clearing a bit (removing a piece)
#define CLEAR_BIT(bitboard, square) ((bitboard) &= ~(1ULL << (square)))

// Checking if a bit is set
#define IS_BIT_SET(bitboard, square) ((bitboard) & (1ULL << (square)))

// Mapping squares (0-63 for A1-H8).
enum squares {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8
};

// Define piece constants for indexing the bitboards (0-indexed).
enum {
    WHITE_PAWNS, WHITE_ROOKS, WHITE_KNIGHTS, WHITE_BISHOPS, WHITE_QUEEN, WHITE_KING,
    BLACK_PAWNS, BLACK_ROOKS, BLACK_KNIGHTS, BLACK_BISHOPS, BLACK_QUEEN, BLACK_KING
};

typedef struct board {
    unsigned long long bitboards[12]; // One bitboard per piece type and color, 12 in total
    char toMove; // next player's letter ('w' or 'b')
    char castling[5]; // whether castling is possible or not
    char pass[3]; //  whether en passant is possible or not
    // (note: we need 3 spots because. when there is en passant availability, the only thing
    // given is the destination square).
    unsigned short int halfmove; // counter for halfmoves
    unsigned short int fullmove; // counter for full moves
} * Board;

void debugPrint(const char *format, ...);

#endif