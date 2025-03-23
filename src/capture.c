/**
 * @file capture.c
 * @brief This file contains the implementation for generating capturing moves on a bitboard.
 * It includes the necessary functions and logic to handle
 * capturing pieces in a bitboard representation of a game board.
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>

#include "bitboard.h"
#include "init.h"
#include "evaluate.h"
#include "search.h"
#include "tools.h"
#include "capture.h"
#include "movegen.h"

const int BISHOP_DIRECTIONS[4] = {7, 9, -7, -9};

// @brief: checks if a move contains only valid characters
int is_valid_move(const char *move) {
    while (*move) {
        if (!strchr("KQRBNabcdefgh12345678x+#-O=", *move)) {
            return 0; // Invalid character found
        }
        move++;
    }
    return 1; // Move is valid
}

// @brief: filters valid chess moves from input string
char* filter_valid_moves(const char *input) {
    char *filtered = malloc(strlen(input) + 1); // Allocate memory for output
    if (!filtered) return NULL; // Check for allocation failure

    char *output = filtered;
    char move[20]; // Buffer for individual moves
    long long unsigned int i = 0;

    while (*input) {
        if (*input == ' ' || *input == '\0') { // End of a move
            move[i] = '\0'; // Null-terminate the move
            if (i > 0 && is_valid_move(move)) { // Check if move is valid
                if (output != filtered) *output++ = ' '; // Add space separator
                strcpy(output, move);
                output += i;
            }
            i = 0; // Reset move index
        } else if (i < sizeof(move) - 1) {
            move[i++] = *input;
        }
        input++;
    }

    if (i > 0) { // Check the last move
        move[i] = '\0';
        if (is_valid_move(move)) {
            if (output != filtered) *output++ = ' ';
            strcpy(output, move);
            output += i;
        }
    }

    *output = '\0'; // Null-terminate the result
    return filtered;
}

// @brief: finds the square of the enemy king
int EnemyKingSquare(Board board){

    // Check if the board is valid
    if(!board) return -1;

    char kingColor = board->toMove;
    int kingIndex;
    if (kingColor == 'w')
        kingIndex = BLACK_KING;
    else if (kingColor == 'b')
        kingIndex = WHITE_KING;
    else
        return -1; // Invalid color
    
    unsigned long long kingBB = board->bitboards[kingIndex];
    for (int square = 0; square < 64; square++) {
        if (kingBB & (1ULL << square))
            return square;
    }
    return -1; // King not found
}

//Function to get the square of the king
int KingSquare(Board board){

    // Check if the board is valid
    if(!board) return -1;
    char kingColor = board->toMove;
    int kingIndex;
    if (kingColor == 'w')
        kingIndex = WHITE_KING;
    else if (kingColor == 'b')
        kingIndex = BLACK_KING;
    else
        return -1; // Invalid color
    
    unsigned long long kingBB = board->bitboards[kingIndex];
    for (int square = 0; square < 64; square++) {
        if (kingBB & (1ULL << square)){
            return square;
        }
    }
    return -1; // King not found
}

int isKingAttacked(Board board){
    int kingSquare = KingSquare(board);
    if (isSquareAttacked(board, kingSquare))
        return 1;
    return 0;
}

//----------------------------------------------------------------------------
// isSquareAttacked: returns nonzero if 'square' (0..63) is attacked by any
// enemy piece. The enemy is determined by board->toMove (i.e. if white is 
// to move, then we check for black attackers).
//----------------------------------------------------------------------------
int isSquareAttacked(Board board, int square) {
    // Determine enemy color: if board->toMove is 'w', enemy is white? 
    // (Since you typically call this to see if your king is in check, enemy is
    // the opposite of the side to move.)
    // Here we assume we want to know if 'square' is attacked by the opponent.
    char enemyColor = (board->toMove == 'w' ? 'b' : 'w');

    // Compute the full occupancy (all pieces on the board).
    unsigned long long occupancy = 0ULL;
    for (int i = 0; i < 12; i++) {
        occupancy |= board->bitboards[i];
    }

    //--------------------------------------------------------------------------
    // 1. Pawn Attacks
    // For pawn attacks we “invert” the pawn-capture move:
    //   - If enemy is Black (board->toMove == 'w'): Black pawns capture DOWNWARD.
    //     A black pawn on square s attacks s - 7 (down-right) and s - 9 (down-left),
    //     so to see if a square is attacked, check if a black pawn exists on square+7
    //     or square+9. (With proper file-bound checks.)
    //   - If enemy is White (board->toMove == 'b'): White pawns capture UPWARD.
    //     A white pawn on s attacks s + 7 (up-left) and s + 9 (up-right),
    //     so check if a white pawn exists on square-7 or square-9.
    //--------------------------------------------------------------------------

    if (enemyColor == 'w') { // Enemy is White.
        int s1 = square - 7;  // white pawn that would attack from the left
        int s2 = square - 9;  // white pawn that would attack from the right
        if (s1 >= 0 && (s1 % 8) != 7 && IS_BIT_SET(board->bitboards[WHITE_PAWNS], s1))
            return 1;
        if (s2 >= 0 && (s2 % 8) != 0 && IS_BIT_SET(board->bitboards[WHITE_PAWNS], s2))
            return 1;
    } else { // enemyColor == 'b'; Enemy is Black.
        int s1 = square + 7;  // black pawn that would attack from the right
        int s2 = square + 9;  // black pawn that would attack from the left
        if (s1 < 64 && (s1 % 8) != 7 && IS_BIT_SET(board->bitboards[BLACK_PAWNS], s1))
            return 1;
        if (s2 < 64 && (s2 % 8) != 0 && IS_BIT_SET(board->bitboards[BLACK_PAWNS], s2))
            return 1;
    }

    //--------------------------------------------------------------------------
    // 2. Knight Attacks
    // Knight moves (offsets relative to the square). We must check board bounds
    // and avoid wrapping across files.
    //--------------------------------------------------------------------------

    int knightOffsets[8] = { 17, 15, 10, 6, -17, -15, -10, -6 };
    for (int i = 0; i < 8; i++) {
        int s = square + knightOffsets[i];
        if (s < 0 || s >= 64)
            continue;
        // Ensure the move did not wrap horizontally.
        if (abs((square % 8) - (s % 8)) > 2)
            continue;
        if (enemyColor == 'w') {
            if (IS_BIT_SET(board->bitboards[WHITE_KNIGHTS], s))
                return 1;
        } else {
            if (IS_BIT_SET(board->bitboards[BLACK_KNIGHTS], s))
                return 1;
        }
    }

    //--------------------------------------------------------------------------
    // 3. Sliding Pieces (Rook/Queen – Straight Moves)
    // We check the four straight directions: up, down, right, left.
    //--------------------------------------------------------------------------

    int straightDirs[4] = { 8, -8, 1, -1 };
    for (int d = 0; d < 4; d++) {
        int offset = straightDirs[d];
        int current = square;
        while (1) {
            int prev = current;
            current += offset;
            // Out-of-bounds check.
            if (current < 0 || current >= 64)
                break;
            // For horizontal moves, ensure we stay on the same rank.
            if ((offset == 1 || offset == -1) && (current / 8 != prev / 8))
                break;
            // If we hit any piece...
            if (IS_BIT_SET(occupancy, current)) {
                // If that piece is an enemy rook or queen, then the square is attacked.
                if (enemyColor == 'w') {
                    if (IS_BIT_SET(board->bitboards[WHITE_ROOKS], current) ||
                        IS_BIT_SET(board->bitboards[WHITE_QUEEN], current))
                        return 1;
                } else {
                    if (IS_BIT_SET(board->bitboards[BLACK_ROOKS], current) ||
                        IS_BIT_SET(board->bitboards[BLACK_QUEEN], current))
                        return 1;
                }
                break;
            }
        }
    }

    //--------------------------------------------------------------------------
    // 4. Sliding Pieces (Bishop/Queen – Diagonal Moves)
    // We check the four diagonal directions.
    //--------------------------------------------------------------------------

    int diagDirs[4] = { 9, 7, -7, -9 };
    for (int d = 0; d < 4; d++) {
        int offset = diagDirs[d];
        int current = square;
        while (1) {
            int prev = current;
            current += offset;
            if (current < 0 || current >= 64)
                break;
            // Diagonal moves must change the file by exactly 1.
            if (abs((current % 8) - (prev % 8)) != 1)
                break;
            if (IS_BIT_SET(occupancy, current)) {
                if (enemyColor == 'w') {
                    if (IS_BIT_SET(board->bitboards[WHITE_BISHOPS], current) ||
                        IS_BIT_SET(board->bitboards[WHITE_QUEEN], current))
                        return 1;
                } else {
                    if (IS_BIT_SET(board->bitboards[BLACK_BISHOPS], current) ||
                        IS_BIT_SET(board->bitboards[BLACK_QUEEN], current))
                        return 1;
                }
                break;
            }
        }
    }

    //--------------------------------------------------------------------------
    // 5. King Attacks
    // Check the eight surrounding squares.
    //--------------------------------------------------------------------------

    int kingOffsets[8] = { 1, -1, 8, -8, 9, 7, -7, -9 };
    for (int i = 0; i < 8; i++) {
        int s = square + kingOffsets[i];
        if (s < 0 || s >= 64)
            continue;
        if (abs((square % 8) - (s % 8)) > 1)
            continue;
        if (enemyColor == 'w') {
            if (IS_BIT_SET(board->bitboards[WHITE_KING], s))
                return 1;
        } else {
            if (IS_BIT_SET(board->bitboards[BLACK_KING], s))
                return 1;
        }
    }

    // If none of the enemy pieces attack the square, return 0.
    return 0;
}

/*
@brief: converts a square index (0-63) into algebraic notation (e.g., "a1", "h8").
Writes the result into the provided buffer (which must be at least 3 bytes).
*/
void squareToAlgebraic(short int square, char *buffer) {
    if (square < 0 || square > 63) {
        buffer[0] = '\0';
        return;
    }
    buffer[0] = 'a' + (square % 8);      // file: a-h
    buffer[1] = '8' - (square / 8);       // rank: 1-8
    buffer[2] = '\0';
}

