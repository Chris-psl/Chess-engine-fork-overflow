/**
 * @file bitboard.c
 * @brief This file contains the implementation of bitboard initializations, operations, etc.
 */

#include <stdio.h>
#include <stdarg.h> // for va_start, va_end in debugPrint and UIPrint functions
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "init.h"
#include "bitboard.h"

/*
brief: parses a given FEN string into bitboards.

parameters: board, a pointer of type Board to a board struct, and
*fen, a pointer to a character, which represents the program's first
argument, the FEN board data.

return: 0 if successful, ERROR_CODE otherwise.
*/
int parseFenRec(Board board, char *fen) {
    // make sure every bitboard is zeroed before we start changing things around
    int i = 0, X = 7, Y = 7;
    while (fen[i] != 32) i++;
    int temp = i;
    i--;
    // Initialize bitboards
    for (int b = 0; b < 12; b++) board->bitboards[b] = 0;

    while (i >= 0) { // Stop when we run out of where to go
        if (fen[i] == '/') {
            Y--; // Move to the next rank
            X = 7;
        } else if (fen[i] >= '1' && fen[i] <= '8') {
            X -= fen[i] - '0'; // Skip empty squares
        } else {
            int square = Y * 8 + X;
            switch (fen[i]) {
                case 'P': SET_BIT(board->bitboards[WHITE_PAWNS], square); break;
                case 'p': SET_BIT(board->bitboards[BLACK_PAWNS], square); break;
                case 'R': SET_BIT(board->bitboards[WHITE_ROOKS], square); break;
                case 'r': SET_BIT(board->bitboards[BLACK_ROOKS], square); break;
                case 'N': SET_BIT(board->bitboards[WHITE_KNIGHTS], square); break;
                case 'n': SET_BIT(board->bitboards[BLACK_KNIGHTS], square); break;
                case 'B': SET_BIT(board->bitboards[WHITE_BISHOPS], square); break;
                case 'b': SET_BIT(board->bitboards[BLACK_BISHOPS], square); break;
                case 'Q': SET_BIT(board->bitboards[WHITE_QUEEN], square); break;
                case 'q': SET_BIT(board->bitboards[BLACK_QUEEN], square); break;
                case 'K': SET_BIT(board->bitboards[WHITE_KING], square); break;
                case 'k': SET_BIT(board->bitboards[BLACK_KING], square); break;
                default: 
                    //fprintf(stderr, "Invalid FEN character: %c\n", fen[i]);
                    return ERROR_CODE;
            }
            X--;
        }
        i--;
    }
    // Successfully read the first part of the fen argument
    i = temp+1;
    debugPrint("At end of board parsing: i = %d, fen[i] = '%c' (ASCII: %d)\n", i, fen[i], fen[i]);
    
    // Active player parsing
    if (fen[i] == 'b' || fen[i] == 'w') {
        board->toMove = fen[i];
        i+=2;
    } else {
        board->toMove = 'w';
        i+=2;
    }

    // Cstling availability parsing
    int castleCount = 0;
    while(1) {
        if (fen[i] != '-') {
            // Case where castling is possible
            board->castling[castleCount] = fen[i];
            castleCount++;
            board->castling[castleCount] = '\0';
            i++; // go to the next character
        } else {
            // No castling availability
            board->castling[0] = '-';
            board->castling[1] = '\0';
            i+=2; // to skip the '-' and the space
            break;
        }
        if (fen[i] == 32) {
            // End of castling section
            i++; // to skip the space
            break;
        }
    }
    // En passant availability parsing
    int passCount = 0;
        while(1) {
        if (fen[i] != '-') {
            // En passant is possible
            board->pass[passCount] = fen[i];
            passCount++;
            i++; // skip the current character
        } else {
            // En passant is impossible
            board->pass[0] = '-';
            i+=2; // to skip the '-' and the space
            break;
        }
        if (fen[i] == 32) {
            // end of en passant section
            i++; // to skip the space
            break;
        }
    }
    board->pass[passCount] = '\0';

    // Halfmove clock parsing
    char tempNum[5] = "0000"; // 4 characters to count half moves
    int numCount = 0;
    while(1) {
        if (fen[i] >= '0' && fen[i] <= '9') {
            tempNum[numCount] = fen[i];
            numCount++; 
            i++; 
        }
        if (fen[i] == 32) {
            board->halfmove = strtoul(tempNum, NULL,10);
            i++; // skip the space
            break;
        }
    }
    numCount = 0;
    tempNum[0] = '0'; tempNum[1] = '0'; tempNum[2] = '0'; tempNum[3] = '0'; tempNum[4] = '\0';

    while(1) {
        if (fen[i] >= '0' && fen[i] <= '9') {
            tempNum[numCount] = fen[i];
            numCount++; 
            i++; 
        }
        if (fen[i] == '\0') {
            board->fullmove = strtoul(tempNum, NULL ,10);
            debugPrint("Full moves: %d\n", board->fullmove);
            break;
        }
    }
    return 0;
}

