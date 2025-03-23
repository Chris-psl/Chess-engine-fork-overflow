#include "bitboard.h"
#include "init.h"
#include "evaluate.h"
#include "movegen.h"
#include "tools.h"
#include "capture.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAX_MOVES 256  // Maximum moves that can be generated (can be adjusted)

//Checks if a square is occupied by any piece
int isOccupied(Board board, int sqr) {
    if (!board) {
        debugPrint("Error: Board is NULL in isOccupied\n");
        return 0;
    }

    for (int i = 0; i < 12; i++) {
        if (IS_BIT_SET(board->bitboards[i], sqr)) {
            return 1;
        }
    }
    return 0;
}


// Function to convert square index to chess notation (e.g., 0 -> "a1", 63 -> "h8")
char* getSquareName(short int sqr) {
    char *squareName = malloc(3 * sizeof(char));  // Allocate memory for "a1\0"
    if (!squareName) {
        debugPrint("Memory allocation failed\n");
        return NULL;
    }

    //squareName[0] = 'h' - (sqr % 8);  // File ('a' to 'h')
    squareName[0] = 'a' + (sqr % 8);  // File ('a' to 'h')
    //WRONG: squareName[1] = '1' + (sqr / 8);  // Rank ('1' to '8')
    squareName[1] = '8' - (sqr / 8);  // Rank ('1' to '8')
    squareName[2] = '\0';  // Null-terminate the string

    return squareName;  // Caller must free this memory
}

//--- Sub-functions for generating moves for each piece type ---(non-attack type) ---


int updateEnPassant(Board board, const char *move) {
    if (strlen(move) < 4) return 0; // Ensure valid move format

    char currentPlayer = board->toMove;
    unsigned long long enemyPieces = 0;

    // Get enemy pieces bitboard
    if (currentPlayer == 'w') {
        for (int i = 6; i < 12; i++) {
            enemyPieces |= board->bitboards[i]; // Black pieces
        }
    } else {
        for (int i = 0; i < 6; i++) {
            enemyPieces |= board->bitboards[i]; // White pieces
        }
    }

    // Convert destination square to index
    int destFile = move[2] - 'a';
    int destRank = move[3] - '1';
    int destIndex = destRank * 8 + destFile;

    // Check left and right squares while ensuring they are in the same row
    int leftIndex = (destFile > 0) ? destIndex - 1 : -1;
    int rightIndex = (destFile < 7) ? destIndex + 1 : -1;

    // Ensure left and right squares are in the same row as the destination
    if ((leftIndex != -1 && (leftIndex / 8 == destRank) && (enemyPieces & (1ULL << leftIndex))) ||
        (rightIndex != -1 && (rightIndex / 8 == destRank) && (enemyPieces & (1ULL << rightIndex)))) {
        board->pass[0] = move[2];
        board->pass[1] = move[3];
        return 1; // Enemy piece found on the left or right in the same row
    }
    board->pass[0] = '-';
    board->pass[1] = '\0';
    return 0; // No adjacent enemy pieces in the same row
}