/*
@brief: generates all possible pawn capture moves in modern algebraic notation (e.g., "exd6"),
taking into account enemy pieces and en passant targets.

@return: a heap-allocated string containing the moves (separated by spaces).
(Note: the caller is responsible for freeing the memory)
*/
char *generatePawnCaptures(Board board) {
    unsigned long long pawnBitboard, enemyPieces, enPassantTarget = 0;
    short int leftCaptureOffset, rightCaptureOffset;
    char *result = malloc(1024); // Allocate memory for the result string
    if (!result) return NULL;
    result[0] = '\0';

    if (board->toMove == 'w') {
        pawnBitboard = board->bitboards[WHITE_PAWNS];
        enemyPieces = board->bitboards[BLACK_PAWNS]  | board->bitboards[BLACK_ROOKS] |
                      board->bitboards[BLACK_KNIGHTS]| board->bitboards[BLACK_BISHOPS] |
                      board->bitboards[BLACK_QUEEN]  | board->bitboards[BLACK_KING];
        leftCaptureOffset = -7;   // Pawn capturing left: file -1, rank +1
        rightCaptureOffset = -9;  // Pawn capturing right: file +1, rank +1
    } else {
        pawnBitboard = board->bitboards[BLACK_PAWNS];
        enemyPieces = board->bitboards[WHITE_PAWNS]  | board->bitboards[WHITE_ROOKS] |
                      board->bitboards[WHITE_KNIGHTS]| board->bitboards[WHITE_BISHOPS] |
                      board->bitboards[WHITE_QUEEN]  | board->bitboards[WHITE_KING];
        leftCaptureOffset = +9;  // Pawn capturing left: file -1, rank -1
        rightCaptureOffset = +7; // Pawn capturing right: file +1, rank -1
    }

    // // Process en passant target square(s) without modifying board->pass.
    // if (strlen(board->pass) > 0) {
    //     char passCopy[32];
    //     strncpy(passCopy, board->pass, sizeof(passCopy) - 1);
    //     passCopy[sizeof(passCopy) - 1] = '\0';
    //     char *token = strtok(passCopy, ",");
    //     while (token != NULL) {
    //         if (strlen(token) >= 2) {
    //             short int epSquare = (token[0] - 'a') + (token[1] - '1') * 8;
    //             SET_BIT(enPassantTarget, epSquare);
    //         }
    //         token = strtok(NULL, ",");
    //     }
    // }

    char to[3];
    char move[10];
    int pawncount = 0;
    if(board-> toMove == 'w'){
        // Iterate over all squares to find pawns and check for capture moves.
        for (short int square = 0; square < 64; square++) {
            if(pawncount >= 8)break;
            if (IS_BIT_SET(pawnBitboard, square)) {
                pawncount ++;
                int file = square % 8;
                // Use only the pawn's file letter (e.g., 'e') in the move output.
                char fileChar = 'a' + file;

                //-------special case promotion capture---------------------------------------

                if (square > 7 && square < 25){

                    //QUEEN PROMOTION
                    // Check left capture (only if pawn is not on file 'a').
                    if (file > 0) {
                        short int targetSquare = square + leftCaptureOffset;
                        if (targetSquare >= 0 && targetSquare < 64 &&
                            abs(file - (targetSquare % 8)) == 1) {
                            if (IS_BIT_SET(enemyPieces, targetSquare) ||
                                IS_BIT_SET(enPassantTarget, targetSquare)) {
                                squareToAlgebraic(targetSquare, to);
                                snprintf(move, sizeof(move), "%cx%s=Q ", fileChar, to);
                                strcat(result, move);
                            }
                        }
                    }
                    // Check right capture (only if pawn is not on file 'h').
                    if (file < 7) {
                        short int targetSquare = square + rightCaptureOffset;
                        if (targetSquare >= 0 && targetSquare < 64 &&
                            abs(file - (targetSquare % 8)) == 1) {
                            if (IS_BIT_SET(enemyPieces, targetSquare) ||
                                IS_BIT_SET(enPassantTarget, targetSquare)) {
                                squareToAlgebraic(targetSquare, to);
                                snprintf(move, sizeof(move), "%cx%s=Q ", fileChar, to);
                                strcat(result, move);
                            }
                        }
                    }

                    // KNIGHT PROMOTION
                    // Check left capture (only if pawn is not on file 'a').
                    if (file > 0) {
                        short int targetSquare = square + leftCaptureOffset;
                        if (targetSquare >= 0 && targetSquare < 64 &&
                            abs(file - (targetSquare % 8)) == 1) {
                            if (IS_BIT_SET(enemyPieces, targetSquare) ||
                                IS_BIT_SET(enPassantTarget, targetSquare)) {
                                squareToAlgebraic(targetSquare, to);
                                snprintf(move, sizeof(move), "%cx%s=N ", fileChar, to);
                                strcat(result, move);
                            }
                        }
                    }
                    // Check right capture (only if pawn is not on file 'h').
                    if (file < 7) {
                        short int targetSquare = square + rightCaptureOffset;
                        if (targetSquare >= 0 && targetSquare < 64 &&
                            abs(file - (targetSquare % 8)) == 1) {
                            if (IS_BIT_SET(enemyPieces, targetSquare) ||
                                IS_BIT_SET(enPassantTarget, targetSquare)) {
                                squareToAlgebraic(targetSquare, to);
                                snprintf(move, sizeof(move), "%cx%s=N ", fileChar, to);
                                strcat(result, move);
                            }
                        }
                    }
                    
                }
                //------- end of special case promotion capture---------------------------------------

                // Check left capture (only if pawn is not on file 'a').
                if (file > 0) {
                    short int targetSquare = square + leftCaptureOffset;
                    if (targetSquare >= 0 && targetSquare < 64 &&
                        abs(file - (targetSquare % 8)) == 1) {
                        if (IS_BIT_SET(enemyPieces, targetSquare) ||
                            IS_BIT_SET(enPassantTarget, targetSquare)) {
                            squareToAlgebraic(targetSquare, to);
                            snprintf(move, sizeof(move), "%cx%s ", fileChar, to);
                            strcat(result, move);
                        }
                    }
                }
                // Check right capture (only if pawn is not on file 'h').
                if (file < 7) {
                    short int targetSquare = square + rightCaptureOffset;
                    if (targetSquare >= 0 && targetSquare < 64 &&
                        abs(file - (targetSquare % 8)) == 1) {
                        if (IS_BIT_SET(enemyPieces, targetSquare) ||
                            IS_BIT_SET(enPassantTarget, targetSquare)) {
                            squareToAlgebraic(targetSquare, to);
                            snprintf(move, sizeof(move), "%cx%s ", fileChar, to);
                            strcat(result, move);
                        }
                    }
                }
            }
        }
    }else {
        // Iterate over all squares to find pawns and check for capture moves.
        for (short int square = 0; square < 64; square++) {
            if (pawncount >= 8)break;
            if (IS_BIT_SET(pawnBitboard, square)) {
                pawncount ++;
                int file = square % 8;
                // Use only the pawn's file letter (e.g., 'e') in the move output.
                char fileChar = 'a' + file;

                //-------special case promotion capture---------------------------------------
                if (square > 7 && square < 25){

                    //QUEEN PROMOTION
                    // Check left capture (only if pawn is not on file 'a').
                    if (file > 0) {
                        short int targetSquare = square + leftCaptureOffset;
                        if (targetSquare >= 0 && targetSquare < 64 &&
                            abs(file - (targetSquare % 8)) == 1) {
                            if (IS_BIT_SET(enemyPieces, targetSquare) ||
                                IS_BIT_SET(enPassantTarget, targetSquare)) {
                                squareToAlgebraic(targetSquare, to);
                                snprintf(move, sizeof(move), "%cx%s=Q ", fileChar, to);
                                strcat(result, move);
                            }
                        }
                    }
                    // Check right capture (only if pawn is not on file 'h').
                    if (file < 7) {
                        short int targetSquare = square + rightCaptureOffset;
                        if (targetSquare >= 0 && targetSquare < 64 &&
                            abs(file - (targetSquare % 8)) == 1) {
                            if (IS_BIT_SET(enemyPieces, targetSquare) ||
                                IS_BIT_SET(enPassantTarget, targetSquare)) {
                                squareToAlgebraic(targetSquare, to);
                                snprintf(move, sizeof(move), "%cx%s=Q ", fileChar, to);
                                strcat(result, move);
                            }
                        }
                    }

                    // KNIGHT PROMOTION
                    // Check left capture (only if pawn is not on file 'a').
                    if (file > 0) {
                        short int targetSquare = square + leftCaptureOffset;
                        if (targetSquare >= 0 && targetSquare < 64 &&
                            abs(file - (targetSquare % 8)) == 1) {
                            if (IS_BIT_SET(enemyPieces, targetSquare) ||
                                IS_BIT_SET(enPassantTarget, targetSquare)) {
                                squareToAlgebraic(targetSquare, to);
                                snprintf(move, sizeof(move), "%cx%s=N ", fileChar, to);
                                strcat(result, move);
                            }
                        }
                    }
                    // Check right capture (only if pawn is not on file 'h').
                    if (file < 7) {
                        short int targetSquare = square + rightCaptureOffset;
                        if (targetSquare >= 0 && targetSquare < 64 &&
                            abs(file - (targetSquare % 8)) == 1) {
                            if (IS_BIT_SET(enemyPieces, targetSquare) ||
                                IS_BIT_SET(enPassantTarget, targetSquare)) {
                                squareToAlgebraic(targetSquare, to);
                                snprintf(move, sizeof(move), "%cx%s=N ", fileChar, to);
                                strcat(result, move);
                            }
                        }
                    }
                    
                }
                //------- end of special case promotion capture---------------------------------------

                // Check left capture (only if pawn is not on file 'a').
                if (file < 7) {
                    short int targetSquare = square + leftCaptureOffset;
                    if (targetSquare >= 0 && targetSquare < 64 &&
                        abs(file - (targetSquare % 8)) == 1) {
                        if (IS_BIT_SET(enemyPieces, targetSquare) ||
                            IS_BIT_SET(enPassantTarget, targetSquare)) {
                            squareToAlgebraic(targetSquare, to);
                            snprintf(move, sizeof(move), "%cx%s ", fileChar, to);
                            strcat(result, move);
                        }
                    }
                }
                // Check right capture (only if pawn is not on file 'h').
                if (file > 0) {
                    short int targetSquare = square + rightCaptureOffset;
                    if (targetSquare >= 0 && targetSquare < 64 &&
                        abs(file - (targetSquare % 8)) == 1) {
                        if (IS_BIT_SET(enemyPieces, targetSquare) ||
                            IS_BIT_SET(enPassantTarget, targetSquare)) {
                            squareToAlgebraic(targetSquare, to);
                            snprintf(move, sizeof(move), "%cx%s ", fileChar, to);
                            strcat(result, move);
                        }
                    }
                }
            }
        }
    }


    // Remove trailing space, if any.
    size_t len = strlen(result);
    if (len > 0 && result[len - 1] == ' ')
        result[len - 1] = '\0';

    return result;
}

