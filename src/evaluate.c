#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "evaluate.h"
#include "bitboard.h"
#include "init.h"
#include "evaluate.h"

// Piece values
int pieceValues[12] = {P_VALUE, N_VALUE, B_VALUE, R_VALUE, Q_VALUE, K_VALUE, -P_VALUE, -N_VALUE, -B_VALUE, -R_VALUE, -Q_VALUE, -K_VALUE};

// Piece-square tables (example values, can be tuned)
const int pawn_table[64] = {
    0,  0,  0,  0,  0,  0,  0,  0,
    5, 10, 10,-20,-20, 10, 10,  5,
    5, -5,-10,  0,  0,-10, -5,  5,
    0,  0,  0, 20, 20,  0,  0,  0,
    5,  5, 10, 25, 25, 10,  5,  5,
    10, 10, 20, 30, 30, 20, 10, 10,
    50, 50, 50, 50, 50, 50, 50, 50,
    0,  0,  0,  0,  0,  0,  0,  0
};

const int knight_table[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
};

const int bishop_table[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -20,-10,-10,-10,-10,-10,-10,-20
};

const int rook_table[64] = {
    0,  0,  0,  5,  5,  0,  0,  0,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    5, 10, 10, 10, 10, 10, 10,  5,
    0,  0,  0,  0,  0,  0,  0,  0
};

const int queen_table[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -10,  5,  5,  5,  5,  5,  0,-10,
     0,  0,  5,  5,  5,  5,  0, -5,
    -5,  0,  5,  5,  5,  5,  0, -5,
    -10,  0,  5,  5,  5,  5,  0,-10,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
};

const int king_table[64] = {
    20, 30, 10,  0,  0, 10, 30, 20,
    20, 20,  0,  0,  0,  0, 20, 20,
    -10,-20,-20,-20,-20,-20,-20,-10,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30
};

const int king_table_endgame[64] = {
    -50,-40,-30,-20,-20,-30,-40,-50,
    -30,-20,-10,  0,  0,-10,-20,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-30,  0,  0,  0,  0,-30,-30,
    -50,-30,-30,-30,-30,-30,-30,-50
};

// returns piece index or -1 if the square is empty
// returns piece index or -1 if the square is empty
int whatPiece(unsigned long long bitboards[12], short int sqr) {
    int piece;

    // check if a square is within bounds
    if (sqr < 0 || sqr > 63) return -1;

    for (piece = 0; piece < 12; piece++) {
        if (IS_BIT_SET(bitboards[piece], sqr)) return piece;
    }
    return -1;
}

// returns the square of the king of the asked player
short int getKingSquare(unsigned long long bitboards[12], char usPlayer) {
    if (usPlayer == 'w') {
        return __builtin_ctzll(bitboards[WHITE_KING]); // White king's position
    } else {
        return __builtin_ctzll(bitboards[BLACK_KING]); // Black king's position
    }
}

// Evaluate the position based on material balance
int evaluateMaterial(Board board){
    int score = 0;
    int player = (board->toMove == 'w') ? 1 : -1;

    // Material balance
    score += __builtin_popcountll(board->bitboards[WHITE_PAWNS]) * P_VALUE;
    score += __builtin_popcountll(board->bitboards[WHITE_ROOKS]) * R_VALUE;
    score += __builtin_popcountll(board->bitboards[WHITE_KNIGHTS]) * N_VALUE;
    score += __builtin_popcountll(board->bitboards[WHITE_BISHOPS]) * B_VALUE;
    score += __builtin_popcountll(board->bitboards[WHITE_QUEEN]) * Q_VALUE;
    score += __builtin_popcountll(board->bitboards[WHITE_KING]) * K_VALUE;

    score -= __builtin_popcountll(board->bitboards[BLACK_PAWNS]) * P_VALUE;
    score -= __builtin_popcountll(board->bitboards[BLACK_ROOKS]) * R_VALUE;
    score -= __builtin_popcountll(board->bitboards[BLACK_KNIGHTS]) * N_VALUE;
    score -= __builtin_popcountll(board->bitboards[BLACK_BISHOPS]) * B_VALUE;
    score -= __builtin_popcountll(board->bitboards[BLACK_QUEEN]) * Q_VALUE;
    score -= __builtin_popcountll(board->bitboards[BLACK_KING]) * K_VALUE;

    if (player == -1) score = -score;
    
    return score;
}

