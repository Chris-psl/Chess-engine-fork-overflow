#ifndef TOOLS
#define TOOLS

// Basic tools have forgotten what they do, so I will just include them here.
void freeMoveSave(char **moveSave, int count);
char **initMoveSave(const char *moves, int *returnSize);
void printBinary(unsigned long long num);

#endif