/*
@brief: generates bishop capture moves in modern algebraic notation (e.g., "Bc1xe3").
It scans in all four diagonal directions from each bishop.

@return: a heap-allocated string containing the moves (separated by spaces).
(Note: the caller is responsible for freeing the memory)
*/
char *generateBishopCaptures(Board board) {
    unsigned long long bishopBitboard, enemyPieces, occupancy = 0ULL;
    char *result = malloc(1024); // Allocate memory for the result string
    if (!result) return NULL;
    result[0] = '\0';

    // Define bishop move directions (assuming board squares numbered 0..63)
    int bishopDirections[4] = { 9, 7, -7, -9 };

    if (board->toMove == 'w') {
        bishopBitboard = board->bitboards[WHITE_BISHOPS];
        enemyPieces = board->bitboards[BLACK_PAWNS]  | board->bitboards[BLACK_ROOKS] |
                      board->bitboards[BLACK_KNIGHTS]| board->bitboards[BLACK_BISHOPS] |
                      board->bitboards[BLACK_QUEEN]  | board->bitboards[BLACK_KING];
        // Occupancy includes all white pieces (friendly) plus enemy pieces.
        occupancy = board->bitboards[WHITE_PAWNS]   | board->bitboards[WHITE_ROOKS] |
                    board->bitboards[WHITE_KNIGHTS] | board->bitboards[WHITE_BISHOPS] |
                    board->bitboards[WHITE_QUEEN]   | board->bitboards[WHITE_KING]  | enemyPieces;
    } else {
        bishopBitboard = board->bitboards[BLACK_BISHOPS];
        enemyPieces = board->bitboards[WHITE_PAWNS]  | board->bitboards[WHITE_ROOKS] |
                      board->bitboards[WHITE_KNIGHTS]| board->bitboards[WHITE_BISHOPS] |
                      board->bitboards[WHITE_QUEEN]  | board->bitboards[WHITE_KING];
        occupancy = board->bitboards[BLACK_PAWNS]   | board->bitboards[BLACK_ROOKS] |
                    board->bitboards[BLACK_KNIGHTS] | board->bitboards[BLACK_BISHOPS] |
                    board->bitboards[BLACK_QUEEN]   | board->bitboards[BLACK_KING]  | enemyPieces;
    }

    char from[3], to[3];
    char move[10];

    int bishopcount=0;
    // Iterate over all squares to find bishops.
    for (short int square = 0; square < 64; square++) {
        if(bishopcount >= 2)break;
        if (IS_BIT_SET(bishopBitboard, square)) {
            bishopcount++;
            squareToAlgebraic(square, from);
            // Check each diagonal direction.
            for (int d = 0; d < 4; d++) {
                int current = square;
                while (1) {
                    int prev = current;
                    current += bishopDirections[d];

                    // Check bounds.
                    if (current < 0 || current >= 64)
                        break;
                    // Ensure movement remains diagonal by checking file change.
                    if (abs((current % 8) - (prev % 8)) != 1)
                        break;
                    
                    // If any piece is encountered, the bishop cannot move further.
                    if (IS_BIT_SET(occupancy, current)) {
                        // If the piece is an enemy, record the capture move.
                        if (IS_BIT_SET(enemyPieces, current)) {
                            squareToAlgebraic(current, to);
                            snprintf(move, sizeof(move), "B%sx%s ", from, to);
                            strcat(result, move);
                        }
                        break; // Stop in this direction regardless.
                    }
                }
            }
        }
    }

    // Remove trailing space, if any.
    size_t len = strlen(result);
    if (len > 0 && result[len - 1] == ' ')
        result[len - 1] = '\0';

    return result;
}

