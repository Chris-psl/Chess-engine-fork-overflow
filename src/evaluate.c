#include "init.h"
#include "evaluate.h"
#include "bitboard.h"
#include <stdio.h>
#include <stdlib.h>

// Library for evaluating a single move

int values[12] = {P_VALUE, R_VALUE, N_VALUE, B_VALUE, Q_VALUE, K_VALUE, P_VALUE, R_VALUE, N_VALUE, B_VALUE, Q_VALUE, K_VALUE};
int sqrValues[3] = {VAL_GR_CENTER, VAL_GO_CENTER, VAL_OUT_CENTER};
int gameState = 1;
// 0 for opening
// 1 for middle game
// 2 for end game

// Function pointer type for evaluation functions
typedef int (*EvalFunction)(unsigned long long *, short int, char);

// Function to evaluate a piece value based on its index the game state
// It receives a square the bitboard and the active player
int getEnemyPieceValue(unsigned long long bitboards[12], short int sqr, char usPlayer) {
    char enemyPlayer = (usPlayer == 'w') ? 'b' : 'w';
    debugPrint("Enemy player: %c\n", enemyPlayer);
    int piece = whatPiece(bitboards, sqr);
    if (piece == -1) return 0;
    return values[piece];
}

//----------------------------------------------------------------------------
// calculateAttackedScore: if the square is attacked it will return the score of the 
// attacking piece, if not it will return 0
//----------------------------------------------------------------------------
int calculateAttckedScore(unsigned long long bitboards[12], int square, char usPlayer) {
    // Determine enemy color: if board->toMove is 'w', enemy is white? 
    // (Since you typically call this to see if your king is in check, enemy is
    // the opposite of the side to move.)
    // Here we assume we want to know if 'square' is attacked by the opponent.
    char enemyColor = (usPlayer == 'w' ? 'b' : 'w');
    int totalScore = 0;

    // Compute the full occupancy (all pieces on the board).
    unsigned long long occupancy = 0ULL;
    for (int i = 0; i < 12; i++) {
        occupancy |= bitboards[i];
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
        if (s1 >= 0 && (s1 % 8) != 7 && IS_BIT_SET(bitboards[WHITE_PAWNS], s1))
            totalScore += getEnemyPieceValue(bitboards, s1, enemyColor);
        if (s2 >= 0 && (s2 % 8) != 0 && IS_BIT_SET(bitboards[WHITE_PAWNS], s2))
            totalScore += getEnemyPieceValue(bitboards, s2, enemyColor);
    } else { // enemyColor == 'b'; Enemy is Black.
        int s1 = square + 7;  // black pawn that would attack from the right
        int s2 = square + 9;  // black pawn that would attack from the left
        if (s1 < 64 && (s1 % 8) != 7 && IS_BIT_SET(bitboards[BLACK_PAWNS], s1))
            totalScore += getEnemyPieceValue(bitboards, s1, enemyColor);
        if (s2 < 64 && (s2 % 8) != 0 && IS_BIT_SET(bitboards[BLACK_PAWNS], s2))
            totalScore += getEnemyPieceValue(bitboards, s2, enemyColor);
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
            if (IS_BIT_SET(bitboards[WHITE_KNIGHTS], s))
                totalScore += getEnemyPieceValue(bitboards, s, enemyColor);
        } else {
            if (IS_BIT_SET(bitboards[BLACK_KNIGHTS], s))
                totalScore += getEnemyPieceValue(bitboards, s, enemyColor);
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
                    if (IS_BIT_SET(bitboards[WHITE_ROOKS], current) ||
                        IS_BIT_SET(bitboards[WHITE_QUEEN], current))
                        totalScore += getEnemyPieceValue(bitboards, current, enemyColor);
                } else {
                    if (IS_BIT_SET(bitboards[BLACK_ROOKS], current) ||
                        IS_BIT_SET(bitboards[BLACK_QUEEN], current))
                        totalScore += getEnemyPieceValue(bitboards, current, enemyColor);
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
                    if (IS_BIT_SET(bitboards[WHITE_BISHOPS], current) ||
                        IS_BIT_SET(bitboards[WHITE_QUEEN], current))
                        totalScore += getEnemyPieceValue(bitboards, current, enemyColor);
                } else {
                    if (IS_BIT_SET(bitboards[BLACK_BISHOPS], current) ||
                        IS_BIT_SET(bitboards[BLACK_QUEEN], current))
                        totalScore += getEnemyPieceValue(bitboards, current, enemyColor);
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
            if (IS_BIT_SET(bitboards[WHITE_KING], s))
                totalScore += getEnemyPieceValue(bitboards, s, enemyColor);
        } else {
            if (IS_BIT_SET(bitboards[BLACK_KING], s))
                totalScore += getEnemyPieceValue(bitboards, s, enemyColor);
        }
    }

    // If none of the enemy pieces attack the square, return 0.
    return totalScore;
}