// For Pawns bug on A rank for white pawns
char *generatePawnMoves(Board board) {
    size_t resultSize = 256;
    char *result = malloc(resultSize);
    if (!result) return NULL;
    result[0] = '\0';
    size_t resultLen = 0;

    // Validate board state
    if (!board || !(board->toMove == 'w' || board->toMove == 'b')) {
        free(result);
        return NULL;
    }
    int pawncount = 0;
    if (board->toMove == 'b') { // Black pawns move up
        for (int i = 8; i < 56; i++) {
            if(pawncount > 8) break;
            if (IS_BIT_SET(board->bitboards[BLACK_PAWNS], i)) {
                pawncount++;
                // Handle promotions
                if (i >= 48 && i <= 55) {
                    // QUEENS
                    char *to = getSquareName(i + 8);
                    appendString(&result, &resultSize, &resultLen, to);
                    appendString(&result, &resultSize, &resultLen, "=Q ");

                    // KNIGHTS
                    appendString(&result, &resultSize, &resultLen, to);
                    appendString(&result, &resultSize, &resultLen, "=N ");
                    free(to);
                    continue;
                }
                // Single-step move
                if (isOccupied(board, i + 8) == 0) {
                    char *to = getSquareName(i + 8);
                    appendString(&result, &resultSize, &resultLen, to);
                    appendString(&result, &resultSize, &resultLen, " ");
                    free(to);
                    // Two-square advance
                    if (i >= 8 && i <= 15 && isOccupied(board, i + 16) == 0) {
                        char *to = getSquareName(i + 16);
                        appendString(&result, &resultSize, &resultLen, to);
                        appendString(&result, &resultSize, &resultLen, " ");
                        free(to);
                    }
                }
            }
        }
    } else { // White pawns move down
        for (int i = 55; i > 7; i--) {
            if(pawncount >= 8) break;
            if (IS_BIT_SET(board->bitboards[WHITE_PAWNS], i)) {
                pawncount++;
                // Handle promotions
                if (i >= 8 && i <= 15) {
                    char *to = getSquareName(i - 8);
                    appendString(&result, &resultSize, &resultLen, to);
                    appendString(&result, &resultSize, &resultLen, "=Q ");
                    appendString(&result, &resultSize, &resultLen, to);
                    appendString(&result, &resultSize, &resultLen, "=N ");
                    free(to);
                    continue;
                }
                // Single-step move
                if (isOccupied(board, i - 8) == 0) {
                    char *to = getSquareName(i - 8);
                    appendString(&result, &resultSize, &resultLen, to);
                    appendString(&result, &resultSize, &resultLen, " ");
                    free(to);
                    // Two-square advance
                    if (i >= 48 && i <= 55 && isOccupied(board, i - 16) == 0) {
                        char *to = getSquareName(i - 16);
                        appendString(&result, &resultSize, &resultLen, to);
                        appendString(&result, &resultSize, &resultLen, " ");
                        free(to);
                    }
                }
            }
        }
    }

    // Remove trailing space, if any
    size_t len = strlen(result);
    if (len > 0 && result[len - 1] == ' ') {
        result[len - 1] = '\0';
    }

    return result;
}