/*
@brief: generates knight capture moves in modern algebraic notation (e.g., "Bc1xe3").
It scans in all four diagonal directions from each bishop.

@return: a heap-allocated string containing the moves (separated by spaces).
(Note: the caller is responsible for freeing the memory)
*/
char *generateKnightCaptures(Board board) {
    unsigned long long knightBitboard, enemyPieces;
    char *result = malloc(1024); // Allocate memory for the result string
    if (!result) return NULL;
    result[0] = '\0';

    // Define knight move directions (assuming board squares numbered 0..63)
    int knightDirections[8] = { 6, 10, 15, 17, -6, -10, -15, -17 };

    if (board->toMove == 'w') {
        knightBitboard = board->bitboards[WHITE_KNIGHTS];
        enemyPieces = board->bitboards[BLACK_PAWNS]  | board->bitboards[BLACK_ROOKS] |
                      board->bitboards[BLACK_KNIGHTS]| board->bitboards[BLACK_BISHOPS] |
                      board->bitboards[BLACK_QUEEN]  | board->bitboards[BLACK_KING];
    } else {
        knightBitboard = board->bitboards[BLACK_KNIGHTS];
        enemyPieces = board->bitboards[WHITE_PAWNS]  | board->bitboards[WHITE_ROOKS] |
                      board->bitboards[WHITE_KNIGHTS]| board->bitboards[WHITE_BISHOPS] |
                      board->bitboards[WHITE_QUEEN]  | board->bitboards[WHITE_KING];
    }

    char from[3], to[3];
    char move[10];
    
    // Iterate over all squares to find knights.
    for (short int square = 0; square < 64; square++) {
        if (IS_BIT_SET(knightBitboard, square)) {
            squareToAlgebraic(square, from);
            // Check each knight move direction.
            for (int d = 0; d < 8; d++) {
                int target = square + knightDirections[d];
                // Check bounds.
                if (target < 0 || target >= 64)
                    continue;
                // Ensure movement remains L-shaped.
                if (abs((square % 8) - (target % 8)) > 2)
                    continue;
                // If the target square is occupied by an enemy piece, record the capture move.
                if (IS_BIT_SET(enemyPieces, target)) {
                    squareToAlgebraic(target, to);
                    snprintf(move, sizeof(move), "N%sx%s ", from, to);
                    strcat(result, move);
                }
            }
        }
    }

    // Remove trailing space, if any.
    size_t len = strlen(result);
    if (len > 0 && result[len - 1] == ' ')
        result[len - 1] = '\0';

    return result;
}