// adds the value of a square with taking into account the piece capturing, the piece being captured
// int piece is the piece being captured
// int sqr is the destination square
// char usPlayer is the current player
// int moving piece is the piece being moved
// int StartSqr is the starting square of movement
int addValue(unsigned long long bitboards[12], int piece, int sqr, char usPlayer, int movingPiece, int StartSqr) {
    int value = 0;
    
    //discourage king moving from his starting square
    if (((movingPiece == BLACK_KING) && (StartSqr == 4) && (sqr != 2 && sqr != 6)) || ((movingPiece == WHITE_KING) && (StartSqr == 60)&& (sqr != 58 && sqr != 62))) {value -= 10*BONUS_K_VALUE;}

    if ((((movingPiece >= 0 && movingPiece <= 5) && (piece <= 11 && piece >= 6)) || ((movingPiece <= 11 && movingPiece >= 6) && (piece >= 0 && piece <= 5)))) {

        // if it can capture, add  the value of the captured piece
        if(values[piece] > values[movingPiece]) value += 2*values[piece];
        if(values[piece] == values[movingPiece]) value += 1.5*values[piece];
        
        if (gameState != 2) {
            
            // add extra value if you ruin castling for the king in question, by attacking him in his starting position
            if (((piece == BLACK_KING) && (sqr == 4)) || ((piece == WHITE_KING) && (sqr == 60))) {value += BONUS_K_VALUE;}

            // if castling is ruined due to the rook moving away from its starting position remove some value
            if ((movingPiece == BLACK_ROOKS) && (StartSqr == 0 || StartSqr == 7)) value -= BONUS_K_VALUE;
            if ((movingPiece == WHITE_ROOKS) && (StartSqr == 56 || StartSqr == 63)) value -= BONUS_K_VALUE;
        }

        // if piece is white pawn and destination rank is 4 and starting rank is 2
        if((movingPiece == WHITE_PAWNS) && ((8 - (sqr/8)) == 4) && ((8 - (StartSqr/8)) == 2)) value += 3*P_VALUE;

        // if piece is black pawn and destination rank is 4 and starting rank is 2
        if((movingPiece == BLACK_PAWNS) && ((8 - (sqr/8)) == 5) && ((8 - (StartSqr/8) + 0) == 7)) value += 3*P_VALUE;

        // add extra value for the ability to capture pawns that can promote soon
        if (piece == WHITE_PAWNS && (sqr >= 16 && sqr <= 23)) value += 3*values[piece];
        if (piece == WHITE_PAWNS && (sqr >= 8 && sqr <= 15)) value += 6*values[piece];
        if (piece == BLACK_PAWNS && (sqr >= 40 && sqr <= 47)) value += 3*values[piece];
        if (piece == BLACK_PAWNS && (sqr >= 48 && sqr <= 55)) value += 6*values[piece];

    } else if (piece == -1) {
            // if its empty add the squares value
            value += squareValue(sqr);

    } else {
            // not sure what this does here but it makes it way smarter
            value += values[piece];

            // the piece in the space is the same as the piece trying to capture
            // if we are protecting a piece more important than ourself, add its value
            if ((values[piece] < values[movingPiece]) && (movingPiece == WHITE_PAWNS || movingPiece == BLACK_PAWNS)) value += 2*values[piece];
    }

    // give double value if the piece moving is a king in the endgame
    if ((gameState == 2) && (piece == BLACK_KING || piece == WHITE_KING)) value *= 2;
    
    // evaluate square threats
    //value += calculateAttckedScore(bitboards, sqr, usPlayer);
    if(!DEBUG)debugPrint("%d\n", IS_BIT_SET(bitboards[BLACK_PAWNS], sqr));
    if(!DEBUG)debugPrint("usPlayer: %c\n", usPlayer);
    
    return value;
}