// Function to generate all possible bishop moves
char *generateBishopMoves(Board board) {

    if(!board) return NULL;
    uint64_t bishopBitboard, enemyPieces, occupancy = 0ULL;
    char *result = malloc(1024); // Allocate initial memory for the result string
    if (!result) return NULL;
    result[0] = '\0';

    // Bishop movement directions (diagonals)
    int bishopDirections[4] = {9, 7, -7, -9};

    // Identify bishops, occupied squares, and enemy pieces
    if (board->toMove == 'w') {
        bishopBitboard = board->bitboards[WHITE_BISHOPS];
        enemyPieces = board->bitboards[BLACK_PAWNS] | board->bitboards[BLACK_ROOKS] |
                      board->bitboards[BLACK_KNIGHTS] | board->bitboards[BLACK_BISHOPS] |
                      board->bitboards[BLACK_QUEEN] | board->bitboards[BLACK_KING];
        occupancy = board->bitboards[WHITE_PAWNS] | board->bitboards[WHITE_ROOKS] |
                    board->bitboards[WHITE_KNIGHTS] | board->bitboards[WHITE_BISHOPS] |
                    board->bitboards[WHITE_QUEEN] | board->bitboards[WHITE_KING] | enemyPieces;
    } else {
        bishopBitboard = board->bitboards[BLACK_BISHOPS];
        enemyPieces = board->bitboards[WHITE_PAWNS] | board->bitboards[WHITE_ROOKS] |
                      board->bitboards[WHITE_KNIGHTS] | board->bitboards[WHITE_BISHOPS] |
                      board->bitboards[WHITE_QUEEN] | board->bitboards[WHITE_KING];
        occupancy = board->bitboards[BLACK_PAWNS] | board->bitboards[BLACK_ROOKS] |
                    board->bitboards[BLACK_KNIGHTS] | board->bitboards[BLACK_BISHOPS] |
                    board->bitboards[BLACK_QUEEN] | board->bitboards[BLACK_KING] | enemyPieces;
    }

    size_t bufferSize = 1024; // Initial buffer size
    size_t moveLen = 0;

    int bishopcount = 0;
    // Iterate over all squares to find bishops
    for (int square = 0; square < 64; square++) {
        // if bishop count is greater than 2, break
        if(bishopcount >= 2) break;
        if (IS_BIT_SET(bishopBitboard, square)) {
            bishopcount++;
            char from[3], to[3];

            // Convert the starting square to algebraic notation
            squareToAlgebraic(square, from);

            // Check each diagonal direction
            for (int d = 0; d < 4; d++) {
                int current = square;
                while (1) {
                    int prev = current;
                    current += bishopDirections[d];

                    // Check if the new position is within bounds
                    if (current < 0 || current >= 64)
                        break;

                    // Ensure the move remains diagonal
                    if (abs((current % 8) - (prev % 8)) != 1)
                        break;

                    // If a piece is encountered, the bishop stops moving
                    if (IS_BIT_SET(occupancy, current)) {
                        break; // Stop in this direction
                    }

                    // Convert the target square to algebraic notation
                    squareToAlgebraic(current, to);

                    // Construct the move string manually
                    appendString(&result, &bufferSize, &moveLen, "B");
                    appendString(&result, &bufferSize, &moveLen, from);
                    appendString(&result, &bufferSize, &moveLen, to);
                    appendString(&result, &bufferSize, &moveLen, " ");
                }
            }
        }
    }

    // Remove trailing space, if any
    size_t len = strlen(result);
    if (len > 0 && result[len - 1] == ' ')
        result[len - 1] = '\0';

    return result;
}

// Function to generate all possible knight moves -- Bug generates one less move
char *generateKnightMoves(Board board) {
    if(!board) return NULL;
    uint64_t knightBoard;
    char currentPlayer = board->toMove;

    // Allocate memory for moves
    char *moveList = calloc(256, sizeof(char)); // Initial allocation for 256 moves
    if (!moveList) {
        debugPrint("Memory allocation failed\n");
        return NULL;
    }
    size_t bufferSize = 256 * 6;  // Initial size for 256 moves
    size_t moveLen = 0;

    // Get current player's knights
    if (currentPlayer == 'w') {
        knightBoard = board->bitboards[WHITE_KNIGHTS]; // White knights
    } else {
        knightBoard = board->bitboards[BLACK_KNIGHTS]; // Black knights
    }

    // Define all possible knight move offsets
    const int knightMoves[8][2] = {
        {2, 1}, {1, 2}, {-1, 2}, {-2, 1},
        {-2, -1}, {-1, -2}, {1, -2}, {2, -1}
    };
    
    // Iterate over all squares to find knights
    for (int square = 0; square < 64; square++) {
        
        if (IS_BIT_SET(knightBoard, square)) {
            
            //debugPrint("Knight found at square %d\n", square);

            // Extract the rank and file of the knight's position
            int knightRank = square / 8;
            int knightFile = square % 8;

            // Loop through possible knight moves
            for (int i = 0; i < 8; i++) {
                int newRank = knightRank + knightMoves[i][0];
                int newFile = knightFile + knightMoves[i][1];

                // Ensure the target square is within bounds
                if (newRank >= 0 && newRank < 8 && newFile >= 0 && newFile < 8) {
                    int movePos = newRank * 8 + newFile;

                    // Ensure the target square is empty
                    if (isOccupied(board, movePos) == 0) {
                        // Create the move string
                        char move[7];  // "Na1b2" + space + null terminator
                        move[0] = 'N';
                        move[1] = 'a' + knightFile;
                        move[2] = '8' - knightRank;
                        move[3] = 'a' + newFile;
                        move[4] = '8' - newRank;
                        move[5] = ' ';
                        move[6] = '\0';

                        // Append the move to the move list
                        appendString(&moveList, &bufferSize, &moveLen, move);
                    }
                }
            }
        }
    }

    // Remove trailing space, if any
    if (moveLen > 0 && moveList[moveLen - 1] == ' ') {
        moveList[moveLen - 1] = '\0';
    }

    return moveList;
}