char *generateRookCaptures(Board board){
    unsigned long long rookBitboard, enemyPieces, occupancy = 0ULL;
    char *result = malloc(1024); // Allocate memory for the result string
    if (!result) return NULL;
    result[0] = '\0';

    // Define rook move directions (assuming board squares numbered 0..63)
    int rookDirections[4] = { 8, -8, 1, -1 };

    if (board->toMove == 'w') {
        rookBitboard = board->bitboards[WHITE_ROOKS];
        enemyPieces = board->bitboards[BLACK_PAWNS]  | board->bitboards[BLACK_ROOKS] |
                      board->bitboards[BLACK_KNIGHTS]| board->bitboards[BLACK_BISHOPS] |
                      board->bitboards[BLACK_QUEEN]  | board->bitboards[BLACK_KING];
        occupancy = board->bitboards[WHITE_PAWNS]   | board->bitboards[WHITE_ROOKS] |
                    board->bitboards[WHITE_KNIGHTS] | board->bitboards[WHITE_BISHOPS] |
                    board->bitboards[WHITE_QUEEN]   | board->bitboards[WHITE_KING]  | enemyPieces;
    } else {
        rookBitboard = board->bitboards[BLACK_ROOKS];
        enemyPieces = board->bitboards[WHITE_PAWNS]  | board->bitboards[WHITE_ROOKS] |
                      board->bitboards[WHITE_KNIGHTS]| board->bitboards[WHITE_BISHOPS] |
                      board->bitboards[WHITE_QUEEN]  | board->bitboards[WHITE_KING];
        occupancy = board->bitboards[BLACK_PAWNS]   | board->bitboards[BLACK_ROOKS] |
                    board->bitboards[BLACK_KNIGHTS] | board->bitboards[BLACK_BISHOPS] |
                    board->bitboards[BLACK_QUEEN]   | board->bitboards[BLACK_KING]  | enemyPieces;
    }

    char from[3], to[3];
    char move[10];

    // Iterate over all squares to find rooks.
    for (short int square = 0; square < 64; square++) {
        if (IS_BIT_SET(rookBitboard, square)) {
            squareToAlgebraic(square, from);
            // Check each rook move direction.
            for (int d = 0; d < 4; d++) {
                int current = square;
                while (1) {
                    int prev = current;
                    current += rookDirections[d];

                    // Check bounds.
                    if (current < 0 || current >= 64)
                        break;
                    // Ensure movement remains in the same rank or file.
                    if ((d >= 2 && (current / 8) != (prev / 8)) || (d < 2 && (current % 8) != (prev % 8)))
                        break;

                    // If any piece is encountered, the rook cannot move further.
                    if (IS_BIT_SET(occupancy, current)) {
                        // If the piece is an enemy, record the capture move.
                        if (IS_BIT_SET(enemyPieces, current)) {
                            squareToAlgebraic(current, to);
                            snprintf(move, sizeof(move), "R%sx%s ", from, to);
                            strcat(result, move);
                        }
                        break; // Stop in this direction regardless.
                    }
                }
            }
        }
    }

    // Remove trailing space, if any.
    size_t len = strlen(result);
    if (len > 0 && result[len - 1] == ' ')
        result[len - 1] = '\0';

    return result;
}

