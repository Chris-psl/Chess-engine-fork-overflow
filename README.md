# Chess Engine - Fork Overflow Team

## Table of Contents

- [About the project](#about-the-program)
  - [Features](#features)
  - [Performance benchmark](#performance-benchmark)
- [How it works](#how-it-works)
  - [Repository structure](#repository-structure)
  - [Libraries (header files) used](#libraries-header-files-used)
  - [Compiling with Makefile](#compiling-with-makefile)
- [Main Code](#main-code)
  - [engine.c](#engine.c)
  - [bitboard.c](#bitboard.c)
  - [capture.c](#capture.c)
  - [evaluate.c](#evaluate.c)
  - [init.c](#init.c)
  - [movegen.c](#movegen.c)
  - [search.c](#search.c)
  - [tools.c](#tools.c)
- [Usage](#usage)
  - [Compilation and execution](#compilation-and-execution)
  - [Command line arguments](#command-line-arguments)
  - [Input constraints](#input-constraints)
  - [Demo](#demo)
- [What we learned](#what-we-learned)
- [Contact](#contact)
- [TODO](#todo)

## About the project
This project contains the solution to **Homework #3** for the course "Intro to Programming", part of the first-semester undergraduate studies in Informatics and Telecommunications at the National and Kapodistrian University of Athens.
In short, it is a chess engine written in C which can evaluate board positions and make optimal moves
under time constraints. It significantly surpasses a random-move opponent.

### Features
- Implements core chess engine mechanics, including legal move generation and state transitions.
- Stores board data efficiently in bitboards.
- Supports the **Forsyth-Edwards Notation (FEN)** for board representation[^fen].
- Uses **Minimax** to optimize move selection[^minimax].
- Uses **Quiescence Search** to avoid the horizon effect[^horizon].
- Provides two interfaces:
  1. **Command-line interface**: Accepts board state, possible moves, and a time limit as inputs.
  2. **Function call interface**: Designed for WebAssembly compatibility. WebAssembly allows our chess engine to run efficiently in your browser, enabling real-time board evaluation without needing a backend server. In Web Assembly, the UI is user-friendly and allows one to witness a proper chess match between the engine and the random move generator, the engine itself, or even the user.

### Performance benchmark
The engine:  
  
  :white_check_mark: compiles with no warnings,  
  :white_check_mark: successfully selects moves within the given time constraint,  
  :white_check_mark: wins **more than 60%** of games against a random move generator engine,    
  :white_check_mark: has a compiled executable whose size is **smaller than 1MB**,  
  :white_check_mark: runs efficiently within the allocated time limit (timeout) given by the user,  
  :white_check_mark: calculates the best move's index from the provided list of legal moves,  

therefore ensuring compliance with the given assignment constraints[^1].

## How it works

### Repository structure 
```
/ (Project Root)
│── src/                     # Source code directory
│   ├── engine.c             # Main code file
│   ├── move_generation.c    # Legal move generation file
│   ├── evaluation.c         # Board position evaluation file
│   ├── search.c             # Depth search file
│   ├── tools.c              # Helper functions file
│   ├── bitboard.c           # Bitboard creating and processing file
│   ├── init.c               # Value initialization file
│   ├── capture.c            # Capture handling file
│   ├── Makefile             # Compilation automation script
│── AUTHORS                  # Information of the two team members
│── README.md                # Project writeup (this file)
```


### Libraries (header files) used
1. >**stdio.h**  
for basic functions such as ```printf``` (used carefully since it is incompatible with WebAssembly)
2. >**stdlib.h**  
for ```malloc```, ```realloc``` (memory managment), ```strtoul``` (string to unsigned long integer conversions), e.t.c.
3. >**stdarg.h**  
for custom variadic debugging functions that work similarly to printf[^inspiration]
4. >**string.h**  
for string manipulation
5. >**ctype.h**  
for the function ```isdigit(int c)```, which checks whether the passed character is decimal digit

### Compiling with Makefile
We utilized `make`, a command-line interface software tool that automates compilation by defining dependencies between source files, ensuring efficient and error-free builds. It is commonly used for **build automation to build executable code** (such as a program or library) from source code[^last-1]. 

In our case, `make` served the purpose of compiling the multiple code files of the engine at once through automatically filling a `build` directory with the executable binary files of all `C` files, as written here:

```make
## Where to put the object files
BINDIR ?= build
```

The given `Makefile` contains the names of all those `C` files as well as the name of the directory under which all the code was written, which we added in this section:

```make
## Where our implementation is located (don't change)
SRCDIR = src
## List all files you want to be part of your engine below
## (Okay to add more files)
SOURCES = \
  $(SRCDIR)/engine.c \
  $(SRCDIR)/bitboard.c \
  $(SRCDIR)/evaluate.c \
  $(SRCDIR)/init.c \
  $(SRCDIR)/tools.c \
  $(SRCDIR)/movegen.c \
  $(SRCDIR)/capture.c \
  $(SRCDIR)/search.c
```

After this extract, we can see the compiler being used (`gcc`), as well as a list of unmodifiable given compilation flags:

```make
## Compiler to use by default
CC = gcc

## Compiler flags
CFLAGS = -Wall -Wextra -Werror -pedantic
```

## Main Code

### **engine.c**
This is the main program of the project. It works in the following way:

1. It checks that exactly 4 parameters were given by the user, as explained in the
compilation and execution section of this writeup.
```c
// First and foremost checking if the user has
// entered the correct types and numbers of parameters.
if (argc > 4) {
    fprintf(stderr, "Only %d arguments were given.\n", argc);
    fprintf(stderr, "Usage: %s <fen> <moves> <timeout>.\n", argv[0]);
    return ERROR_CODE;
}
```

2. This is the initialization section. The board state handling is made possible by using a struct
called board (but it is initialized with a pointer Board to this struct in init.h, so that's why here
we create a pointer board of type Board). The pointer to the struct is created and then dynamically
allocated using malloc (Note: a previous implementation included the mistake
`board = malloc(sizeof(Board));` instead of the current memory allocation command, which gave us the
size of a pointer (Board *), not the size of the board struct itself - this caused peculiar issues in
memory allocation). After that, it is initialized using the memset function:
- ```console
  $ man memset
  ...
  DESCRIPTION
         The memset() function fills the first n bytes of the memory area pointed to by s with the constant byte c.
  
  RETURN VALUE
         The memset() function returns a pointer to the memory area s.
  ...
  ```
  In this context, we fill the first `sizeof(struct board)` bytes (which is safe because `sizeof()` returns the amount of
  memory in bytes occupied by the struct board) of the memory area of the struct as pointed to by the pointer board to the
  struct with the constant byte 0.
  This function is used similarly in various places to initialize the struct board in order to avoid memory errors and leaks.
```c
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
```

3. Then, the program reads the three arguments given by the user.
Namely, it utilizes the function `parseFenRec`, which parses the FEN data, and it saves the timeout in a variable (assuming it is an integer). The legal moves given are read later.
```c
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
timeout++;
debugPrint("Timeout given: %d\n", timeout);
```

4. Next, the program selects a move to play and outputs it in stdout, as required.
It does so by utilizing the function `choose_move`, which selects the move it thinks is best and returns its index (according to the given list of the legal moves). In case of a negative index, there was an error in the function and the program exits with ERROR_CODE (as initialized as -1 in init.h).
```c
// Then, showing current board state (debug print).
if (DEBUG) printBoard(board);

// Finally, selecting a move to play and printing it.
int move_chosen = choose_move(argv[1], argv[2], timeout);
if(move_chosen == -1) {
    free(board);
    return ERROR_CODE;
}
printf("%d\n", move_chosen);
```

5. Lastly, the program frees all the memory occupied and returns with exit code 0.
```c
free(board);
return 0;
```

The choose_move function contains the main code, which initializes the bitboards, 
performs the evaluation by calling the proper functions, and returns the index of the chosen move.

### **bitboard.c**
Processes chessboard states using bitboards. It includes functions for parsing FEN notation, updating chess moves, 
handling special moves (castling, en passant, promotions), and debug-printing board states. 

### **capture.c**
Includes functions which generate all the legal moves that are captures.

### **evaluate.c**
Includes the evaluation functions, which basically assign an arithmetic value to a specific board
state given (through bitboards and various other parameters as given in the struct board).

### **init.c**
Includes a debug print function for cleaner debug output handling, which prints only if DEBUG is enabled,
which is a macro that we define in `init.h`, along with many more macros, as well as the struct board itself, which contains
the necessary information for each board, aka the bitboards, the next player's letter ('w' or 'b'), whether castling and 
en passant are possible or not (checked separately), as well as the counters for halfmoves and full moves.

### **movegen.c**
Includes functions which generate all the legal moves that are **not** captures.

### **search.c**
Includes the main algorithm of the engine, minimax. As mentioned before, there are capabilities for further 
optimizations, but, unfortunately, not all were included because of various circumstances.

### **tools.c**
Includes various custom-made functions, mostly for memory handling (saving and freeing the moves) and also
some for debugging purposes.

## Usage

### Compilation and execution
One can compile the chess engine by running:
```sh
make engine
```
This will generate an executable called `engine`.

The compilation occurs thanks to the make file `Makefile`.  

### Command line arguments
The engine can be run with the following command line arguments:
```sh
./engine "<FEN>" "<moves>" <timeout>
```
Example (initial board state):
```sh
./engine "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" \
"a3 a4 b3 b4 c3 c4 d3 d4 e3 e4 f3 f4 g3 g4 h3 h4 Na3 Nc3 Nf3 Nh3" 3
```
This command provides the engine with:
- **A board state in FEN format**
- **A list of possible moves (space-separated)**
- **A time limit (in seconds)**

The engine will output the index of the selected move (starting from 0) and exit successfully (code `0`).

### Demo

#### Command Line Interface

Given the following FEN data:
```
rnbqkbnr/ppppp2p/8/5pp1/4P3/2N5/PPPP1PPP/R1BQKBNR w KQkq - 0 3
```
as visualised below:  
<img src="https://github.com/progintro/hw3-fork-overflow/blob/main/assets/before_fools.png?raw=true" width="312.5" height="326.9">

  
then the legal moves (for white, since it is their turn, as noted in the FEN string):
```
"a3 a4 b3 b4 d3 d4 e5 f3 f4 g3 g4 h3 h4 Rb1 Nb1 Na4 Nb5 Nd5 Ne2 Qe2 Qf3 Qg4 Qh5 Be2 Bd3 Bc4 Bb5 Ba6"
```
as well as the timeout:
```
3
```
the output should look simply be:
```
22
```
since the move in index 22 of this 0-indexed array of strings is the best move in this case, because it leads to checkmate[^2].

#### Web Assembly Interface  

:arrow_forward:
Prerequisites: 
- emscripten compiler (`apt install emscripten`)
- python 3 (`apt install python3`)  

  
1. Compile binary through:

```sh
make engine
```

2. Compile it to webassembly:

```sh
make WEB_TARGET=web/engine.wasm web/engine.wasm
```

3. Run the website:

```sh
$ make run
python3 -m http.server --directory web
Serving HTTP on 0.0.0.0 port 8000 (http://0.0.0.0:8000/) ...
```

You should now be able to interact with the engine on http://0.0.0.0:8000/ (http://localhost:8000/). It will look like this:

<img src="https://github.com/progintro/hw3-fork-overflow/blob/main/web/img/screenshot.png?raw=true" width="320" height="440">

## What we learned
The goal to outperform other chess engines is merely secondary compared to satisfying our desire to learn programming
and constantly improve our coding skills, as well as our ability to work together as a team. We are proud to have achieved these objectives - below is a list of some invaluable lessons that this challenging project has taught us:
  
- **Combining and organizing multiple coding files**  
The arrangement of an organized structure of our code in multiple files as well as the process of deciding how to
allocate our various functions in it.

- **Understanding and modifying the Makefile**  
Altering the given Makefile[^last-1] to compile all of our code files together at once with the appropriate flags.  

- **Creating and performing operations on bitboards**  
Getting familiar with a completely new form of data storing for the chess board, which is largely more efficient, though
different in nature compared to traditional data structures such as one-dimensional arrays.  

- **Using macro definitions in C**  
As we learned, those serve as shorthand for commonly used logic or conditions in our code and have saved us a lot of time.

- **Getting used to git commands**  
Git is an incredible tool which we had the pleasure to utilize extensively, as we both had to work together, merge commits, create branches, e.t.c. to organize and merge our code.

- **Fixing memory errors and memory leaks with Valgrind**  
Valgrind is a very useful tool as well, pinpointing to the exact lines of errors and leaks (the temporary -g3 debugging compilation flag also helped).

## Notes
To compose this writeup, we utilized a free and open-source reference guide with Markdown tips and workarounds[^last].
Our biggest and most helpful source of information was the chess programming wiki[^wiki].

## Contact
This project was created by Grigorios-Christos Psyllos and Eleni Mouchli, first-year undergraduate students in the Department of Informatics and Telecommunications at the National and Kapodistrian University of Athens.

[^elo]:[Elo Rating System](https://www.chess.com/terms/elo-rating-chess)  
[^minimax]:[Minimax Algorithm](https://en.wikipedia.org/wiki/Minimax)  
[^1]: [Exercise Instructions and Constraints](https://progintro.github.io/assets/pdf/hw3.pdf)  
[^2]: [Fool's mate.](https://en.wikipedia.org/wiki/Fool%27s_mate)  
[^last-1]: [Makefile Documentation](https://en.wikipedia.org/wiki/Make_(software))  
[^last]: [Markdown guide.](https://www.markdownguide.org/)  
[^horizon]: [[Tony Marsland](https://www.chessprogramming.org/Tony_Marsland)) (1992). Computer Chess and Search. Encyclopedia of Artificial Intelligence (2nd ed.) (ed. S.C. Shapiro) pp. 224-241. John Wiley & Sons, Inc., New York, NY. ISBN 0-471-50305-3.](https://webdocs.cs.ualberta.ca/~tony/RecentPapers/encyc.mac-1991.pdf)
[^fen]: [Forsyth-Edwards Notation (FEN)](https://en.wikipedia.org/wiki/Forsyth%E2%80%93Edwards_Notation)
[^inspiration]: [Inspiration Lecture for Variadic Functions pp. 40-41.](https://progintro.github.io/assets/pdf/lec23.pdf)
[^builtin_ctzll]: [Built-in GCC compiler function builtin_ctzll()](https://codeforces.com/blog/entry/72437)
[^wiki]: [Chess Programming wiki](https://www.chessprogramming.org/Main_Page)