// Evaluate the position based on piece-square tables
int evaluatePosition(Board board, int gameState){
    int player = (board->toMove == 'w') ? 1 : -1;
    int score = 0;

    // Piece-square tables
    for (int square = 63; square > -1; square--) {
        if (IS_BIT_SET(board->bitboards[WHITE_PAWNS], square)) {
            score += pawn_table[square];
        }
        if (IS_BIT_SET(board->bitboards[BLACK_PAWNS], square)) {
            score -= pawn_table[63 - square]; // Flip for black
        }

        if (IS_BIT_SET(board->bitboards[WHITE_KNIGHTS], square)) {
            score += knight_table[square];
        }
        if (IS_BIT_SET(board->bitboards[BLACK_KNIGHTS], square)) {
            score -= knight_table[63 - square];
        }

        if (IS_BIT_SET(board->bitboards[WHITE_BISHOPS], square)) {
            score += bishop_table[square];
        }
        if (IS_BIT_SET(board->bitboards[BLACK_BISHOPS], square)) {
            score -= bishop_table[63 - square];
        }

        if (IS_BIT_SET(board->bitboards[WHITE_ROOKS], square)) {
            score += rook_table[square];
        }
        if (IS_BIT_SET(board->bitboards[BLACK_ROOKS], square)) {
            score -= rook_table[63 - square];
        }

        if (IS_BIT_SET(board->bitboards[WHITE_QUEEN], square)) {
            score += queen_table[square];
        }
        if (IS_BIT_SET(board->bitboards[BLACK_QUEEN], square)) {
            score -= queen_table[63 - square];
        }

        if (IS_BIT_SET(board->bitboards[WHITE_KING], square)) {
            if(gameState == 2){ 
                score += king_table_endgame[square];
            }else{
                score += king_table[square];
            }
        }
        if (IS_BIT_SET(board->bitboards[BLACK_KING], square)) {
            if(gameState == 2){ 
                score -= king_table_endgame[63 - square];
            }else{
                score -= king_table[63 - square];
            }
        }
    }
        if(player == -1) score = -score;

        return score;
}

// Check if a pawn is isolated
int isBackwardPawn(unsigned long long pawnBitboard, int square, char color) {
    int file = square % 8;
    int rank = square / 8;

    int advanceRank = (color == 'w') ? rank + 1 : rank - 1;

    if (advanceRank < 0 || advanceRank > 7) return 0; // If out of bounds, it's not backward.

    // Get bitboards for same file and adjacent files
    unsigned long long sameFile = 1ULL << (advanceRank * 8 + file);
    unsigned long long leftFile = (file > 0) ? 1ULL << (advanceRank * 8 + file - 1) : 0;
    unsigned long long rightFile = (file < 7) ? 1ULL << (advanceRank * 8 + file + 1) : 0;

    // Check if there are friendly pawns ahead in the same file or adjacent files
    unsigned long long ahead = pawnBitboard & (sameFile | leftFile | rightFile);
    
    return !ahead; // Return true (1) if no supporting pawns, false (0) otherwise.
}

// Evaluate the pawn structure based on pawn support
int PawnSupport(Board board){
    int score = 0;
    int player = (board->toMove == 'w') ? 1 : -1;

    // Isolated pawns
    unsigned long long whitePawns = board->bitboards[WHITE_PAWNS];
    unsigned long long blackPawns = board->bitboards[BLACK_PAWNS];

    for (int square = 63; square > -1; square--) {
        if (IS_BIT_SET(whitePawns, square)) {
            if(IS_BIT_SET(whitePawns, square - 7) || IS_BIT_SET(whitePawns, square - 9)){
                score += 15;
            }
        }
        if (IS_BIT_SET(blackPawns, square)) {
            if(IS_BIT_SET(blackPawns, square + 7) || IS_BIT_SET(blackPawns, square + 9)){
                score -= 15;
            }
        }
    }

    if (player == -1) score = -score;

    return score;
}

// Check if a pawn is isolated
int evaluatePawnStructures(Board board){
    int score = 0;
    int player = (board->toMove == 'w') ? 1 : -1;

    // Isolated pawns
    unsigned long long whitePawns = board->bitboards[WHITE_PAWNS];
    unsigned long long blackPawns = board->bitboards[BLACK_PAWNS];

    for (int square = 0; square < 64; square++) {
        if (IS_BIT_SET(whitePawns, square)) {
            if (isBackwardPawn(whitePawns, square, 'w')) {
                score -= 10;
            }
        }
        if (IS_BIT_SET(blackPawns, square)) {
            if (isBackwardPawn(blackPawns, square, 'b')) {
                score += 10;
            }
        }
    }

    // Adjust bonus for Pawn structure
    score += PawnSupport(board);

    if (player == -1) score = -score;

    return score;
}