// checks the 4 cardinal direction starting from one square of the board
// returns value depending on possible moves and holdings 
// used for rooks and queens (yasss)
int checkCross(unsigned long long bitboards[12], short int sqr, char usPlayer) {
    int totalCrossValue = 0, coveredSquares = 0;

    // Downwards check
    int temp_sqr = sqr, piece;
    while (1) { // breaks when an edge or a piece is encountered
        if (temp_sqr == sqr) { // if on the first iteration of the loop, no reason to check current spot, its the starting one
            if (temp_sqr >= 56 && temp_sqr <= 63) break; // we reached the down edge - stop!
            temp_sqr += 8;
            coveredSquares++;
            continue;
        }

        piece = whatPiece(bitboards, temp_sqr);
        totalCrossValue += addValue(bitboards, piece, temp_sqr, usPlayer, whatPiece(bitboards, sqr), sqr);
        if (piece == -1) {
            if (temp_sqr >= 56 && temp_sqr <= 63) break;// we reached the down edge - stop!
            temp_sqr += 8;
            coveredSquares++;
            continue;
        }
        break;
    }

    // Upwards check
    temp_sqr = sqr;
    while (1) { // breaks when an edge or a piece is encountered
        if (temp_sqr == sqr) { // if on the first iteration of the loop, no reason to check current spot, its the starting one
            if (temp_sqr >= 0 && temp_sqr <= 7) break;// we reached the down edge
            temp_sqr += -8;
            coveredSquares++;
            continue;
        }
        piece = whatPiece(bitboards, temp_sqr);
        totalCrossValue += addValue( bitboards, piece, temp_sqr, usPlayer, whatPiece(bitboards, sqr), sqr);
        if (piece == -1) {
            if (temp_sqr >= 0 && temp_sqr <= 7) break;// we reached the down edge
            temp_sqr += -8;
            coveredSquares++;
            continue;
        } 
        break;
    }

    // Right check
    temp_sqr = sqr;
    while (1) { // breaks when an edge or a piece is encountered
        if (temp_sqr == sqr) { // if on the first iteration of the loop, no reason to check current spot, its the starting one
            if (temp_sqr % 8 == 7) break;// we reached the down edge
            temp_sqr += 1;
            coveredSquares++;
            continue;
        }
        piece = whatPiece(bitboards, temp_sqr);
        totalCrossValue += addValue( bitboards, piece, temp_sqr, usPlayer, whatPiece(bitboards, sqr), sqr);
        if (piece == -1) {
            if (temp_sqr % 8 == 7) break;// we reached the down edge
            temp_sqr += 1;
            coveredSquares++;
            continue;
        } 
        break;
    }

    // Left check
    temp_sqr = sqr;
    while (1) { // breaks when an edge or a piece is encountered
        if (temp_sqr == sqr) { // if on the first iteration of the loop, no reason to check current spot, its the starting one
            if (temp_sqr % 8 == 0) break;// we reached the down edge
            temp_sqr += -1;
            coveredSquares++;
            continue;
        }
        piece = whatPiece(bitboards, temp_sqr);
        totalCrossValue += addValue( bitboards, piece, temp_sqr, usPlayer, whatPiece(bitboards, sqr), sqr);
        if (piece == -1) {
            if (temp_sqr % 8 == 0) break;// we reached the down edge
            temp_sqr += -1;
            coveredSquares++;
            continue;
        }
        break;
    }
    if (coveredSquares < 5) totalCrossValue /= 3;
    return totalCrossValue;
}

// checks the 4 diagonals starting from one sqr of the board
// used on bishop and queen
int checkDiagonal(unsigned long long bitboards[12], short int sqr, char usPlayer) {
    int totalDiagonalVal = 0, coveredSquares = 0;
    int directions[4] = {-8 +1, -8 -1, +8 -1, +8 +1}; // up-right, up-left, down-left, down-right

    for (int d = 0; d < 4; d++) { // Loop over 4 diagonal directions
        int temp_sqr = sqr;

        while (1) {
            // int current_row = temp_sqr / 8, 
            int current_col = temp_sqr % 8;

            // Move to the next square in the current diagonal direction
            temp_sqr += directions[d];

            // Calculate the new row and column
            int new_row = temp_sqr / 8, new_col = temp_sqr % 8;

            // Stop if out of bounds (row or column boundary reached)
            if (temp_sqr < 0 || temp_sqr >= 64 || 
                new_col < 0 || new_col > 7 || // Check column boundaries explicitly
                new_row < 0 || new_row > 7 || // Redundant but safe row bounds check
                (current_col == 0 && (directions[d] == 7 || directions[d] == -9)) || // Wrapping left
                (current_col == 7 && (directions[d] == 9 || directions[d] == -7))) { // Wrapping right
                break;
            }

            int piece = whatPiece(bitboards, temp_sqr);
            totalDiagonalVal += addValue( bitboards, piece, temp_sqr, usPlayer, whatPiece(bitboards, sqr), sqr);
            if (piece == -1) {
                // If the square is empty continue
                coveredSquares++;
                continue;
            } else {
                // If there is a piece, stop looking further
                break;
            }
        }
    }
    if (coveredSquares < 5) totalDiagonalVal /= 3;
    return totalDiagonalVal;
}