// For Rooks -- Bug here doesnt recognise occupied squares
char *generateRookMoves(Board board) {
    uint64_t rookBoard;
    char currentPlayer = board->toMove;
    char *moveList = calloc(1024, sizeof(char)); // Allocate memory for moves
    if (!moveList) {
        debugPrint("Memory allocation failed\n");
        return NULL;
    }

     // Allocate initial memory for the move list
     size_t bufferSize = 256 * 6;  // Initial size for 256 moves
     size_t moveLen = 5;
     int moveCount = 0;

    // Get current player's rooks
    if (currentPlayer == 'w') {
        rookBoard = board->bitboards[WHITE_ROOKS]; // White rooks
    } else {
        rookBoard = board->bitboards[BLACK_ROOKS]; // Black rooks
    }

    int rookcount = 0;
    // Iterate over all squares to find rooks
    for (int square = 0; square < 64; square++) {
        if(rookcount >= 2 ) break;
        if (IS_BIT_SET(rookBoard, square)) {
            rookcount++;
            //debugPrint("Rook found at square %d\n", square);
            // Define directions for rook movement (vertical & horizontal)
            int directions[8] = {1, -1, 8, -8};

            // Loop through each direction
            for (int d = 0; d < 4; d++) {
                int newPos = square + directions[d];
                //debugPrint("New position: %d\n", newPos);

                // Keep moving in the given direction until out of bounds or blocked
                while (1) {
                    if (newPos < 0 || newPos >= 64) break; // Stop if out of bounds
                    if ((directions[d] == -1 && square % 8 < newPos % 8) || 
                        (directions[d] == 1 && square % 8 > newPos % 8)) break;

                    // Ensure the target square is empty
                    if (isOccupied(board, newPos) == 0) {
                        // Create the move string
                        char move[7];  // "Ra1b2" + space + null terminator
                        move[0] = 'R';
                        move[1] = 'a' + (square % 8);
                        move[2] = '8' - (square / 8);
                        move[3] = 'a' + (newPos % 8);
                        move[4] = '8' - (newPos / 8);
                        move[5] = ' ';
                        move[6] = '\0';

                        // Append the move to the move list
                        appendString(&moveList, &bufferSize, &moveLen, move);
                        moveCount++;
                    } else {
                        break; // Stop if the square is occupied
                    }

                    // Move to the next position in the same direction
                    newPos += directions[d];
                }
            }
        }
    }

    // Remove trailing space, if any
    size_t len = strlen(moveList);
    if (len > 0 && moveList[len - 1] == ' ')
        moveList[len - 1] = '\0';

    return moveList;
}