/*
UI FUNCTION. NON-FINAL.
@brief: converts bitboards to a 2D array for visualization.
*/
char (*bitboardsToArray(unsigned long long *bitboards))[8] {
    static char state[8][8];
    // Initialize the board with empty squares
    for (int rank = 0; rank < 8; rank++) {
        for (int file = 0; file < 8; file++) {
            state[rank][file] = '*'; // Empty square
        }
    }

    // Iterate through each piece bitboard and set the array
    for (int piece = 0; piece < 12; piece++) {
        unsigned long long bitboard = bitboards[piece];
        while (bitboard) {
            // Find the least significant bit set (LSB)
            int square = __builtin_ctzll(bitboard);

            // Convert the square index into rank and file
            int rank = square / 8;  
            int file = square % 8;

            // Place the corresponding piece in the array
            state[7 - rank][file] = "PRNBQKprnbqk"[piece];

            // Clear the LSB
            bitboard &= bitboard - 1;
        }
    }
    return state;
}

// @brief: handles castling moves
void handleCastling(int move_size, Board board){
    if (move_size == 3) {
        // Kingside
        if (board->toMove == 'w') {
            // Starting and destination coords are standard here.
            debugPrint("Kingside castling (white)\n");

            // Move Rook
            if(IS_BIT_SET(board->bitboards[WHITE_ROOKS], 63)){
                // Move Rook
                DeletePrevious(WHITE_ROOKS, board->bitboards, 'h', '1', 'f', '1');
                updateMove(WHITE_ROOKS, board->bitboards, 'f', '1');

                // Move King
                DeletePrevious(WHITE_KING, board->bitboards, 'e', '1', 'g', '1');
                updateMove(WHITE_KING, board->bitboards, 'g', '1');
            }else {
                debugPrint("Rook not found at h1\n");
            }
        } else {
            // Starting and destination coords are standard here.
            debugPrint("Kingside castling (black)\n");
            if(IS_BIT_SET(board->bitboards[BLACK_ROOKS], 7)){   
                // Move Rook
                DeletePrevious(WHITE_ROOKS, board->bitboards, 'h', '8', 'f', '8');
                updateMove(WHITE_ROOKS, board->bitboards, 'f', '8');

                // Move King
                DeletePrevious(pieceIndex('k'), board->bitboards, 'e', '8', 'g', '8');
                updateMove(pieceIndex('k'), board->bitboards, 'g', '8');
            }else {
                debugPrint("Rook not found at h8\n");
            }
        }
        return;
    }
    // Queenside
    if (board->toMove == 'w') {
        // Starting and destination coords are standard here.

        // Move Rook
        DeletePrevious(WHITE_ROOKS, board->bitboards, 'a', '1', 'd', '1');
        updateMove(WHITE_ROOKS, board->bitboards, 'd', '1');

        // Move King
        DeletePrevious(WHITE_KING, board->bitboards, 'e', '1', 'c', '1');
        updateMove(WHITE_KING, board->bitboards, 'c', '1');
    } else {
        // Starting and destination coords are standard here.

        // Move Rook
        DeletePrevious(BLACK_ROOKS, board->bitboards, 'a', '8', 'd', '8');
        updateMove(BLACK_ROOKS, board->bitboards, 'd', '8');

        // Move King
        DeletePrevious(BLACK_KING, board->bitboards, 'e', '8', 'c', '8');
        updateMove(BLACK_KING, board->bitboards, 'c', '8');
    }
    return;
}

