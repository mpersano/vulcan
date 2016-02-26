/* game.h -- part of vulcan
 *
 * This program is copyright (C) 2006 Mauro Persano, and is free
 * software which is freely distributable under the terms of the
 * GNU public license, included as the file COPYING in this
 * distribution.  It is NOT public domain software, and any
 * redistribution not permitted by the GNU General Public License is
 * expressly forbidden without prior written permission from
 * the author.
 *
 */

#ifndef GAME_H_
#define GAME_H_

enum player_type {
	HUMAN_PLAYER,
	COMPUTER_PLAYER
};

enum game_status {
	MATE_FOR_BLACK,
	MATE_FOR_WHITE,
	IN_PLAY,
};

/* board dimensions */
enum {
	BLEVELS = 7,
	BROWS = 10 + 4,		/* 4 extra rows for bounds checking */
	BCOLS = 6 + 2,		/* 2 extra columns for bounds checking */
	BAREA = BROWS*BCOLS,

	NUM_MAIN_BOARDS = 3,
	NUM_ATTACK_BOARDS_PER_SIDE = 2,
	NUM_ATTACK_BOARDS = 2*NUM_ATTACK_BOARDS_PER_SIDE,

	MAIN_BOARD_SIZE = 4,
	ATTACK_BOARD_SIZE = 2,
};

struct board_state {
	unsigned char board[BLEVELS][BAREA];
	unsigned attack_board_bits;		/* bit map for presence of
						 * attack boards  */
	unsigned attack_board_side;		/* bit map for color of
						 * attack boards  */
	int material_imbalance;			/* positive value is good for
						 * white */
	unsigned castling_rights;		/* bitmap for castling rights */
	int cur_ply;
};

enum square_state {
	BLACK_FLAG = 8,
	PIECE_MASK = BLACK_FLAG - 1,
	MOVED_FLAG = 16,

	PAWN = 1,
	ROOK,
	KNIGHT,
	BISHOP,
	QUEEN,
	KING,

	NUM_PIECES = KING - PAWN + 1,

	/* each square may be in one of the states below: */

	INVALID = 255,	/* for bounds checking */
	EMPTY = 0,

	WHITE_PAWN = PAWN,
	MOVED_WHITE_PAWN = MOVED_FLAG|PAWN,
	WHITE_ROOK = ROOK,
	WHITE_KNIGHT = KNIGHT,
	WHITE_BISHOP = BISHOP,
	WHITE_QUEEN = QUEEN,
	WHITE_KING = KING,

	BLACK_PAWN = BLACK_FLAG|PAWN,
	MOVED_BLACK_PAWN = MOVED_FLAG|BLACK_PAWN,
	BLACK_ROOK = BLACK_FLAG|ROOK,
	BLACK_KNIGHT = BLACK_FLAG|KNIGHT,
	BLACK_BISHOP = BLACK_FLAG|BISHOP,
	BLACK_QUEEN = BLACK_FLAG|QUEEN,
	BLACK_KING = BLACK_FLAG|KING,
};

/* attack board position bits */
enum {
	AB_UP_FLAG = 1,
	AB_RIGHT_FLAG = 2,
	AB_ABOVE_FLAG = 4,
};

/* position of an attack board relative to the main board it's attached to */
enum {
	AB_BELOW_LEFT_DOWN,
	AB_BELOW_LEFT_UP,
	AB_BELOW_RIGHT_DOWN,
	AB_BELOW_RIGHT_UP,
	AB_ABOVE_LEFT_DOWN,
	AB_ABOVE_LEFT_UP,
	AB_ABOVE_RIGHT_DOWN,
	AB_ABOVE_RIGHT_UP,
};

/* castling rights bits */
enum {
	CR_WHITE_KINGSIDE = 1UL,
	CR_WHITE_QUEENSIDE = 2UL,
	CR_BLACK_KINGSIDE = 4UL,
	CR_BLACK_QUEENSIDE = 8UL,
};

#define ATTACK_BOARD_IS_ACTIVE(state, main_board, attack_board) \
  ((state)->attack_board_bits & (1UL << (8*(main_board) + (attack_board))))