// For Queens
char *generateQueenMoves(Board board) {
    // Directions: -8 (up), 8 (down), -1 (left), 1 (right), -9 (up-left), -7 (up-right), 9 (down-right), 7 (down-left)

    if(!board) return NULL;

    // Allocate initial memory for the move list
    size_t bufferSize = 256 * 6;  // Initial size for 256 moves
    size_t moveLen = 5;

    unsigned long long queenBoard = 0ULL;
    int directions[8] = {-8, 8, -1, 1, -9, -7, 9, 7};
    char currentPlayer = board->toMove;
    char *moveList = calloc(bufferSize, sizeof(char));
    if(!moveList) return NULL;
    
    int moveCount = 0;

    // Get the queen's bitboard based on the current player
    queenBoard = (currentPlayer == 'w') ? board->bitboards[WHITE_QUEEN] : board->bitboards[BLACK_QUEEN];

    int queenPos = -1;

    // Finds the position of the queen
    for(int i = 0; i<64; i++){
        if(IS_BIT_SET(queenBoard, i)){
            queenPos = i;
            break;
        }else if(i == 63 && queenPos == -1){
            free(moveList);
            return NULL;
        }
    }

    // Generate moves in all 8 directions
    for (int i = 0; i < 8; i++) {
        int step = directions[i];
        int pos = queenPos;

        while (1) {
            pos += step;

            // Stop if out of bounds
            if (pos < 0 || pos >= 64) break;

            // Prevent file wraparound for horizontal moves
            if ((step == -1 && pos % 8 == 7) || (step == 1 && pos % 8 == 0)) break;

            // Prevent rank/file inconsistency for diagonal moves
            if ((step == -9 && pos % 8 == 7) ||  // Moving up-left
                (step == -7 && pos % 8 == 0) ||  // Moving up-right
                (step == 9 && pos % 8 == 0) ||   // Moving down-right
                (step == 7 && pos % 8 == 7)) {   // Moving down-left
                break;
            }
            
            // If the square is not occupied, add the move
            if (isOccupied(board, pos) == 0) {
                // Create the move string
                char move[7];  // "Qa1b2" + space + null terminator
                move[0] = 'Q';
                move[1] = 'a' + (queenPos % 8);
                move[2] = '8' - (queenPos / 8);
                move[3] = 'a' + (pos % 8);
                move[4] = '8' - (pos / 8);
                move[5] = ' ';
                move[6] = '\0';

                // Append the move to the move list
                appendString(&moveList, &bufferSize, &moveLen, move);
                moveCount++;
            }else{
                break;
            }
        }
    }
    

    // Remove trailing space, if any.
    size_t len = strlen(moveList);
    if (len > 0 && moveList[len - 1] == ' ') {
        moveList[len - 1] = '\0';
    }

    char *validMoves = filter_valid_moves(moveList);
    free(moveList);
    if(!validMoves) return NULL;
    
    return validMoves;
}

// For Kings -- havent found errors for now
char *generateKingMoves(Board board) {
    unsigned long long kingBoard;
    int kingDirections[8] = {-8, 8, -1, 1, -9, -7, 9, 7};
    char currentPlayer = board->toMove;

    // Allocate initial memory for the move list
    size_t bufferSize = 256 * 6;  // Initial size for 256 moves
    char *moveList = calloc(bufferSize, sizeof(char));
    if (!moveList) {
        debugPrint("Memory allocation failed\n");
        return NULL;
    }

    size_t moveLen = 0;
    kingBoard = (currentPlayer == 'w') ? board->bitboards[WHITE_KING] : board->bitboards[BLACK_KING];
    int kingPos = 0;

    // Find the king's position
    for (int i = 0; i < 64; i++) {
        if (IS_BIT_SET(kingBoard, i)) {
            kingPos = i;
            break;
        }
    }

    // Generate moves
    for (int i = 0; i < 8; i++) {
        int newPos = kingPos + kingDirections[i];
        if (newPos < 0 || newPos >= 64 || 
            (kingDirections[i] == -1 && kingPos % 8 == 0) || 
            (kingDirections[i] == 1 && kingPos % 8 == 7)) {
            continue;
        }
        if (isOccupied(board, newPos) == 0) {
            // Create the move string
            char move[7];  // "Ka1b2" + space + null terminator
            move[0] = 'K';
            move[1] = 'a' + (kingPos % 8);
            move[2] = '8' - (kingPos / 8);
            move[3] = 'a' + (newPos % 8);
            move[4] = '8' - (newPos / 8);
            move[5] = ' ';
            move[6] = '\0';

            // Append the move to the move list
            if (!appendString(&moveList, &bufferSize, &moveLen, move)) {
                debugPrint("Failed to append move to list\n");
                free(moveList);
                return NULL;
            }
        }
    }

    // Remove trailing space, if any
    size_t len = strlen(moveList);
    if (len > 0 && moveList[len - 1] == ' ') {
        moveList[len - 1] = '\0';
    }

    return moveList;
}