// @brief: handles major piece movements
void handlePieces(int move_size, Board board, const char move[MAX_MOVE_LENGTH]){
    short int i = 0;

    char piece = move[i];
    i++;

    // Convert to lowercase if black.
    if (board->toMove == 'b') {
        piece = tolower(piece);
    }

    int pieceType = pieceIndex(piece);
    if (pieceType < 0 || pieceType >= 12) {
        debugPrint("Invalid piece type: %c\n", piece);
        return;
    }

    if (move_size == 3) {
        // PLAIN MOVE
        // This is the plain, standard case (e.g. Be8), where the only thing stated other
        // than the piece is its destination.
        // Destination coords:
        // move[1] = file, move[2] = rank
        DeletePrevious(pieceIndex(piece), board->bitboards, '\0', '\0', move[1], move[2]);
        updateMove(pieceIndex(piece), board->bitboards, move[1], move[2]);
        return;
    }
    if (move_size == 4) {
        // PLAIN CAPTURE
        if (move[i] == 'x') {
            i++; // ignore capture since the presence of a piece or lack 
                // thereof is checked in evaluateBitboards itself anyways.
            DeletePrevious(pieceIndex(piece), board->bitboards, '\0', '\0', move[2], move[3]);
            updateMove(pieceIndex(piece), board->bitboards, move[2], move[3]);
            return;
        }

        if (isdigit(move[i])) {
            // OR disambiguating rank
            // move[1] = disambiguating rank, move[2] = file, move[3] = rank
            DeletePrevious(pieceIndex(piece), board->bitboards, '\0', move[1], move[2], move[3]);
            updateMove(pieceIndex(piece), board->bitboards, move[2], move[3]);
            return;
        } else {
            // OR disambiguating file
            // move[1] = disambiguating file, move[2] = file, move[3] = rank
            DeletePrevious(pieceIndex(piece), board->bitboards, move[1], '\0', move[2], move[3]);
            updateMove(pieceIndex(piece), board->bitboards, move[2], move[3]);
            return;
        }
    }
    if (move_size == 5) {
        if (isdigit(move[i])) {
            // Disambiguating rank with capture (e.g. B3xe5)
            // move[1] = disambiguating rank, move[2] = 'x' (ignore),
            // move[3] = file, move[4] = rank
            DeletePrevious(pieceIndex(piece), board->bitboards, '\0', move[1], move[3], move[4]);
            updateMove(pieceIndex(piece), board->bitboards, move[3], move[4]);
            return;
        }
        if (move[i + 1] == 'x') {
            // Disambiguating file with capture (e.g. Bcxe5)
            // move[1] = disambiguating file, move[2] = 'x' (ignore),
            // move[3] = file, move[4] = rank
            DeletePrevious(pieceIndex(piece), board->bitboards, move[1], '\0', move[3], move[4]);
            updateMove(pieceIndex(piece), board->bitboards, move[3], move[4]);
            return;
        }
        // Else, it is the disambiguating file and rank case (e.g. Bc3e5)
        // move[1] = disambiguating file, move[2] = disambiguating rank,
        // move[3] = file, move[4] = rank
        DeletePrevious(pieceIndex(piece), board->bitboards, move[1], move[2], move[3], move[4]);
        updateMove(pieceIndex(piece), board->bitboards, move[3], move[4]);
        return;
    }
    if (move_size == 6) {
        // Disambiguating file and rank with capture(e.g. Bc3xe5)
        // move[1] = disambiguating file, move[2] = disambiguating rank,
        // move[3] = 'x' (ignore), move[4] = file, move[5] = rank
        DeletePrevious(pieceIndex(piece), board->bitboards, move[1], move[2], move[4], move[5]);
        updateMove(pieceIndex(piece), board->bitboards, move[4], move[5]);
        return;
    }
}

// @brief: handles pawn movements
void handlePawnU(int move_size, Board board, const char move[MAX_MOVE_LENGTH]){

    short int i = 0;

    if (move[move_size - 1] == '+') {
        move_size--;
        if (move[move_size - 2] == '+') {
            move_size--;
        }
    }
    char piece = 'P';

    // Convert to lowercase if black.
    if (board->toMove == 'b') {
        piece = tolower(piece);
    }
    char file = move[i];
    i++;

    if (isdigit(move[i])) {
        char rank = move[i];
        if (move_size == 2) {
            // Plain move case
            //debugPrint("regular move, file: %c, rank: %c\n", file, rank);
            DeletePrevious(pieceIndex(piece), board->bitboards, '\0', '\0', file, rank);
            updateMove(pieceIndex(piece), board->bitboards, file, rank);
            return;
        }
        // Promotion case
        // Skip '=' character indicating capture.
        i+=2;

        //debugPrint("Promotion move\n");
        DeletePrevious(pieceIndex(piece), board->bitboards, file, '\0', file, rank);
        debugPrint("Promotion to piece: %c\n", move[i]);
        updateMove(pieceIndex(move[i]), board->bitboards, file, rank);
        return;
    } else {
        // Capture case or en passant case
        // Skip 'x' character indicating capture.
        i++;
        char file_target = move[i];
        i++;
        char rank_target = move[i];
        
        if ((board->pass)[0] != '-' && (board->pass)[0] != '\0') {
            // Checking if the target square
            // is the same as the board->pass string.
            char target_square[3];
            target_square[0] = file_target;
            target_square[1] = rank_target;
            target_square[2] = '\0';
            if (strcmp(target_square, board->pass) == 0) {
                // En passant case
                debugPrint("En passant!!!!!!!!! target square: %s\n", target_square);
                DeletePrevious(pieceIndex(piece), board->bitboards, file, '\0', file_target, rank_target);
                updateMove(pieceIndex(piece), board->bitboards, file_target, rank_target);
                return;
            }
            // Remove en passant availability.
            board->pass[0] = '-';
            board->pass[1] = '\0';
        }

        // If that's not the case (no en passant availability
        // or no en passant played), then the pawn's move is a plain capture.
        debugPrint("Plain capture move\n");
        DeletePrevious(pieceIndex(piece), board->bitboards, file, '\0', file_target, rank_target);
        updateMove(pieceIndex(piece), board->bitboards, file_target, rank_target);
        return;
    }
}

