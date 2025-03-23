/**
 * @file engine.c
 * @brief Chess engine implementation for choosing the best move using minimax evaluation.
 *
 * This file contains the implementation of a chess engine that evaluates and chooses the best move
 * from a given list of legal moves using the minimax algorithm. The engine reads the board position
 * in Forsyth-Edwards Notation (FEN), generates legal moves, and evaluates them to select the optimal move.
 *
 */


 int main(int argc, char * argv[]);
 #include <stdio.h>
 #include <stdlib.h>
 #include <stdarg.h>
 #include <string.h>
 
 #include "bitboard.h" 
 #include "evaluate.h" 
 #include "movegen.h"
 #include "search.h"
 #include "init.h"
 #include "tools.h"
 #include "capture.h"
 
 /*
 ./engine "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" \
 "a3 a4 b3 b4 c3 c4 d3 d4 e3 e4 f3 f4 g3 g4 h3 h4 Na3 Nc3 Nf3 Nh3" \
 3
 hello
 */
 
 /**
  * @brief Chooses the best move from a given list of legal moves using minimax evaluation.
  *
  * @param fen A string representing the board position in Forsyth-Edwards Notation (FEN).
  * @param moves A string containing all legal moves.
  * @param timeout An integer representing the maximum allowed computation time.
  * @return The index of the best move in the given list, or -1 in case of memory allocation failure.
  */
 int choose_move(char * fen, char * moves, int timeout) {
     // Save the original board.
     Board board ;
     board = malloc(sizeof(struct board));
     if (!board) {
         return -1;
     }
 
     // Initialisize board.
     memset(board,0,sizeof(struct board));
 
     Board tempBoard = NULL; // to copy the board in
     tempBoard = malloc(sizeof(struct board));
     if (!tempBoard) {
         return -1;
     }
 
     char *testMoves = moves;
     if (testMoves == NULL) {
         free(board);
         free(tempBoard);
         return 0;
     }
     
     // Initialisize temporary board (to avoid memory leaks and errors).
     memset(tempBoard,0,sizeof(struct board));
 
     // Read and create the board from the FEN string.
     parseFenRec(board, fen); 
     
     int index = 0, returnSize = 0;
 
     // Save the possible moves and the number of possible moves.
     char **choices = initMoveSave(testMoves, &returnSize); 
     if(!choices) {
         free(board);
         free(tempBoard);
         return -1;
     }
     
     if (returnSize == 1) { // no reason to evaluate, only one legal move available
         // free everything
         freeMoveSave(choices, returnSize);
         free(board);
         free(tempBoard);
         return 0; // return the index of the only legal move
     }
 
     // For every possible move, evaluate the board.
     int max = 0, flag = 1, currentVal = 0, i = 0; 
 
     for (i = 0; i < returnSize; i++) {
         // Copy the data from board to tempBoard.
         memcpy(tempBoard, board, sizeof(struct board));
 
         // Update the temp board with the possible move.
         UpdateBitboards(tempBoard, choices[i]);
        
         int depth = 2;
         if (timeout <= 1) depth = 1;
         // Get the value given by this move.
         currentVal = minimax(tempBoard, depth, -1e9, 1e9, true);
         if(DEBUG)printBoard(tempBoard);
         
         // Compare the value to max.
         if (!flag) {
             if (max < (currentVal)) {
                 max = currentVal;
                 index = i; // update max and index
             }
         } else {
             flag = 0;
             max = (currentVal);
             index = i; // update max and index
         }
         debugPrint("index: %d, max: %d, currentVal: %d\n", index, max, currentVal);
     }    
 
     // Free everything.
     freeMoveSave(choices, returnSize);
     free(board);
     free(tempBoard);
     
     return index;
 }
 
 /**
  * @brief Main function to run the chess engine.
  *
  * This function checks the user input for correctness, initializes the board, parses the FEN string,
  * reads the legal moves and timeout, and then selects and prints the best move.
  *
  * @param argc The number of command-line arguments.
  * @param argv The array of command-line arguments.
  * @return Returns 0 on success, or an error code on failure.
  */
 
 int main(int argc, char * argv[]) {
     // First and foremost checking if the user has
     // entered the correct types and numbers of parameters.
     if (argc > 4) {
         fprintf(stderr, "Only %d arguments were given.\n", argc);
         fprintf(stderr, "Usage: %s <fen> <moves> <timeout>.\n", argv[0]);
         return ERROR_CODE;
     }
 
     // Initializing a pointer Board to a struct of type board.
     Board board;
     
     // Dynamically allocating its size.
     board = malloc(sizeof(struct board));
     if (!board) {
        fprintf(stderr, "Error: memory allocation for board failure.\n");
        return ERROR_CODE;
     }
 
     // Initialisizing the board.
     memset(board,0,sizeof(struct board));
 
     // Reading the three arguments given by the user.
     // 1. Reading fen data.
     if (parseFenRec(board, argv[1]) != 0) {
         fprintf(stderr, "FEN parsing failed\n");
         free(board);
         return ERROR_CODE;
     }
     
     // 2. Reading moves.
     debugPrint("Legal moves given: %s\n", argv[2]);
     
     // 3. Reading timeout.
     int timeout = atoi(argv[3]);
     debugPrint("Timeout given: %d\n", timeout);
     
     // Then, showing current board state (debug print).
     if (DEBUG) printBoard(board);
 
     // Finally, selecting a move to play and printing it.
     int move_chosen = choose_move(argv[1], argv[2], timeout);
     if(move_chosen == -1) {
         free(board);
         return ERROR_CODE;
     }
     printf("%d\n", move_chosen);
 
     // test print to see what changed
     debugPrint("\nOutput state:\n");
     int returnSize = 0;
     char **choices = initMoveSave(argv[2], &returnSize); // save the possible moves and the number of possible moves
     if(!choices) {
         free(board);
         return ERROR_CODE;
     }
     if(DEBUG)UpdateBitboards(board, choices[move_chosen]);
     if (DEBUG) printBoard(board);
     if (DEBUG) fprintBitToFen(stdout, board);
     if (DEBUG) debugPrint("\n%s\n", choices[move_chosen]);
     freeMoveSave(choices, returnSize);
 
     free(board);
     return 0;
 }