#define ATTACK_BOARD_IS_BLACK(state, main_board, attack_board) \
  ((state)->attack_board_side & (1UL << (8*(main_board) + (attack_board))))

#define ATTACK_BOARD_IS_WHITE(state, main_board, attack_board) \
  !ATTACK_BOARD_IS_BLACK(state, main_board, attack_board)

#define ATTACK_BOARD_SIDE(board, main_board, attack_board) \
  (ATTACK_BOARD_IS_BLACK(board, main_board, attack_board) ? BLACK_FLAG : 0)

#define ATTACK_BOARD_IS_UP(attack_board) ((attack_board) & AB_UP_FLAG)

#define ATTACK_BOARD_IS_DOWN(attack_board) (!ATTACK_BOARD_IS_UP(attack_board))

#define ATTACK_BOARD_IS_RIGHT(attack_board) ((attack_board) & AB_RIGHT_FLAG)

#define ATTACK_BOARD_IS_LEFT(attack_board) !ATTACK_BOARD_IS_RIGHT(attack_board)

#define ATTACK_BOARD_IS_ABOVE(attack_board) ((attack_board) & AB_ABOVE_FLAG)

#define ATTACK_BOARD_IS_BELOW(attack_board) \
  (!ATTACK_BOARD_IS_ABOVE(attack_board))

/* some constants used when rendering the board */
#define SQUARE_SIZE 20.f
#define MAIN_BOARD_DISTANCE 80.f	/* distance between main boards */
#define ATTACK_BOARD_DISTANCE 36.f	/* distance from main board to
					 * attached attack board */

/* selection flags */
enum selection_type {
	UNSELECTED_SQUARE,
	SELECTED_SQUARE,
	ATTACKED_SQUARE,
	HIGHLIGHTED_PIECE,
};

struct selected_squares {
	unsigned char selection[BLEVELS][BAREA];
};

enum move_type {
	PIECE_MOVE,
	ATTACK_BOARD_MOVE,
	WHITE_KINGSIDE_CASTLING,
	WHITE_QUEENSIDE_CASTLING,
	BLACK_KINGSIDE_CASTLING,
	BLACK_QUEENSIDE_CASTLING,
};

struct position {
	int level;
	int square;
};

#define SAME_POS(p, q) ((p)->level == (q)->level && (p)->square == (q)->square)

struct attack_board {
	int main_board;		/* main board the attack board is attached to */
	int position;		/* position of the attack board with respect
				 * to main board  */
};

struct piece_move {
	enum move_type type;
	struct position from;
	struct position to;
};

struct attack_board_move {
	enum move_type type;
	struct attack_board from;
	struct attack_board to;
};

union move {
	enum move_type type;
	struct piece_move piece_move;
	struct attack_board_move attack_board_move;
};

enum {
	MAX_SQUARES_TOUCHED_PER_MOVE = 8
};

struct undo_move_info {
	int num_squares_touched;
	struct position squares_touched[MAX_SQUARES_TOUCHED_PER_MOVE]; 
	unsigned char prev_states[MAX_SQUARES_TOUCHED_PER_MOVE];
	unsigned prev_attack_board_bits;
	unsigned prev_attack_board_side;
	unsigned prev_castling_rights;
	int prev_material_imbalance;
};

extern const int piece_values[NUM_PIECES];

void
init_board_state(struct board_state *state);

enum game_status
get_game_status(struct board_state *state);

void
do_move(struct board_state *state, const union move *move,
  struct undo_move_info *undo_info);

void
undo_move(struct board_state *state, const struct undo_move_info *undo_info);

void
get_main_board_position(struct position *first_pos, int main_board);

void
get_attack_board_position(struct position *first_pos, int main_board,
  int attack_board);

int
attack_board_is_empty(const struct board_state *state, int main_board,
  int attack_board);
  
int
attack_board_can_move(const struct board_state *state, int main_board,
  int attack_board);

int
get_attack_board_for_position(struct attack_board *apos,
  const struct board_state *state, const struct position *pos);

void
get_closest_main_board_square_for_attack_board(struct position *pos,
  const struct attack_board *aboard);

char *
move_as_string(const struct board_state *state, const union move *move);

#endif /* GAME_H_ */
