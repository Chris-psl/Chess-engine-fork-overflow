#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tools.h"
#include "init.h"
#include "bitboard.h"


// @brief: dynamically frees all of the possible given moves.
void freeMoveSave(char **moveSave, int count) {
    if (!moveSave) return;
    for (int i = 0; i < count; i++) {
        free(moveSave[i]);
    }
    free(moveSave);
}

// @brief: dynamically saves all of the possible given moves.
char** initMoveSave(const char *moves, int *returnSize) {
    if (moves[0] == '\0') {
        *returnSize = 0;
        return NULL;
    }
    int j = 0, i = 0, k = 0;
    if (moves[0] == ' ') j = 1;
    int resize = 4;
    char **moveSave = malloc(resize * sizeof(char *));
    if (!moveSave) return NULL;

    moveSave[0] = malloc((MAX_MOVE_LENGTH + 1) * sizeof(char));
    if (!moveSave[0]) {
        free(moveSave);
        return NULL;
    }

    while (moves[j]) {
        if (i == resize - 1) {
            resize *= 2;
            char **temp = realloc(moveSave, resize * sizeof(char *));
            if (!temp) {
                freeMoveSave(moveSave, i);
                return NULL;
            }
            moveSave = temp;
        }

        if (moves[j] == ' ') {

            moveSave[i][k] = '\0';
            j++;
            i++;           
            k = 0;
            

            moveSave[i] = malloc((MAX_MOVE_LENGTH + 1) * sizeof(char));
            if (!moveSave[i]) {
                freeMoveSave(moveSave, i);
                return NULL;
            }
        } else if (moves[j] != '\0') {
            moveSave[i][k] = moves[j];
            j++;
            k++;
        }
    }
    moveSave[i][k] = '\0';

    *returnSize = i + 1;
    return moveSave;
}

// @brief: prints binary from unsigned long long (debug function)
void printBinary(unsigned long long num) {
    for (int i = 63; i >= 0; i--) {  // 64-bit representation
        printf("%lld", (num >> i) & 1);
        if (i % 8 == 0) {
            printf("\n");
        }
    }
    printf("\n");
}