char *generateQueenCaptures(Board board) {
    unsigned long long queenBitboard, enemyPieces, occupancy = 0ULL;
    char *result = malloc(1024); // Allocate memory for the result string
    if (!result) return NULL;
    result[0] = '\0';

    // Define queen move directions (combining rook and bishop directions)
    int queenDirections[8] = { 8, -8, 1, -1, 9, 7, -7, -9 };

    if (board->toMove == 'w') {
        queenBitboard = board->bitboards[WHITE_QUEEN];
        enemyPieces = board->bitboards[BLACK_PAWNS]  | board->bitboards[BLACK_ROOKS] |
                      board->bitboards[BLACK_KNIGHTS]| board->bitboards[BLACK_BISHOPS] |
                      board->bitboards[BLACK_QUEEN]  | board->bitboards[BLACK_KING];
        occupancy = board->bitboards[WHITE_PAWNS]   | board->bitboards[WHITE_ROOKS] |
                    board->bitboards[WHITE_KNIGHTS] | board->bitboards[WHITE_BISHOPS] |
                    board->bitboards[WHITE_QUEEN]   | board->bitboards[WHITE_KING]  | enemyPieces;
    } else {
        queenBitboard = board->bitboards[BLACK_QUEEN];
        enemyPieces = board->bitboards[WHITE_PAWNS]  | board->bitboards[WHITE_ROOKS] |
                      board->bitboards[WHITE_KNIGHTS]| board->bitboards[WHITE_BISHOPS] |
                      board->bitboards[WHITE_QUEEN]  | board->bitboards[WHITE_KING];
        occupancy = board->bitboards[BLACK_PAWNS]   | board->bitboards[BLACK_ROOKS] |
                    board->bitboards[BLACK_KNIGHTS] | board->bitboards[BLACK_BISHOPS] |
                    board->bitboards[BLACK_QUEEN]   | board->bitboards[BLACK_KING]  | enemyPieces;
    }

    char from[3], to[3];
    char move[10];

    // Iterate over all squares to find queens.
    for (short int square = 0; square < 64; square++) {
        if (IS_BIT_SET(queenBitboard, square)) {
            squareToAlgebraic(square, from);
            // Check each queen move direction.
            for (int d = 0; d < 8; d++) {
                int current = square;
                while (1) {
                    int prev = current;
                    current += queenDirections[d];

                    // Check bounds.
                    if (current < 0 || current >= 64)
                        break;
                    // Ensure movement remains in the same rank, file, or diagonal.
                    if ((d < 4 && ((d >= 2 && (current / 8) != (prev / 8)) || (d < 2 && (current % 8) != (prev % 8)))) ||
                        (d >= 4 && abs((current % 8) - (prev % 8)) != 1))
                        break;

                    // If any piece is encountered, the queen cannot move further.
                    if (IS_BIT_SET(occupancy, current)) {
                        // If the piece is an enemy, record the capture move.
                        if (IS_BIT_SET(enemyPieces, current)) {
                            squareToAlgebraic(current, to);
                            snprintf(move, sizeof(move), "Q%sx%s ", from, to);
                            strcat(result, move);
                        }
                        break; // Stop in this direction regardless.
                    }
                }
            }
        }
    }

    // Remove trailing space, if any.
    size_t len = strlen(result);
    if (len > 0 && result[len - 1] == ' ')
        result[len - 1] = '\0';

    return result;
}