/*
@brief: parses a move given in standard algebraic notation and updates bitboards based on it.
*/
void UpdateBitboards(Board board, char *move) {    
    // Calculate move size (excluding null byte).
    int move_size = strlen(move);

    if (move[move_size - 1] == '+') {
        move_size--;
        if (move[move_size - 2] == '+') {
            move_size--;
        }
    }

    // Castling.
    // 0-0 (or O-O) OR 0-0-0 (or O-O-O) for Kingside and Queenside accordingly.
    if (move[0] == '0' || move[0] == 'O') {
        handleCastling(move_size, board);
    } 
    // Non-pawn piece.
    else if (move[0] == 'R' || move[0] == 'N' || move[0] == 'B' || move[0] == 'Q' || move[0] == 'K') {
        handlePieces(move_size, board, move);
    }
    // Pawn.
    else {
        handlePawnU(move_size, board, move);
    }


    // The pawn is moving.
    /*
    PAWN
    plain: d4
    capture: exd5   When a pawn makes a capture, the file from which the pawn departed 
                    is used to identify the pawn. 
                    For example, exd5 (pawn on the e-file captures the piece on d5).
    en passant: exd6   En passant captures are indicated by specifying the capturing pawn's 
                    file of departure, the "x" and the destination square (not the square 
                    of the captured pawn).
    promotion: e8=Q    A pawn on e7 promoting to a queen on e8
    */
}

// @brief: deletes the previous position of a piece
void emptySquare(unsigned long long *bitboards, char file, char rank) {
    int sqr  = 56 + (file - 'a') - ((rank - '1')* 8);
    for (int i = 0; i <= BLACK_KING; i++) CLEAR_BIT(bitboards[i], sqr);
}

/*
@brief: changes the destination bit/square/spot of the given piece's bitboard
from 0 to 1 to show it has been moved there.
*/
void updateMove(int piece, unsigned long long *bitboards, char file, char rank) {
    emptySquare(bitboards, file, rank);
    int sqr  = 56 + (file - 'a') - ((rank - '1')* 8);
    SET_BIT(bitboards[piece], sqr);
}

// @brief: prints a given piece's bitboard (which 
// is essentially just a binary number) in stdout.
void printBitboard(unsigned long long * bitboards, int piece) {
    unsigned long long n = bitboards[piece];
    while (n) {
        if (n & 1)
            printf("1");
        else
            printf("0");

        n >>= 1;
    }
    printf("\n");
}

/*
@brief: reads a bitboard, translates it into fen data, and prints 
the fen to a customizable stream (can either be stdout or another filename).

@parameters: *stream, a file pointer to a stream, and board, a pointer of type 
Board to a board struct.
*/
void fprintBitToFen(FILE *stream, Board board) {
    // Create a buffer to save and, eventually, print the fen board data.
    // It holds exactly 72 because 64 positions maximum, 7 slash characters, null byte.
    char fen[72]; 
    
    int i = 0; // to traverse fen in order to save things in it
    int concGaps = 0; // to count back to back gaps

    for (int num = 0; num < 8; num++) {
        for (int SQR = num * 8; SQR < (num * 8) + 8; SQR++) {
            int found = 0;
            for (int PIECE = WHITE_PAWNS; PIECE <= BLACK_KING; PIECE++) {
                if (IS_BIT_SET(board->bitboards[PIECE], SQR)) {
                    found = 1;
                    // found a piece so if there were gaps before we need to write that down
                    if (concGaps > 0) { 
                        fen[i] = concGaps + '0';
                        concGaps = 0;
                        i++;
                    }
                    // TODO set the position fen[i] equal to the piece found here
                    switch (PIECE) {
                        case WHITE_PAWNS: fen[i] = 'P'; break;
                        case WHITE_ROOKS: fen[i] = 'R'; break;
                        case WHITE_KNIGHTS: fen[i] = 'N'; break;
                        case WHITE_BISHOPS: fen[i] = 'B'; break;
                        case WHITE_QUEEN: fen[i] = 'Q'; break;
                        case WHITE_KING: fen[i] = 'K'; break;
                        case BLACK_PAWNS: fen[i] = 'p'; break;
                        case BLACK_ROOKS: fen[i] = 'r'; break;
                        case BLACK_KNIGHTS: fen[i] = 'n'; break;
                        case BLACK_BISHOPS: fen[i] = 'b'; break;
                        case BLACK_QUEEN: fen[i] = 'q'; break;
                        case BLACK_KING: fen[i] = 'k'; break;
                    }
                    i++;
                    break; // go to the next SQR
                }
            }
            if (!found) {
                // if the if didn't break it means there is a gap cause it reached black king
                // without finding anything
                concGaps++;
            }
        }
        // set the current position to slash since a line just ended
        if (concGaps != 0) {
            fen[i] = concGaps + '0';
            concGaps = 0;
            i++;
        }
        if (num != 7) { // in every loop but the last one add a / at the end
            fen[i] = '/';
            i++;
        }
    }
    fen[i] = '\0';
    fprintf(stream, "%s\n", fen);
}