// checks the 8 possible possible positions of a knight starting from one square
// returns value depending on possible moves and holdings 
int checkKnight(unsigned long long bitboards[12], short int sqr, char usPlayer) {
    int totalKnightVal = N_VALUE;
    short int directions[8] = {1,1,1,1,1,1,1,1};
    // bool for if its possible to go to a square or not
    // 0 = down left, 1 = down right
    // 2 = right down, 3 = right up
    // 4 = up right, 5 = up left
    // 6 = left up, 7 = left down

    if (sqr % 8 == 0) { // knight on file A 
        directions[0] = 0; directions[5] = 0; directions[6] = 0; directions[7] = 0;
    }

    if (sqr % 8 == 1) { // knight on file B
        directions[6] = 0; directions[7] = 0;
    }

    if (sqr % 8 == 6) { // knight on file G
        directions[2] = 0; directions[3] = 0;
        
    }
    if (sqr % 8 == 7) { // knight on file H
        directions[1] = 0; directions[2] = 0; directions[3] = 0; directions[4] = 0;
    }

    if (sqr >= 0 && sqr <= 7) { // knight on rank 8 
        directions[6] = 0; directions[5] = 0; directions[4] = 0; directions[3] = 0; 
    }

    if (sqr >= 8 && sqr <= 15) { // knight on ranks 7
        directions[5] = 0; directions[4] = 0;
    }

    if (sqr >= 56 && sqr <= 63) { // knight on ranks 1
        directions[7] = 0; directions[0] = 0; directions[1] = 0; directions[2] = 0; 
    }

    if (sqr >= 48 && sqr <= 55) { // knight on rank 2
        directions[0] = 0; directions[1] = 0;
    }

    int piece;

    if (directions[0]) { // check the piece in down left position
        piece = whatPiece(bitboards, sqr +16 -1); // find the piece in said position
        totalKnightVal += addValue( bitboards, piece, sqr+16-1, usPlayer, whatPiece(bitboards, sqr), sqr);
    }

    if (directions[1]) { // check the piece in down right position
        piece = whatPiece(bitboards, sqr +16 +1); // find the piece in said position
        totalKnightVal += addValue( bitboards, piece, sqr +16+1, usPlayer, whatPiece(bitboards, sqr), sqr); 
    }

    if (directions[2]) { // check the piece in right down position
        piece = whatPiece(bitboards, sqr +8 +2); // find the piece in said position
        totalKnightVal += addValue( bitboards, piece, sqr+8+2, usPlayer, whatPiece(bitboards, sqr), sqr);
    }

    if (directions[3]) { // check the piece in right up position
        piece = whatPiece(bitboards, sqr -8 +2); // find the piece in said position
        totalKnightVal += addValue( bitboards, piece, sqr-8+2, usPlayer, whatPiece(bitboards, sqr), sqr);
    }
    
    if (directions[4]) { // check the piece in up right position
        piece = whatPiece(bitboards, sqr -16 +1); // find the piece in said position
        totalKnightVal += addValue( bitboards, piece, sqr -16+1, usPlayer, whatPiece(bitboards, sqr), sqr);
    }

    if (directions[5]) { // check the piece in up left position
        piece = whatPiece(bitboards, sqr -16 -1); // find the piece in said position
        totalKnightVal += addValue( bitboards, piece, sqr-16-1, usPlayer, whatPiece(bitboards, sqr), sqr); 
    }

    if (directions[6]) { // check the piece in left up position
        piece = whatPiece(bitboards, sqr -8 -2); // find the piece in said position
        totalKnightVal += addValue( bitboards, piece, sqr -8 - 2, usPlayer, whatPiece(bitboards, sqr), sqr);
    }

    if (directions[7]) { // check the piece in left down position
        piece = whatPiece(bitboards, sqr +8 -2); // find the piece in said position
        totalKnightVal += addValue( bitboards, piece, sqr +8 -2, usPlayer, whatPiece(bitboards, sqr), sqr);
    }

    return totalKnightVal;
}

