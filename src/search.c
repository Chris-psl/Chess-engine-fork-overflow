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
    int start = (board->toMove == 'w') ? 6 : 0;
    int end = start + 6;
    
    for (int i = start; i < end; i++) {
        if (IS_BIT_SET(board->bitboards[i], square)) {
            return 1;
        }
    }
    return 0;
}

// Quiescence search function
double quiescence(Board board, double alpha, double beta) {
    // Evaluate the current position
    double stand_pat = evaluateBitboard(board);

    // Fail-high beta cutoff
    if (stand_pat >= beta) {
        return beta;
    }

    // Update alpha if stand_pat is better
    if (stand_pat > alpha) {
        alpha = stand_pat;
    }

    // Generate capture moves only
    int moveCount;
    char *legalCaptures = generateLegalCaptures(board);
    if (legalCaptures == NULL) return alpha; // Return alpha instead of 0

    char **moves = initMoveSave(legalCaptures, &moveCount);
    free(legalCaptures); // Free the string containing moves
    if (moves == NULL) return alpha;

    for (int i = 0; i < moveCount; i++) {
        short int dst = (moves[i][3] - '1') * 8 + (moves[i][2] - 'a');

        // Check if move is a valid capture (including en passant)
        if (enemyPiece(board, dst)) {
            // Create new board state
            Board newBoard = malloc(sizeof(struct board));
            if (!newBoard) {
                freeMoveSave(moves, moveCount);
                return alpha;
            }

            memcpy(newBoard, board, sizeof(struct board));
            UpdateBitboards(newBoard, moves[i]);

            // Recursive quiescence search
            double eval = -quiescence(newBoard, -beta, -alpha);
            free(newBoard); // Free allocated board

            // Fail-hard beta cutoff
            if (eval >= beta) {
                freeMoveSave(moves, moveCount);
                return beta;
            }

            if (eval > alpha) {
                alpha = eval;
            }
        }
    }

    freeMoveSave(moves, moveCount);
    return alpha;
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
        //return evaluateBitboard(board);
        return quiescence(board, alpha, beta);
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