// Returns the gamestate based on the current board
int setGameState(Board board){
    
    // Get the king's square
    int kingSquare = getKingSquare(board->bitboards, 'w');
    int kingSquareB = getKingSquare(board->bitboards, 'b');

    // Endgame considerations
    int total_pieces = __builtin_popcountll(board->bitboards[WHITE_PAWNS] | board->bitboards[WHITE_ROOKS] |
                                board->bitboards[WHITE_KNIGHTS] | board->bitboards[WHITE_BISHOPS] |
                                board->bitboards[WHITE_QUEEN]) +
                        __builtin_popcountll(board->bitboards[BLACK_PAWNS] | board->bitboards[BLACK_ROOKS] |
                                board->bitboards[BLACK_KNIGHTS] | board->bitboards[BLACK_BISHOPS] |
                                board->bitboards[BLACK_QUEEN]);
    
    // state considerations
    if (board->fullmove <= 12 && (kingSquareB == 59 || kingSquareB == 60) && (kingSquare == 3 || kingSquare == 4)) {
        return 0; // we are in the opening stage
    } else if(board->fullmove >= 12 && board->fullmove <= 30 && total_pieces > PIECES_ENDGAME) {
        return 1; // we are in the middle game stage
    } else if (total_pieces <= PIECES_ENDGAME && board->fullmove >= 30) {
        return 2; // we re in the endgame now
    }
    return 1;

}


// Returns the value the enemy piece that claim a square
int evaluatePieceSquare(Board board, int square, int player){
    int score = 0;

    if (player == -1){
        for (int piece = 6; piece < 12; piece++) {
            if(piece == BLACK_KNIGHTS) continue;
            if (IS_BIT_SET(board->bitboards[piece], square)) {
                score += pieceValues[piece];
            }
        }
    } else if(player == 1){ 
        for (int piece = 0; piece < 6; piece++) {
            if(piece == WHITE_KNIGHTS) continue;
            if (IS_BIT_SET(board->bitboards[piece], square)) {
                score -= pieceValues[piece];
            }
        }
    }  
    return score;
}

// Function that returns the first vertical or horizontal square that is occupied by a piece
int getSquare(Board board, int square, int direction){
    int Square = square + direction;
    while (Square >= 0 && Square < 64) {
        if (whatPiece(board->bitboards, Square) != -1) {
            return Square;
        }
        Square += direction;
    }
    return -1;
}

// Function that returns the amount of knights that threaten a square 
// A knight on a best case scenario claims 8 possible squares
int getKnightThreats(Board board, int square){
    int threats = 0;
    int player = (board->toMove == 'w') ? 1 : -1;

    int directions[8] = {6, 10, 15, 17, -6, -10, -15, -17};
    for(int i = 0; i < 8; i++){
        if(whatPiece(board->bitboards, square + directions[i]) == player * WHITE_KNIGHTS){
            threats++;
        }
    }

    return threats;
}

// Function that returns the value of the vertical and horizontal threats to a square
int getThreats(Board board){
    int score = 0;
    int knightThreats = 0;
    int directions[8] = {1, -1, 8, -8, 7, -7, 9, -9};

    for (int square = 0; square < 64; square++) {
        knightThreats = getKnightThreats(board, square);
        for(int i = 0; i < 8; i++){
            int Square = getSquare(board, square, directions[i]);
            if(Square != -1){
                score += evaluatePieceSquare(board, Square, -1);
            }
        }
    }

    score -= knightThreats*10;

    return score;
}

// Function that calculates the value of the pieces that guard a square
int getDefenders(Board board){
    int score = 0;
    int player = (board->toMove == 'w') ? 1 : -1;
    int directions[8] = {1, -1, 8, -8, 7, -7, 9, -9};

    for (int square = 0; square < 64; square++) {
        for(int i = 0; i < 8; i++){
            int Square = getSquare(board, square, directions[i]);
            if(Square != -1){
                score += evaluatePieceSquare(board, Square, 1);
            }
        }
        score += evaluatePieceSquare(board, square, player);
    }

    return score;
}

// Function that calculates the value of the pieces that attack a square
// And defends the square along with the value of the piece on it
int evaluateSquare(Board board){
    int score = 0;

    score += getThreats(board);
    score += getDefenders(board);

    return score;
}

// Evaluate the board score from the perspective of the current player
int evaluateBitboard(Board board) {
    int score = 0;

    // Set the game state
    int gameState = setGameState(board);

    // Evaluate the material balance
    score += evaluateMaterial(board);

    // Evaluate the position
    score += evaluatePosition(board, gameState);

    // Evaluate the pawn structures
    score += evaluatePawnStructures(board);
    
    // Attacked squares penalty currently working on it
    //score += evaluateSquare(board);

    return score;
}