// checks the four possible moves of a pawn
// returns value depending on possible moves and holdings 
int checkPawn(unsigned long long bitboards[12], short int sqr, char usPlayer) {
    int totalPawnvalue = P_VALUE, piece;
    if (usPlayer == 'w') { // white pawn plays 
        if (sqr % 8 != 0) { // if we're not on the A file check for capture possibility
            piece = whatPiece(bitboards, sqr -1 -8);
            totalPawnvalue += addValue( bitboards, piece, sqr -1 - 8, usPlayer, whatPiece(bitboards, sqr), sqr);
        }
        if (sqr % 8 != 7) {  // if we're not on the H file check for capture possibility
            piece = whatPiece(bitboards, sqr +1 -8);
            totalPawnvalue += addValue( bitboards, piece, sqr +1 - 8, usPlayer, whatPiece(bitboards, sqr), sqr);
        }
        if (whatPiece(bitboards, sqr -8) == -1) {
            totalPawnvalue += squareValue(sqr - 8);
        }
        if ((sqr >= 48 && sqr <= 55) && (whatPiece(bitboards, sqr -16) == -1)) {
            totalPawnvalue += squareValue(sqr - 16);
        }
        // add value if the pawn is close to promotion 
        if (sqr >= 16 && sqr <= 23) totalPawnvalue += 3*P_VALUE;
        if (sqr >= 8 && sqr <= 15) totalPawnvalue += 6*P_VALUE;
        

// ---------------------------------------------------------------------------------------------------------------
    } else { // black pawn plays
        if (sqr % 8 != 0) { // if we're not on the A file check for capture possibility
            piece = whatPiece(bitboards, sqr -1 +8);
            totalPawnvalue += addValue( bitboards, piece, sqr -1 + 8, usPlayer, whatPiece(bitboards, sqr), sqr);
        }
        if (sqr % 8 != 7) {  // if we're not on the H file check for capture possibility
            piece = whatPiece(bitboards, sqr +1 +8);
            totalPawnvalue += addValue( bitboards, piece, sqr +1 + 8, usPlayer, whatPiece(bitboards, sqr), sqr);
        }
        if (whatPiece(bitboards, sqr +8) == -1) {
            totalPawnvalue += squareValue(sqr + 8);
        }
        if ((sqr >= 8 && sqr <= 15) && (whatPiece(bitboards, sqr +16) == -1)) {
            totalPawnvalue += squareValue(sqr + 16);
        }
        // add value if the pawn is close to promotion 
        if (sqr >= 40 && sqr <= 47) totalPawnvalue += 3*P_VALUE;
        if (sqr >= 48 && sqr <= 55) totalPawnvalue += 6*P_VALUE;
    }
    return totalPawnvalue;
}