// @brief: creates a 2D representation of the board and prints it to stdout.
void printBoard(Board board) {
    char (*state)[8] = bitboardsToArray(board->bitboards);
    printf("---------------\n");
    for (int i = 7; i >= 0; i--) {
        for (int j = 0; j < 8; j++) {
            printf("%c ", state[i][j]);
        }
        printf("\n");
    }
    printf("---------------\n");
}

// @brief: 
void handleBQ(int piece, unsigned long long *bitboards, char sFile, char sRank, char dFile, char dRank){
    int dSquare =  56 + (dFile - 'a') - ((dRank - '1')* 8), sSquare = -1;
    if (piece == WHITE_BISHOPS ||piece == BLACK_BISHOPS || piece == WHITE_QUEEN || piece == BLACK_QUEEN) {
        
        sSquare = dSquare;
        while(1) { // direction up + right
            if (((sSquare % 8 + 'a') == 'h') || (sSquare - 8 < 0)) break;   // check that the limit hasn't been surpassed
            sSquare += - 8 + 1;
            // check if you can find the correct piece
            if (IS_BIT_SET(bitboards[piece], sSquare) != 0 && (possiblePiece(bitboards, sFile, sRank, sSquare, piece) == 1)) return;
            if (whatPieceBit(bitboards, sSquare) != -1) break; // found a different piece, impossible for ours to be here
        }

        sSquare = dSquare;
        while(1) { // direction down + right
            if (((sSquare % 8 + 'a') == 'h') || (sSquare + 8 < 0)) break;   // check that the limit hasn't been surpassed    
            sSquare += + 8 + 1;
            // check if you can find the correct piece
            if (IS_BIT_SET(bitboards[piece], sSquare) != 0 && (possiblePiece(bitboards, sFile, sRank, sSquare, piece) == 1)) return;
            if (whatPieceBit(bitboards, sSquare) != -1) break; // found a different piece, impossible for ours to be here      
        }

        sSquare = dSquare;
        while(1) { // direction up + left
            if (((sSquare % 8 + 'a') == 'a') || (sSquare - 8 < 0))  break;  // check that the limit hasn't been surpassed
            sSquare += - 8 - 1;
            // check if you can find the correct piece
            if (IS_BIT_SET(bitboards[piece], sSquare) != 0 && (possiblePiece(bitboards, sFile, sRank, sSquare, piece) == 1)) return;
            if (whatPieceBit(bitboards, sSquare) != -1) break; // found a different piece, impossible for ours to be here
        }

        sSquare = dSquare;
        while(1) { // direction down + left
            if (((sSquare % 8 + 'a') == 'a') || (sSquare + 8 > 63)) break;   // check that the limit hasn't been surpassed    
            sSquare += + 8 - 1;
            // check if you can find the correct piece
            if (IS_BIT_SET(bitboards[piece], sSquare) != 0 && (possiblePiece(bitboards, sFile, sRank, sSquare, piece) == 1)) return;
            if (whatPieceBit(bitboards, sSquare) != -1) break; // found a different piece, impossible for ours to be here
        }

        // if nothing was found, return but its an error
        debugPrint("Couldn't find %d piece position\n", piece);
        return;
    }
}


void handleRQ(int piece, unsigned long long *bitboards, char sFile, char sRank, char dFile, char dRank){
    int dSquare =  56 + (dFile - 'a') - ((dRank - '1')* 8), sSquare = -1;
    if (piece == WHITE_ROOKS || piece == BLACK_ROOKS || piece == WHITE_QUEEN || piece == BLACK_QUEEN) {
        
        sSquare = dSquare;
        while(1) { // direction down
            sSquare += 8;
            if (sSquare > 63) break;
            // check that the limit hasn't been surpassed and check if you can find the correct piece
            if (IS_BIT_SET(bitboards[piece], sSquare) != 0 && (possiblePiece(bitboards, sFile, sRank, sSquare, piece) == 1)) return;
            if (whatPieceBit(bitboards, sSquare) != -1) break; // found a different piece, impossible for ours to be here
        }

        sSquare = dSquare;
        while(1) { // direction up
            sSquare += -8;
            if (sSquare < 0) break;
            // check that the limit hasn't been surpassed and check if you can find the correct piece
            if (IS_BIT_SET(bitboards[piece], sSquare) != 0 && (possiblePiece(bitboards, sFile, sRank, sSquare, piece) == 1)) return;
            if (whatPieceBit(bitboards, sSquare) != -1) break; // found a different piece, impossible for ours to be here      
        }

        sSquare = dSquare;
        while(1) { // direction right
        // check that the limit hasn't been surpassed
            if ((sSquare % 8 + 'a') == 'h') break;
            sSquare += 1;
            // check if you can find the correct piece
            if (IS_BIT_SET(bitboards[piece], sSquare) != 0 && (possiblePiece(bitboards, sFile, sRank, sSquare, piece) == 1)) return;
            if (whatPieceBit(bitboards, sSquare) != -1) break; // found a different piece, impossible for ours to be here
        }

        sSquare = dSquare;
        while(1) { // direction left
        // check that the limit hasn't been surpassed
            if ((sSquare % 8 + 'a') == 'a') break;
            sSquare -= 1;
            // check if you can find the correct piece
            if (IS_BIT_SET(bitboards[piece], sSquare) != 0 && (possiblePiece(bitboards, sFile, sRank, sSquare, piece) == 1)) return;
            if (whatPieceBit(bitboards, sSquare) != -1) {
                debugPrint("NTOE NOTE NTON\n");
                break; // found a different piece, impossible for ours to be here
            }
        }

        // if its a rook and nothing was found, return but its an error
        if (piece == WHITE_ROOKS || piece == BLACK_ROOKS) {
            debugPrint("Couldn't find %d piece position\n", piece);
            return;
        }
    }
}