char *generateKingCaptures(Board board){
    unsigned long long kingBitboard, enemyPieces;
        char *result = malloc(1024); // Allocate memory for the result string
        if (!result) return NULL;
        result[0] = '\0';

        // Define king move directions (assuming board squares numbered 0..63)
        int kingDirections[8] = { 1, -1, 8, -8, 9, 7, -7, -9 };

        if (board->toMove == 'w') {
            kingBitboard = board->bitboards[WHITE_KING];
            enemyPieces = board->bitboards[BLACK_PAWNS]  | board->bitboards[BLACK_ROOKS] |
                          board->bitboards[BLACK_KNIGHTS]| board->bitboards[BLACK_BISHOPS] |
                          board->bitboards[BLACK_QUEEN]  | board->bitboards[BLACK_KING];
        } else {
            kingBitboard = board->bitboards[BLACK_KING];
            enemyPieces = board->bitboards[WHITE_PAWNS]  | board->bitboards[WHITE_ROOKS] |
                          board->bitboards[WHITE_KNIGHTS]| board->bitboards[WHITE_BISHOPS] |
                          board->bitboards[WHITE_QUEEN]  | board->bitboards[WHITE_KING];
        }

        char from[3], to[3];
        char move[10];

        // Iterate over all squares to find the king.
        for (short int square = 0; square < 64; square++) {
            if (IS_BIT_SET(kingBitboard, square)) {
                squareToAlgebraic(square, from);
                // Check each king move direction.
                for (int d = 0; d < 8; d++) {
                    int target = square + kingDirections[d];
                    // Check bounds.
                    if (target < 0 || target >= 64)
                        continue;
                    // Ensure movement remains within one square.
                    if (abs((square % 8) - (target % 8)) > 1)
                        continue;
                    // If the target square is occupied by an enemy piece, record the capture move.
                    if (IS_BIT_SET(enemyPieces, target) && isSquareAttacked(board, target) == 0) {
                        squareToAlgebraic(target, to);
                        snprintf(move, sizeof(move), "K%sx%s ", from, to);
                        strcat(result, move);
                    }
                }
            }
        }

        // Remove trailing space, if any.
        size_t len = strlen(result);
        if (len > 0 && result[len - 1] == ' ')
            result[len - 1] = '\0';

        return result;
}

int appendString(char **buffer, size_t *size, size_t *len, const char *str) {
    size_t strLen = strlen(str);
    if (*len + strLen + 1 > *size) {
        *size = (*len + strLen + 1) * 2;
        *buffer = realloc(*buffer, *size);
        if (!*buffer) {
            debugPrint("Memory allocation failed.\n");
            return 0; // Indicate failure
        }
    }
    strcat(*buffer, str);
    *len += strLen;
    return 1; // Indicate success
}