// Basically a function that unifies all the sub-functions -- should be error free
char *generateAllMoves(Board board){

    // Check if the board is valid
    if(!board){
        debugPrint("Board is NULL\n");
        return NULL;
    }

    char *result = malloc(1); // Start with a single byte for the null terminator
    if (!result) return NULL;
    result[0] = '\0';
    size_t resultSize = 1; // Initial size of the result buffer
    size_t resultLen = 0;  // Current length of the result string

    // Debug print to determine position
    if(DEBUG)printBoard(board);

    // Generate captures for each piece type
    debugPrint("\nGenerating regular moves...\n");
    char *pawns = generatePawnMoves(board);
    debugPrint("Pawns: %s\n", pawns);
    char *knights = generateKnightMoves(board);
    debugPrint("Knights: %s\n", knights);
    char *bishops = generateBishopMoves(board);
    debugPrint("Bishops: %s\n", bishops);
    char *rooks = generateRookMoves(board);
    debugPrint("Rooks: %s\n", rooks);
    char *queens = generateQueenMoves(board);
    debugPrint("Queens: %s\n", queens);
    char *king = generateKingMoves(board);
    debugPrint("King: %s\n", king);
    char *captureMoves = generateAllCaptures(board); // Generate all possible captures non entirely tested

    //Append captures to the result string
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
    if (captureMoves && strlen(captureMoves) > 0) {
        if (resultLen > 0) appendString(&result, &resultSize, &resultLen, " ");
        appendString(&result, &resultSize, &resultLen, captureMoves);
    }

    //Free memory allocated by individual capture functions
    free(pawns);
    free(knights);
    free(bishops);
    free(rooks);
    free(queens);
    free(king);
    free(captureMoves);

    // Remove trailing space, if any.
    size_t len = strlen(result);
    if (len > 0 && result[len - 1] == ' ')
        result[len - 1] = '\0';

    
    char *filtered = filter_valid_moves(result);
    if(!filtered) {
        free(result);
        return NULL;
    }
    free(result);
    return filtered;
}

char *generateMoveMoves(Board board, const char *captureMoves) {

       // Check if the board is valid
       if(!board || !captureMoves){
        debugPrint("Board is NULL\n");
        return NULL;
    }

    char *result = malloc(1); // Start with a single byte for the null terminator
    if (!result) return NULL;
    result[0] = '\0';
    size_t resultSize = 1; // Initial size of the result buffer
    size_t resultLen = 0;  // Current length of the result string

    // Debug print to determine position
    if(DEBUG)printBoard(board);

    // Generate captures for each piece type
    debugPrint("\nGenerating regular moves...\n");
    char *pawns = generatePawnMoves(board);
    debugPrint("Pawns: %s\n", pawns);
    char *knights = generateKnightMoves(board);
    debugPrint("Knights: %s\n", knights);
    char *bishops = generateBishopMoves(board);
    debugPrint("Bishops: %s\n", bishops);
    char *rooks = generateRookMoves(board);
    debugPrint("Rooks: %s\n", rooks);
    char *queens = generateQueenMoves(board);
    debugPrint("Queens: %s\n", queens);
    char *king = generateKingMoves(board);
    debugPrint("King: %s\n", king);

    //Append captures to the result string
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
    if (captureMoves && strlen(captureMoves) > 0) {
        if (resultLen > 0) appendString(&result, &resultSize, &resultLen, " ");
        appendString(&result, &resultSize, &resultLen, captureMoves);
    }

    //Free memory allocated by individual capture functions
    free(pawns);
    free(knights);
    free(bishops);
    free(rooks);
    free(queens);
    free(king);
    //free(captureMoves);

    // Remove trailing space, if any.
    size_t len = strlen(result);
    if (len > 0 && result[len - 1] == ' ')
        result[len - 1] = '\0';

    
    char *filtered = filter_valid_moves(result);
    if(!filtered) {
        free(result);
        return NULL;
    }
    free(result);
    return filtered;
}