void handlePawns(int piece, unsigned long long *bitboards, char sFile, char sRank, char dFile, char dRank){
    //debugPrint("handling pawn move: %c%c to %c%c\n", sFile, sRank, dFile, dRank);
    int dSquare =  56 + (dFile - 'a') - ((dRank - '1')* 8), sSquare = -1;
    //debugPrint("dSquare: %d\n", dSquare);

    if (piece == WHITE_PAWNS) { // white pawns have 4 possible previous locations
        // if on rank 4 only, check the spot that is two squares behind you
          sSquare = dSquare + 16;
          if ((dRank == '4') && (IS_BIT_SET(bitboards[WHITE_PAWNS], sSquare))) {
              // it could be here so check sFile and sRank
              if (possiblePiece(bitboards, sFile, sRank, sSquare, piece) == 1) return;
          }
  
          // on any rank check the square exactly behind you
          sSquare = dSquare + 8;
          if ((IS_BIT_SET(bitboards[WHITE_PAWNS], sSquare)) && possiblePiece(bitboards, sFile, sRank, sSquare, piece) == 1) return;
  
          // if not on file a, check the behind and left position
          if (sFile != 'a') {
              sSquare = dSquare + 8 - 1;
              if ((IS_BIT_SET(bitboards[WHITE_PAWNS], sSquare)) && possiblePiece(bitboards, sFile, sRank, sSquare, piece) == 1) return;
          }
  
          // if not on file h, check the behind and right position
          if (sFile != 'h') {
              sSquare = dSquare + 8 + 1;
              if ((IS_BIT_SET(bitboards[WHITE_PAWNS], sSquare)) && possiblePiece(bitboards, sFile, sRank, sSquare, piece) == 1) return;
          }
          
          debugPrint("FAILED TO FIND POSSIBLE WHITE PAWN POSITION\n");
          return;
      }
      
      if (piece == BLACK_PAWNS) { // black pawns have 4 possible previous locations
          // if on rank 5 only, check the spot that is two squares behind you
          sSquare = dSquare - 16;
          if ((dRank == '5') && (IS_BIT_SET(bitboards[BLACK_PAWNS], sSquare))) {
              // it could be here so check sFile and sRank
              if (possiblePiece(bitboards, sFile, sRank, sSquare, piece) == 1) return;
          }
  
          // on any rank check the square exactly begind you
          sSquare = dSquare - 8;
          if ((IS_BIT_SET(bitboards[BLACK_PAWNS], sSquare)) && possiblePiece(bitboards, sFile, sRank, sSquare, piece) == 1) return;
  
          // if not on file a, check the behind and left position
          if (sFile != 'a') {
              sSquare = dSquare - 8 - 1;
              if ((IS_BIT_SET(bitboards[BLACK_PAWNS], sSquare)) && possiblePiece(bitboards, sFile, sRank, sSquare, piece) == 1) return;
          }
  
          // if not on file h, check the behind and right position
          if (sFile != 'h') {
              sSquare = dSquare - 8 + 1;
              if ((IS_BIT_SET(bitboards[BLACK_PAWNS], sSquare)) && possiblePiece(bitboards, sFile, sRank, sSquare, piece) == 1) return;
          }
          
          debugPrint("FAILED TO FIND POSSIBLE BLACK PAWN POSITION sSquare: %d dSquare %d\n", sSquare, dSquare);
          return;
      }
}

