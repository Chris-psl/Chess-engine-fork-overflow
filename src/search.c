#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
//#include <emscripten.h>

#include "search.h"
#include "bitboard.h"
#include "evaluate.h"
#include "init.h"
#include "movegen.h"
#include "tools.h"

void clearEnPassant(Board board) {
    board->pass[0] = '-';
    board->pass[1] = '\0';
}

int enemyPiece(Board board, int square) {
    for (int i = 6; i < 12; i++) {
        if (IS_BIT_SET(board->bitboards[i], square)) {
            return 1;
        }
    }
    return 0;
}

// Quiescence search function
double quiescence(Board board, double alpha, double beta) {
    // Evaluate the current position
    double stand_pat = evaluateBitboard(board->bitboards, board->toMove, board->fullmove);

    // If the current evaluation is better than beta, return beta (fail-high)
    if (stand_pat >= beta) {
        return beta;
    }

    // Update alpha if the current evaluation is better than alpha
    if (stand_pat > alpha) {
        alpha = stand_pat;
    }

        // Generate only capture moves for quiescence search
        int moveCount;

        char *legalCaptures = generateLegalCaptures(board);
        if(legalCaptures == NULL) return 0;

        char **moves = initMoveSave(legalCaptures, &moveCount);
        for (int i = 0; i < moveCount; i++) {
            
        // Calculate the destination squares
        short int dst = (moves[i][3] - '1') * 8 + (moves[i][2] - 'a');

        // Check if the move is a capture
        if (enemyPiece(board, dst) == 1) { // Not empty and not our piece
            // Create a new board state and apply the move
            Board newBoard = malloc(sizeof(struct board));
            memcpy(newBoard, board, sizeof(struct board));
            UpdateBitboards(newBoard, moves[i]);

            // Recursively evaluate the move
            double eval = -quiescence(newBoard, -beta, -alpha);
            //free(newBoard);

            // Update alpha and check for cutoffs
            if (eval >= beta) {
                //freeMoveSave(moves, moveCount);
                return beta; // Fail-high
            }
            if (eval > alpha) {
                alpha = eval;
            }
        }
    }
    //freeMoveSave(moves, moveCount);
    return alpha; // Return the best score found
}

// Minimax function with alpha-beta pruning and quiescence search
double minimax(Board board, int depth, double alpha, double beta, bool maximizingPlayer) {

    if(!board) return 0;

    char currentPlayer = board->toMove;
    int inCheck = isKingAttacked(board);
    int moveCount = 0;

    // Generate all legal moves
    char *legalMoves = generateLegalMoves(board);
    if(legalMoves == NULL) return 0;
    char **moves = initMoveSave(legalMoves, &moveCount);
    free(legalMoves);
    if(!moves) return 0;

    if (moveCount == 0) {
        freeMoveSave(moves, moveCount); // Ensure moves is freed
        return inCheck ? ((currentPlayer == 'w') ? -1e9 : 1e9) : 0;
    }

    if (depth == 0) {
        freeMoveSave(moves, moveCount); // Ensure moves is freed
        return evaluateBitboard(board->bitboards, board->toMove, board->fullmove);
        //return quiescence(board, alpha, beta);
    }

    double bestEval = maximizingPlayer ? -1e9 : 1e9;

    for (int i = 0; i < moveCount; i++) {
        Board newBoard = malloc(sizeof(struct board));
        if (!newBoard) {
            debugPrint("Memory allocation failed\n");
            freeMoveSave(moves, moveCount); // Ensure moves is freed
            return 0;
        }

        //  Initialisize bit board.
        memset(newBoard, 0, sizeof(struct board));
        memcpy(newBoard, board, sizeof(struct board)); // Copy the current board state
        
        // Apply a move
        UpdateBitboards(newBoard, moves[i]);
        newBoard->toMove = (newBoard->toMove == 'w') ? 'b' : 'w'; // Switch player
        debugPrint("tomoce: %c\n", newBoard->toMove);
        
        double eval = minimax(newBoard, depth - 1, alpha, beta, !maximizingPlayer);
        free(newBoard); // Free the allocated board

        if (maximizingPlayer) {
            bestEval = fmax(bestEval, eval);
            alpha = fmax(alpha, eval);
        } else {
            bestEval = fmin(bestEval, eval);
            beta = fmin(beta, eval);
        }

        if (beta <= alpha) break; // Prune the search tree
    }

    debugPrint("depth: %d, BestEval: %f\n", depth, bestEval);
    freeMoveSave(moves, moveCount); // Ensure moves is freed
    return bestEval;
}