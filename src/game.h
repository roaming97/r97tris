#ifndef TETROMINO_HEADER
#define TETROMINO_HEADER

#define QUEUE_SIZE 5

static const unsigned char BOARD_WIDTH = 10;
static const unsigned char BOARD_HEIGHT = 20;
static const unsigned char CELL_SIZE = 16;
static const unsigned int PIECE_COLORS[8] = {
	0x999999, // Empty/placeholder piece
	0x5FF4EA,
	0x1550F8,
	0xEF7C2F,
	0xEFD82B,
	0x62ED2F,
	0xED3131,
	0xE450F4,
};

static const unsigned char ROTATION_DATA[7][4][4][2] = {
	{
		// I
		/*
		{{1, 2}, {2, 2}, {3, 2}, {4, 2}},
		{{2, 1}, {2, 2}, {2, 3}, {2, 4}},
		{{0, 2}, {1, 2}, {2, 2}, {3, 2}},
		{{2, 3}, {2, 2}, {2, 1}, {2, 0}},
		*/
		{{0, 1}, {1, 1}, {2, 1}, {3, 1}},
		{{2, 0}, {2, 1}, {2, 2}, {2, 3}},
		{{0, 1}, {1, 1}, {2, 1}, {3, 1}},
		{{2, 0}, {2, 1}, {2, 2}, {2, 3}},
	},
	{
		// J️
		{{0, 0}, {0, 1}, {1, 1}, {2, 1}},
		{{2, 0}, {1, 0}, {1, 1}, {1, 2}},
		{{2, 2}, {2, 1}, {1, 1}, {0, 1}},
		{{0, 2}, {1, 2}, {1, 1}, {1, 0}},
	},
	{
		// L️
		{{2, 0}, {2, 1}, {1, 1}, {0, 1}},
		{{2, 2}, {1, 2}, {1, 1}, {1, 0}},
		{{0, 2}, {0, 1}, {1, 1}, {2, 1}},
		{{0, 0}, {1, 0}, {1, 1}, {1, 2}},
	},
	{
		// O️
		{{1, 0}, {2, 0}, {1, 1}, {2, 1}},
		{{1, 1}, {2, 1}, {1, 2}, {2, 2}},
		{{0, 1}, {1, 1}, {0, 2}, {1, 2}},
		{{0, 0}, {1, 0}, {0, 1}, {1, 1}},
	},
	{
		// S
		{{1, 0}, {2, 0}, {0, 1}, {1, 1}},
		{{2, 1}, {2, 2}, {1, 0}, {1, 1}},
		{{1, 2}, {0, 2}, {2, 1}, {1, 1}},
		{{0, 1}, {0, 0}, {1, 2}, {1, 1}},
	},
	{
		// Z
		{{0, 0}, {1, 0}, {1, 1}, {2, 1}},
		{{2, 0}, {2, 1}, {1, 1}, {1, 2}},
		{{2, 2}, {1, 2}, {1, 1}, {0, 1}},
		{{0, 2}, {0, 1}, {1, 1}, {1, 0}},
	},
	{
		// T️
		{{0, 1}, {1, 0}, {1, 1}, {2, 1}},
		{{1, 0}, {2, 1}, {1, 1}, {1, 2}},
		{{2, 1}, {1, 2}, {1, 1}, {0, 1}},
		{{1, 2}, {0, 1}, {1, 1}, {1, 0}},
	}
};