void handleKnights(int piece, unsigned long long *bitboards, char sFile, char sRank, char dFile, char dRank){
    int dSquare =  56 + (dFile - 'a') - ((dRank - '1')* 8), sSquare = -1;
    sSquare = dSquare;
    if (piece == BLACK_KNIGHTS || piece == WHITE_KNIGHTS) {
        short int directions[8] = {1,1,1,1,1,1,1,1};
            // bool for if its possible to go to a square or not
            // 0 = down left, 1 = down right
            // 2 = right down, 3 = right up
            // 4 = up right, 5 = up left
            // 6 = left up, 7 = left down
        if (sSquare % 8 == 0) { // knight on file A 
            directions[0] = 0; directions[5] = 0; directions[6] = 0; directions[7] = 0;
        }
        if (sSquare % 8 == 1) { // knight on file B
            directions[6] = 0; directions[7] = 0;
        }
        if (sSquare % 8 == 6) { // knight on file G
            directions[2] = 0; directions[3] = 0;
        }
        if (sSquare % 8 == 7) { // knight on file H
            directions[1] = 0; directions[2] = 0; directions[3] = 0; directions[4] = 0;
        }
        if (sSquare >= 0 && sSquare <= 7) { // knight on rank 8 
            directions[6] = 0; directions[5] = 0; directions[4] = 0; directions[3] = 0; 
        }
        if (sSquare >= 8 && sSquare <= 15) { // knight on ranks 7
            directions[5] = 0; directions[4] = 0;
        }
        if (sSquare >= 56 && sSquare <= 63) { // knight on ranks 1
            directions[7] = 0; directions[0] = 0; directions[1] = 0; directions[2] = 0; 
        }
        if (sSquare >= 48 && sSquare <= 55) { // knight on rank 2
            directions[0] = 0; directions[1] = 0;
        }

        // check every valid direction of knights, if there is one, it gets cleared and we return
        sSquare = dSquare +16 -1;
        if (directions[0] && IS_BIT_SET(bitboards[piece], sSquare) != 0 && (possiblePiece(bitboards, sFile, sRank, sSquare, piece) == 1)) return;

        sSquare = dSquare +16 +1;
        if (directions[1] && IS_BIT_SET(bitboards[piece], sSquare) != 0 && (possiblePiece(bitboards, sFile, sRank, sSquare, piece) == 1)) return;
     
        sSquare = dSquare +8 + 2;
        if (directions[2] && IS_BIT_SET(bitboards[piece], sSquare) != 0 && (possiblePiece(bitboards, sFile, sRank, sSquare, piece) == 1)) return;

        sSquare = dSquare -8 +2;
        if (directions[3] && IS_BIT_SET(bitboards[piece], sSquare) != 0 && (possiblePiece(bitboards, sFile, sRank, sSquare, piece) == 1)) return;
    
        sSquare = dSquare -16 +1;
        if (directions[4] && IS_BIT_SET(bitboards[piece], sSquare) != 0 && (possiblePiece(bitboards, sFile, sRank, sSquare, piece) == 1)) return;

        sSquare = dSquare -16 -1;
        if (directions[5] && IS_BIT_SET(bitboards[piece], sSquare) != 0 && (possiblePiece(bitboards, sFile, sRank, sSquare, piece) == 1)) return;

        sSquare = dSquare -8 -2;
        if (directions[6] && IS_BIT_SET(bitboards[piece], sSquare) != 0 && (possiblePiece(bitboards, sFile, sRank, sSquare, piece) == 1)) return;

        sSquare = dSquare +8 -2;
        if (directions[7] && IS_BIT_SET(bitboards[piece], sSquare) != 0 && (possiblePiece(bitboards, sFile, sRank, sSquare, piece) == 1)) return;

       debugPrint("Couldn't find %d piece position\n", piece);
       return; 
    }
}