// checks the eight possible moves of a king
// returns value depending on possible moves and holdings 
int checkKing(unsigned long long bitboards[12], short int sqr, char usPlayer) {
    int totalKingVal = K_VALUE;
    short int directions[8] = {1,1,1,1,1,1,1,1};
    // each number in directions stands for if its possible to go in that direction or not
    // 0 = down left, 1 = down, 2 = down right
    // 3 = right 4= up right
    // 5 = up, 6 = up left, 7 = left
    
    if (sqr % 8 == 0) { // if on file A we cant go left
        directions[6] = 0; directions[7] = 0; directions[0] = 0;
    }
    if (sqr % 8 == 7) { // if on file H we cant go right
        directions[2] = 0; directions[3] = 0; directions[4] = 0;
    }
    if (sqr >= 56 && sqr <= 63) { // if on rank 1 cant go down
        directions[0] = 0; directions[1] = 0; directions[2] = 0;
        
    }
    if (sqr >= 0 && sqr <= 7) { // if on rank 8 we cant go up
        directions[4] = 0; directions[5] = 0; directions[6] = 0;
    }
    int piece;
    if (directions[0]) { // check the piece in down left position
        piece = whatPiece(bitboards, sqr + 8 -1); // checks what exists in the position
        totalKingVal = addValue( bitboards, piece, sqr + 8 -1, usPlayer, whatPiece(bitboards, sqr), sqr);
    }

    if (directions[1]) { // check the piece in down position
        piece = whatPiece(bitboards, sqr + 8);
        totalKingVal = addValue( bitboards, piece, sqr + 8, usPlayer, whatPiece(bitboards, sqr), sqr);
   }

    if (directions[2]) { // check the piece in down right position
        piece = whatPiece(bitboards, sqr +8 +1);
        totalKingVal = addValue( bitboards, piece, sqr + 8 +1, usPlayer, whatPiece(bitboards, sqr), sqr);
    }

    if (directions[3]) { // check the piece in right position
        piece = whatPiece(bitboards, sqr +1);
        totalKingVal = addValue( bitboards, piece, sqr +1, usPlayer, whatPiece(bitboards, sqr), sqr);
    }

    if (directions[4]) { // check the piece in up right position
        piece = whatPiece(bitboards, sqr -8 +1);
        totalKingVal = addValue( bitboards, piece, sqr - 8 +1, usPlayer, whatPiece(bitboards, sqr), sqr);
    }

    if (directions[5]) { // check the piece in up position
        piece = whatPiece(bitboards, sqr -8);
        totalKingVal = addValue( bitboards, piece, sqr -8, usPlayer, whatPiece(bitboards, sqr), sqr);
   }

    if (directions[6]) { // check the piece in up left position
        piece = whatPiece(bitboards, sqr -8 -1);
        totalKingVal = addValue( bitboards, piece, sqr - 8 -1, usPlayer, whatPiece(bitboards, sqr), sqr);
    }

    if (directions[7]) { // check the piece in left position
        piece = whatPiece(bitboards, sqr -1);
        totalKingVal = addValue( bitboards, piece, sqr -1, usPlayer, whatPiece(bitboards, sqr), sqr);
    }

    int pawn_count = 0;
    int castlingDiv = 0;

    // castling importance dependant on gamestate
    switch (gameState) 
        {
        case 0:
            castlingDiv = 1;
            break;
        case 1:
            castlingDiv = 5;
            break;
        case 2:
            castlingDiv = 10;
            break;
    }

    // check if the king is protected via castling
    if (usPlayer == 'w') {
        // kingside castling has happened check
        pawn_count = (((whatPiece(bitboards, 54)) == WHITE_PAWNS) + ((whatPiece(bitboards, 55) == WHITE_PAWNS)));

        if ((sqr == 62 || sqr == 63) && pawn_count == 2) {
            // castling has happened!
            totalKingVal += 5*(BONUS_K_VALUE + pawn_count*(P_VALUE)) / castlingDiv;
            // if protected from backlog mate
            if (((whatPiece(bitboards, 47)) == WHITE_PAWNS)) totalKingVal += (P_VALUE*10) / castlingDiv;
        }

        // queenside castling has happened check
        pawn_count = (((whatPiece(bitboards, 49)) == WHITE_PAWNS) + ((whatPiece(bitboards, 50) == WHITE_PAWNS)));
        if ((sqr == 56 || sqr == 57 || sqr == 58) && pawn_count == 2) {
            // castling has happened!
            totalKingVal += 5*(BONUS_K_VALUE + pawn_count*(P_VALUE)) / castlingDiv;
            // if protected from backlog mate
            if (((whatPiece(bitboards, 40)) == WHITE_PAWNS)) totalKingVal += (P_VALUE*10) / castlingDiv;
        }

    } else { // if black
        
        // kingside castling has happened check
        pawn_count = (((whatPiece(bitboards, 9)) == BLACK_PAWNS) + ((whatPiece(bitboards, 10) == BLACK_PAWNS)));
        if ((sqr == 6 || sqr == 7) && pawn_count == 2) {
            // castling has happened!
            totalKingVal += 5*(BONUS_K_VALUE + pawn_count*(P_VALUE)) / castlingDiv;
            // if protected from backlog mate
            if (((whatPiece(bitboards, 23)) == BLACK_PAWNS)) totalKingVal += (P_VALUE*10) / castlingDiv;
        }
        // queenside castling has happened check
        pawn_count = (((whatPiece(bitboards, 9)) == BLACK_PAWNS) + ((whatPiece(bitboards, 10) == BLACK_PAWNS)));
        if ((sqr == 0 || sqr == 1 || sqr == 2) && pawn_count == 2) {
            // castling has happened!
            totalKingVal += 5*(BONUS_K_VALUE + pawn_count*(P_VALUE)) / castlingDiv;
            // if protected from backlog mate
            if (((whatPiece(bitboards, 16)) == BLACK_PAWNS)) totalKingVal += (P_VALUE*10) / castlingDiv;
        }
    }

    return totalKingVal;
}

// provided the square is reachable by a piece finds returns its value (defined in brain.h)
int squareValue(short int sqr) {
    switch (sqr) {
        case D4: case D5: case E4: case E5:
            return sqrValues[0];
        case D3: case D6: case E3: case E6: case C3: case C4: 
        case C5: case C6: case F3: case F4: case F5: case F6:
            return sqrValues[1];
        default:
            return sqrValues[2];
    }
}

