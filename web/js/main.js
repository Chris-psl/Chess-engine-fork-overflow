// Main entrypoint for starting a chess game

// Sole requirement for our chess engine: a memory base address
// to place our FEN and moves strings
const importObject = {
    env: {
        __memory_base: 0,
    }
}

// All data related to a game being played on screen
class GameState {
    constructor(game, board, white_move, black_move, white_to_move, text) {
        // the Chess game object
        this.game = game
        // the chessboard
        this.board = board
        // functions to call when it's time to move for each player
        this.white_move = white_move
        this.black_move = black_move
        // whose turn it is
        this.white_to_move = white_to_move
        // text div to provide game updates
        this.text = text
    }
}

var TIME_BETWEEN_MOVES = 100 // milliseconds

// Random engine for testing - we gotta beat this!
var randomEngine = function (state) {
    var possibleMoves = state.game.moves()
    var randomIdx = Math.floor(Math.random() * possibleMoves.length)
    state.game.move(possibleMoves[randomIdx])
    state.board.position(state.game.fen())
}

// Game driver that runs the game loop
var gameDriver = function (state) {
    state.white_to_move = !state.white_to_move
    state.text.innerHTML = state.white_to_move ? 'White to move' : 'Black to move'
    if(!state.game.game_over()) {
        if (state.white_to_move && state.white_move === null || !state.white_to_move && state.black_move === null) {
            console.log("Human's turn!")
            return
        } else {
            if (state.white_to_move) {
                state.white_move()
            } else {
                state.black_move()
            }
            state.text.innerHTML = state.white_to_move ? 'Black to move' : 'White to move'
            window.setTimeout(() => gameDriver(state), TIME_BETWEEN_MOVES)
        }
    } else {
        console.log("Game over")
        console.log(state.game.in_checkmate())
        var result = ''
        if (state.game.in_draw()) {
            result += "Draw 1 - 1"
        }
        if (state.game.in_checkmate()) {
            result += state.white_to_move ? "Black wins 0 - 3" : "White wins 3 - 0"
        }
        result += '<br />' + state.game.pgn()
        state.text.innerHTML = result
        // console.log()
    }
}

// Interface with a WebAssembly engine that implements the choose_move function
// to select the best move given a FEN and list of possible moves
var build_engine = function (wasm, game, board) {
    return () => {
        // console.log("running engine")
        if (game.game_over()) return
        var fen = game.fen();
        var moves = game.moves();
        var moves_str = moves.join(' ');
        var time = 3;
        console.log(fen, moves_str, time);
        var memory = wasm.instance.exports.memory;
        var buffer = new Uint8Array(memory.buffer);
        for (var i = 0; i < fen.length; i++) {
            buffer[i] = fen.charCodeAt(i);
        }
        buffer[fen.length] = 0;
        for (var i = 0; i < moves_str.length; i++) {
            buffer[fen.length + i + 1] = moves_str.charCodeAt(i);
        }
        buffer[fen.length + moves_str.length + 1] = 0;
        var num = wasm.instance.exports.choose_move(0, fen.length + 1, time);
        game.move(moves[num])
        board.position(game.fen())
        return num;
    }
}

// Main entrypoint for starting a chess game
startGame = function (board_id) {
    console.log("Starting game")
    var board = null
    var game = new Chess()
    var white = document.getElementById('white').value
    var black = document.getElementById('black').value
    var text = document.createElement("div")
    var white_move = null
    var black_move = null

    var state = new GameState(game, board, white_move, black_move, white_to_move, text)

    // Set up the game based on the player selections
    if (white == "random") {
        state.white_move = () => randomEngine(state)
    }
    if (black == "random") {
        state.black_move = () => randomEngine(state)
    }

    // assert(white == "human" || white == "random" || white == "engine")
    // assert(black == "human" || black == "random" || black == "engine")

    // Game driver starts by flipping the turn to white
    var white_to_move = false

    // Human player setup is special, requires defining
    // onSnapEnd, onDragStart, and onDrop
    var onSnapEnd = function () {
        state.board.position(game.fen())
        state.white_to_move = !state.white_to_move
        window.setTimeout(() => gameDriver(state), TIME_BETWEEN_MOVES)
    }
    var onDragStart = function (source, piece, position, orientation) {
        // do not pick up pieces if the game is over
        if (!white_to_move && white != "human") return false
        if (white_to_move && black != "human") return false
        if (state.game.game_over()) return false

        if (white == "human" && black != "human" && piece.search(/^b/) !== -1) return false
        if (black == "human" && white != "human" && piece.search(/^w/) !== -1) return false
    }

    var onDrop = function (source, target) {
        // see if the move is legal
        var move = state.game.move({
          from: source,
          to: target,
          promotion: 'q' // NOTE: always promote to a queen for example simplicity
        })

        // illegal move
        if (move === null) return 'snapback'
    }

    var config = {
      draggable: white == "human" || black == "human",
      position: 'start',
      onDragStart: onDragStart,
      onDrop: onDrop,
      onSnapEnd: onSnapEnd
    }

    state.board = Chessboard(board_id, config)
    // append a div to the board_id element
    var board_elem = document.getElementById(board_id)
    board_elem.appendChild(text)

    var engines = []
    if (white == "engine") {
        // use Webassembly instantiate streaming to fetch the engine and then we'll use Promise.all to populate them all
        engines.push(WebAssembly.instantiateStreaming(fetch('/engine.wasm'), importObject))
    }
    if (black == "engine") {
        engines.push(WebAssembly.instantiateStreaming(fetch('/engine.wasm'), importObject))
    }

    text.innerHTML = 'White to move'

    // Fetch all engines and then start the game(s)
    Promise.all(engines).then(wasms => {
        var engine_index = 0
        if (white == "engine") {
            state.white_move = build_engine(wasms[engine_index], state.game, state.board)
            engine_index += 1
        }
        if (black == "engine") {
            state.black_move = build_engine(wasms[engine_index], state.game, state.board)
            engine_index += 1
        }

        // Start the game loop
        window.setTimeout(() => gameDriver(state), 100)
    })
}