// Generate all possible captures for the current side to move
char *generateAllCaptures(Board board) {
    char *result = malloc(1); // Start with a single byte for the null terminator
    if (!result) return NULL;
    result[0] = '\0';
    size_t resultSize = 1; // Initial size of the result buffer
    size_t resultLen = 0;  // Current length of the result string


    // Generate captures for each piece type
    debugPrint("\nGenerating capture moves...\n");
    char *pawns = generatePawnCaptures(board);
    debugPrint("Pawns: %s\n", pawns);
    char *knights = generateKnightCaptures(board);
    debugPrint("Knights: %s\n", knights);
    char *bishops = generateBishopCaptures(board);
    debugPrint("Bishops: %s\n", bishops);
    char *rooks = generateRookCaptures(board);
    debugPrint("Rooks: %s\n", rooks);
    char *queens = generateQueenCaptures(board);
    debugPrint("Queens: %s\n", queens);
    char *king = generateKingCaptures(board);
    debugPrint("King: %s\n", king);

    // Append captures to the result string
    if (pawns && strlen(pawns) > 0) {
        appendString(&result, &resultSize, &resultLen, pawns);
    }
    if (knights && strlen(knights) > 0) {
        if (resultLen > 0) appendString(&result, &resultSize, &resultLen, " "); // Add space only if needed
        appendString(&result, &resultSize, &resultLen, knights);
    }
    if (bishops && strlen(bishops) > 0) {
        if (resultLen > 0) appendString(&result, &resultSize, &resultLen, " ");
        appendString(&result, &resultSize, &resultLen, bishops);
    }
    if (rooks && strlen(rooks) > 0) {
        if (resultLen > 0) appendString(&result, &resultSize, &resultLen, " ");
        appendString(&result, &resultSize, &resultLen, rooks);
    }
    if (queens && strlen(queens) > 0) {
        if (resultLen > 0) appendString(&result, &resultSize, &resultLen, " ");
        appendString(&result, &resultSize, &resultLen, queens);
    }
    if (king && strlen(king) > 0) {
        if (resultLen > 0) appendString(&result, &resultSize, &resultLen, " ");
        appendString(&result, &resultSize, &resultLen, king);
    }

    // Free memory allocated by individual capture functions
    free(pawns);
    free(knights);
    free(bishops);
    free(rooks);
    free(queens);
    free(king);

    return result;
}

// Function that returns all possible legal moves from a string given
char *LegalMoves(Board board, const char *moves) {
    // Validate input
    if (!board || !moves) return NULL;

    // Allocate memory for result
    char *result = malloc(1024);
    if (!result) return NULL;
    result[0] = '\0';

    // Duplicate the moves string
    char *movesCopy = strdup(moves);
    if (!movesCopy) {
        free(result);
        return NULL;
    }

    // Tokenize the moves string
    char *token = strtok(movesCopy, " ");
    while (token != NULL) {

        // Allocate memory for tempBoard
        Board tempBoard = malloc(sizeof(struct board));
        if (!tempBoard) {
            free(movesCopy);
            free(result);
            return NULL;
        }

        // Validate move token length before accessing `token[i+1]` or `token[i+2]`
        int k = strlen(token);
        int flag = 0;
        for (int i = 0; i < k - 2; i++) {  // Ensure we don't access out-of-bounds
            if (token[i] == 'x') {
                char sFile = token[i + 1];
                char sRank = token[i + 2];

                // debugPrint("attack move: %c%c\n", sFile, sRank);
                // debugPrint("enemy king square: %d\n", EnemyKingSquare(board));

                int sqr = 56 + (sFile - 'a') - ((sRank - '1') * 8);
                if (sqr == EnemyKingSquare(board)) {
                    token = strtok(NULL, " ");
                    free(tempBoard);
                    flag = 1;
                    break;
                }
            }
        }

        if (flag) {
            token = strtok(NULL, " ");  // Move to the next token
            continue;
        }

        // Initialize and copy board data safely
        memset(tempBoard, 0, sizeof(struct board));
        memcpy(tempBoard, board, sizeof(struct board));

        // Update the board with the move
        UpdateBitboards(tempBoard, token);

        // If the king is not attacked, append the move to the result
        if (!isKingAttacked(tempBoard)) {
            if (strlen(result) > 0) {
                strcat(result, " ");
            }
            strcat(result, token);
        }

        // Free tempBoard before processing the next token
        free(tempBoard);

        token = strtok(NULL, " ");
    }

    free(movesCopy);

    return result;
}


char *generateLegalCaptures(Board board) {
    // Check if the board is valid
    if(!board) return NULL;

    // Generate all possible captures
    char *allCaptures = generateAllCaptures(board);
    if (!allCaptures) return NULL;

    // Filter illegal captures
    char *legalCaptures = LegalMoves(board, allCaptures);
    free(allCaptures);
    if(!legalCaptures) return NULL;
    
    return legalCaptures;
}

char *generateLegalMoves(Board board) {
    
    // Check if the board is valid
    if(!board) return NULL;

    // Generate all possible moves
    char *allMoves = generateAllMoves(board);
    if (!allMoves) return NULL;

    // filter illegal moves
    char *legalMoves = LegalMoves(board, allMoves);
    if(!legalMoves){
        free(allMoves);
        return NULL;
    }

    // Debug print to determine progress
    debugPrint("Legal moves: %s\n", legalMoves);
    free(allMoves);
    return legalMoves;
}