// checks if a square is empty of pieces
// returns pawn piece if there is a piece or returns -1 if its empty
int whatPiece(unsigned long long bitboards[12], short int sqr) {
    int piece;
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

// function to demermine the game state
int setGameState(unsigned short int fullmove, unsigned long long bitboards[12]) {

    int whitePieceCount = countPieces('w', bitboards);
    int blackPieceCount = countPieces('b', bitboards);
    int kingSquare = getKingSquare(bitboards, 'w');
    int kingSquareB = getKingSquare(bitboards, 'b');
    
    debugPrint("\nGeneral information: \n");
    debugPrint("White pieces: %d, Black pieces: %d, Fullmove: %d\n", whitePieceCount, blackPieceCount, fullmove);
    if (fullmove <= 12 && (kingSquareB == 59 || kingSquareB == 60) && (kingSquare == 3 || kingSquare == 4)) {
        return 0; // we are in the opening stage
    } else if(fullmove >= 12 && fullmove <= 30 && whitePieceCount > PIECES_ENDGAME && blackPieceCount > PIECES_ENDGAME) {
        return 1; // we are in the middle game stage
    } else if ((whitePieceCount < PIECES_ENDGAME) || (blackPieceCount < PIECES_ENDGAME) ) {
        return 2; // we re in the endgame now
    }
    return 1;
}

// Function to evaluate the value of a queen on a given square
int checkQueen(unsigned long long bitboards[12], short int sqr, char usPlayer) {
    int totalQueenVal = Q_VALUE; // Start with the base material value of the queen

    // Evaluate queen's rook-like moves (horizontal and vertical)
    totalQueenVal += checkCross(bitboards, sqr, usPlayer);

    // Evaluate queen's bishop-like moves (diagonal)
    totalQueenVal += checkDiagonal(bitboards, sqr, usPlayer);

    return totalQueenVal;
}

// Map piece types to their respective evaluation functions 
// Used only in evaluatePiece function
EvalFunction getEvalFunction(int pieceType) {
    switch (pieceType) {
        case WHITE_PAWNS:
        case BLACK_PAWNS:
            return checkPawn; // Evaluation function for pawns

        case WHITE_KNIGHTS:
        case BLACK_KNIGHTS:
            return checkKnight; // Evaluation function for knights

        case WHITE_BISHOPS:
        case BLACK_BISHOPS:
            return checkDiagonal; // Evaluation function for bishops

        case WHITE_ROOKS:
        case BLACK_ROOKS:
            return checkCross; // Evaluation function for rooks

        case WHITE_QUEEN:
        case BLACK_QUEEN:
            return checkQueen; // Evaluation function for queens

        case WHITE_KING:
        case BLACK_KING:
            return checkKing; // Evaluation function for kings

        default:
            return NULL; // Invalid piece type
    }
}

// Evaluate the value of a piece on the board
void evaluatePiece(unsigned long long *bitboards, int pieceType, char player, int *totalPieceVal, int *player_val, int (*evalFunc)(unsigned long long *, short int, char)) {
    unsigned long long pieces = bitboards[pieceType];
    while (pieces) {
        short int sqr = __builtin_ctzll(pieces);
        *totalPieceVal += values[pieceType];
        *player_val += evalFunc(bitboards, sqr, player);
        pieces &= pieces - 1;
        //debugPrint("Piece: %d, Value: %d, Total Value: %d square: %d\n", pieceType, *player_val, *totalPieceVal,sqr);
    }
}

// calculates the value of the board based on certain criteria detailed below
int evaluateBitboard(unsigned long long bitboards[12], char usPlayer, unsigned short int fullmove) {
    int white_val = 0, black_val = 0;
    debugPrint("usPlayer: %c\n", usPlayer);
    // set the gamestate
    gameState = setGameState(fullmove, bitboards);
    debugPrint("Gamestate: %d\n", gameState);

    // change values of pieces depending 
    switch (gameState) {
        case 0: // Opening phase (emphasis on capturing the center)
            values[WHITE_PAWNS] = 3 * P_VALUE; values[ BLACK_PAWNS] = 3 * P_VALUE;
            //values[WHITE_ROOKS] = R_VALUE; values[BLACK_ROOKS] = R_VALUE;
            values[WHITE_KNIGHTS] = 2 * N_VALUE; values[BLACK_KNIGHTS] = 2 * N_VALUE;
            values[WHITE_BISHOPS] = 1.5 * B_VALUE; values[BLACK_BISHOPS] = 1.5 * B_VALUE;
            //values[WHITE_QUEEN] = Q_VALUE; values[BLACK_QUEEN] = Q_VALUE;
            if (usPlayer == 'w') { // it is more important to protect our king than to attack the enemy's in the opening stage
                values[WHITE_KING] = K_VALUE;
                values[BLACK_KING] = K_VALUE/10;
            } else {
                values[BLACK_KING] = K_VALUE;
                values[WHITE_KING] = K_VALUE/10;
            }
            // Value adjusted for center positioning
            sqrValues[0] = VAL_GR_CENTER*1.5; sqrValues[1] = VAL_GO_CENTER*1.25; sqrValues[2] = VAL_OUT_CENTER;
            break;
        case 1: // Middle game phase (default values)s
            values[WHITE_PAWNS] = P_VALUE; values[ BLACK_PAWNS] = P_VALUE;
            values[WHITE_ROOKS] = R_VALUE; values[BLACK_ROOKS] = R_VALUE;
            values[WHITE_KNIGHTS] = N_VALUE; values[BLACK_KNIGHTS] = N_VALUE;
            values[WHITE_BISHOPS] = B_VALUE; values[BLACK_BISHOPS] = B_VALUE;
            values[WHITE_QUEEN] = Q_VALUE; values[BLACK_QUEEN] = Q_VALUE;
            values[WHITE_KING] = K_VALUE; values[BLACK_KING] = K_VALUE;

            // Value adjusted for even positioning
            sqrValues[0] = VAL_GR_CENTER; sqrValues[1] = VAL_GO_CENTER; sqrValues[2] = VAL_OUT_CENTER;
            break;
        case 2: // Endgame phase
            values[WHITE_PAWNS] = 2.5 * P_VALUE; values[BLACK_PAWNS] = 2.5 * P_VALUE; // Increase pawn value for promotion potential
            values[WHITE_ROOKS] = 2 * R_VALUE; values[BLACK_ROOKS] = 2*R_VALUE; // Rooks are more valuable in open positions
            values[WHITE_KNIGHTS] = N_VALUE/2; values[BLACK_KNIGHTS] = N_VALUE/2; // Knights are less effective in open positions
            //values[WHITE_BISHOPS] = 1.5 * B_VALUE; values[BLACK_BISHOPS] = 1.5 * B_VALUE; // Bishops excel in open positions
            //values[WHITE_QUEEN] = Q_VALUE; values[BLACK_QUEEN] = Q_VALUE; // Queen remains powerful
            // if (usPlayer == 'w') {
            //     values[WHITE_KING] = 2 * K_VALUE; // Emphasize king activity
            //     values[BLACK_KING] = K_VALUE/10;
            // } else {
            //     values[BLACK_KING] = 2 * K_VALUE;
            //     values[WHITE_KING] = K_VALUE/10;
            // }
            sqrValues[0] = VAL_GO_CENTER * 0.75; sqrValues[1] = VAL_GO_CENTER * 0.75; sqrValues[2] = VAL_GO_CENTER * 0.75;
            break;
    }

    // Evaluate the value of each move on the board
    int totalPieceValW = 0, totalPieceValB = 0;
    for (int i = 0; i < 12; i++) {
        if (i < 6) { // White pieces
            evaluatePiece(bitboards, i, 'w', &totalPieceValW, &white_val, getEvalFunction(i));
        } else { // Black pieces
            evaluatePiece(bitboards, i, 'b', &totalPieceValB, &black_val, getEvalFunction(i));
        }
    }
    
    // Returning the score based on the player
    if (usPlayer == 'w') {
        debugPrint("Evaluation for White: %d\n", (POS_MULT * white_val) - (NEG_MULT * black_val));
        return ((POS_MULT * white_val) - (NEG_MULT * black_val));
    } else {
        debugPrint("Evaluation for Black: %d\n", (POS_MULT * black_val) - (NEG_MULT * white_val));
        return ((POS_MULT * black_val) - (NEG_MULT * white_val));
    }
}

// returns how many pieces a player has
int countPieces(char player, unsigned long long * bitboards) {
    int count = 0, sqr;

    if (player == 'w') {
        for (sqr = 0; sqr < 64; sqr++) {
            for (int i = WHITE_PAWNS; i <= WHITE_KING; i++) {
                count += (IS_BIT_SET(bitboards[i], sqr) != 0);
            }
        }
    }

    if (player == 'b') {
        for (sqr = 0; sqr < 64; sqr++) {
            for (int i = BLACK_PAWNS; i <= BLACK_KING; i++) {
                count += (IS_BIT_SET(bitboards[i], sqr) != 0);
            }
        }
    }

    return count;
}