void handleKings(int piece, unsigned long long *bitboards, char sFile, char sRank, char dFile, char dRank){
    if (piece == WHITE_KING || piece == BLACK_KING) {
        int dSquare =  56 + (dFile - 'a') - ((dRank - '1')* 8), sSquare = -1;
        short int directions[8] = {1,1,1,1,1,1,1,1};
        // each number in directions stands for if its possible to go in that direction or not
        // 0 = down left, 1 = down, 2 = down right
        // 3 = right 4= up right
        // 5 = up, 6 = up left, 7 = left
        sSquare = dSquare;
        if (sSquare % 8 == 0) { // if on file A we cant go left
            directions[6] = 0; directions[7] = 0; directions[0] = 0;
        }
        if (sSquare % 8 == 7) { // if on file H we cant go right
            directions[2] = 0; directions[3] = 0; directions[4] = 0;
        }
        if (sSquare >= 56 && sSquare <= 63) { // if on rank 1 cant go down
            directions[0] = 0; directions[1] = 0; directions[2] = 0;
        }
        if (sSquare >= 0 && sSquare <= 7) { // if on rank 8 we cant go up
            directions[4] = 0; directions[5] = 0; directions[6] = 0;
        }

        // check every valid direction of knights, if there is one, it gets cleared and we return
        sSquare = dSquare +8 -1;
        if (directions[0] && IS_BIT_SET(bitboards[piece], sSquare) != 0 && (possiblePiece(bitboards, sFile, sRank, sSquare, piece) == 1)) return;
        
        sSquare = dSquare +8;
        if (directions[1] && IS_BIT_SET(bitboards[piece], sSquare) != 0 && (possiblePiece(bitboards, sFile, sRank, sSquare, piece) == 1)) return;

        sSquare = dSquare +8 +1;
        if (directions[2] && IS_BIT_SET(bitboards[piece], sSquare) != 0 && (possiblePiece(bitboards, sFile, sRank, sSquare, piece) == 1)) return;

        sSquare = dSquare +1;
        if (directions[3] && IS_BIT_SET(bitboards[piece], sSquare) != 0 && (possiblePiece(bitboards, sFile, sRank, sSquare, piece) == 1)) return;

        sSquare = dSquare -8 +1;
        if (directions[4] && IS_BIT_SET(bitboards[piece], sSquare) != 0 && (possiblePiece(bitboards, sFile, sRank, sSquare, piece) == 1)) return;

        sSquare = dSquare -8;
        if (directions[5] && IS_BIT_SET(bitboards[piece], sSquare) != 0 && (possiblePiece(bitboards, sFile, sRank, sSquare, piece) == 1)) return;

        sSquare = dSquare -8 -1;
        if (directions[6] && IS_BIT_SET(bitboards[piece], sSquare) != 0 && (possiblePiece(bitboards, sFile, sRank, sSquare, piece) == 1)) return;

        sSquare = dSquare -1;
        if (directions[7] && IS_BIT_SET(bitboards[piece], sSquare) != 0 && (possiblePiece(bitboards, sFile, sRank, sSquare, piece) == 1)) return;

        
        // if nothing was found, return but its an error
        debugPrint("Couldn't find %d piece position\n", piece);
        return;
    }
}

void DeletePrevious(int piece, unsigned long long *bitboards, char sFile, char sRank, char dFile, char dRank) {

    // if neither starting coords are null, we can just reset the bit in that particular square.
    if (sFile != '\0' && sRank != '\0') {
        int sqr  = 56 + (sFile - 'a') - ((sRank - '1')* 8);
        CLEAR_BIT(bitboards[piece], sqr);
        return;
    }

    // other cases are more complicated
    // we have to check all possible starting positions based on the destination coords
    // and then check if they make sense given the particular file or rank we have 
    // if we have neither we know for a fact there can be only one possible move so we stop at the first one we find

    //pawns
    handlePawns(piece, bitboards, sFile, sRank, dFile, dRank);

    // rooks and queens can go in four directions. Search the directions starting from the dSquare 
    // until you find a knight or queen, then check if its a valid position with possiblePiece()
    handleRQ(piece, bitboards, sFile, sRank, dFile, dRank);

    //knights
    handleKnights(piece, bitboards, sFile, sRank, dFile, dRank);

    // bishops and queens can go in four directions. Search the directions starting from the dSquare 
    // until you find a knight or queen, then check if its a valid position with possiblePiece()
    handleBQ(piece, bitboards, sFile, sRank, dFile, dRank);

    //kings
    handleKings(piece, bitboards, sFile, sRank, dFile, dRank);

}

// checks if there is a pawn in position sSquare, that matches the criteria set by sFile and sRank
// if sFile and sRank are both null it just checks if there is a pawn there
// if there is it gets deleted 
int possiblePiece(unsigned long long *bitboards, char sFile, char sRank, int sSquare, int piece) {
    if (((sRank != '\0') && (sRank == (8 - ((sSquare) / 8)) + '0'))) {
        CLEAR_BIT(bitboards[piece], sSquare);
        return 1;
    } else if  ((sFile != '\0') && (sFile == ('a' + ((sSquare) % 8)))) {
        CLEAR_BIT(bitboards[piece], sSquare);
        return 1;
    } else if ((sRank == '\0') && (sFile == '\0')) {
        CLEAR_BIT(bitboards[piece], sSquare);
        return 1;
    }
    return 0;
}

// checks if a square is empty of pieces
// returns pawn number if there is a pawn or returns -1 if its empty
int whatPieceBit(unsigned long long bitboards[12], int sqr) {
    for (int piece = 0; piece < 12; piece++) {
        if (IS_BIT_SET(bitboards[piece], sqr)) return piece;
    }
    return -1;
}

// gets a piece value in char and returns the index based on the enum
int pieceIndex(char p) {
    switch (p)
    {
    case 'P': return WHITE_PAWNS;
    case 'R': return WHITE_ROOKS;
    case 'N': return WHITE_KNIGHTS;
    case 'B': return WHITE_BISHOPS;
    case 'Q': return WHITE_QUEEN;
    case 'K': return WHITE_KING;
    case 'p': return BLACK_PAWNS;
    case 'r': return BLACK_ROOKS;
    case 'n': return BLACK_KNIGHTS;
    case 'b': return BLACK_BISHOPS;
    case 'q': return BLACK_QUEEN;
    case 'k': return BLACK_KING;
    }

    // if the char is not a valid piece
    return -1;
}