/*
	Wall kick data indexing:

	p = Piece type (2)
	r = Rotation (4)
	d = Direction (2)
	n = Test number (5 - 1 since all first tets are 0, 0)
	(x, y) (2)
*/
static const int WALLKICK_TESTS = 4;
static const unsigned char WALLKICK_DATA[2][4][2][4][2] = {
	// Non-I pieces
	{
		// Rotation 0
		{
			// Direction -1
			{{1, 0}, {1, 1}, {0, -2}, {1, -2}},
			// Direction 1
			{{-1, 0}, {-1, 1}, {0, -2}, {-1, -2}},
		},
		// Rotation 1
		{
			// Direction -1
			{{1, 0}, {1, -1}, {0, 2}, {1, 2}},
			// Direction 1
			{{1, 0}, {1, -1}, {0, 2}, {1, 2}},
		},
		// Rotation 2
		{
			// Direction -1
			{{-1, 0}, {-1, 1}, {0, -2}, {-1, -2}},
			// Direction 1
			{{1, 0}, {1, 1}, {0, -2}, {1, -2}},
		},
		// Rotation 3
		{
			// Direction -1
			{{-1, 0}, {-1, -1}, {0, 2}, {-1, 2}},
			// Direction 1
			{{-1, 0}, {-1, -1}, {0, 2}, {-1, 2}},
		},
	},
	// I piece
	{
		// Rotation 0
		{
			// Direction -1
			{{-1, 0}, {2, 0}, {-1, 2}, {2, -1}},
			// Direction 1
			{{-2, 0}, {1, 0}, {-2, -1}, {1, 2}},
		},
		// Rotation 1
		{
			// Direction -1
			{{2, 0}, {-1, 0}, {2, 1}, {-1, -2}},
			// Direction 1
			{{-1, 0}, {2, 0}, {-1, 2}, {2, -1}},
		},
		// Rotation 2
		{
			// Direction -1
			{{1, 0}, {-2, 0}, {1, -2}, {-2, -1}},
			// Direction 1
			{{2, 0}, {-1, 0}, {2, 1}, {-1, -2}},
		},
		// Rotation 3
		{
			// Direction -1
			{{-2, 0}, {1, 0}, {-2, -1}, {1, 2}},
			// Direction 1
			{{1, 0}, {-2, 0}, {1, -2}, {-2, 1}},
		},
	}
};

unsigned char *board = NULL;

enum PieceIndex
{
	PIECE_NONE,
	PIECE_I,
	PIECE_J,
	PIECE_L,
	PIECE_O,
	PIECE_S,
	PIECE_Z,
	PIECE_T,
};
typedef enum PieceIndex PieceIndex;

enum SoundIndex
{
	SOUND_PIECECOLLIDE,
	SOUND_DISAPPEAR = 8,
	SOUND_FALL = 9,
	SOUND_PIECELOCK = 10,
	SOUND_IRS = 11,
};

enum Rotation
{
	ROT_0,
	ROT_90,
	ROT_180,
	ROT_270,
};
typedef enum Rotation Rotation;

typedef struct Block
{
	unsigned short collide : 1;
	unsigned short piece : 3;
	unsigned short rotation : 2;
	unsigned short y : 5;
	unsigned short x : 5;
} Block;

// Create a new block.
Block new_block(unsigned short data);

typedef struct Piece
{
	Block blocks[4];
} Piece;

// Create a new tetromino from a piece index.
// Passing index `-1` creates a random piece and adds it to the queue.
Piece *new_piece(int idx);
// Checks if a piece can be moved to an offest.
bool check_move(Piece *piece, int x, int y);
// Move a tetromino by an offset in x and/or y.
void move_piece(Piece *piece, int x, int y);
// Rotate a tetromino in a given direction, `1` is clockwise and `-1` counter-clockwise.
void rotate_piece(Piece *piece, int direction);
// Lock a tetromino by placing it on the play board then frees its memory.
void lock_piece(Piece *piece);
// Checks if a line has been cleared.
void check_line(unsigned int y);

#define DAS_FRAMES 12
#define ARE_FRAMES 30

typedef struct GameState
{
	Piece *piece;
	PieceIndex queue[5]; // store the previous pieces in a queue
	unsigned char *board;
	uint64_t ticks;
	unsigned int level;
	unsigned int score;
	unsigned int gravity;
	unsigned int tpu; // ticks per update
	unsigned int ftr; // fall tickrate
	unsigned int dhf; // direction hold frames
	unsigned int das; // Delayed Auto Shift, frames before autorepeat
	unsigned int are; // spawn delay, ticks are copied into this variable so it can be compared against `ARE_FRAMES` 
	bool game_over;
} GameState;

void init_game_state();
void draw_board();
void restart_game();

bool every_n_frames(unsigned int frames);
bool key_is_down_buffered(SDL_KeyCode key);

void init_queue();
unsigned int new_index(unsigned int seed);
bool index_in_queue(int idx);
void update_queue(int idx);

// The game's state.
// Yes, I was too lazy to try and implement it into the engine manager.
GameState *game_state = NULL;

#endif