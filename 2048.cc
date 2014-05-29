#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <algorithm>

#define N 4
#define ROW_NUM 65536
#define ROW_MASK 0xffffll

#if 0
#define ALWAYS_INLINE __attribute__((always_inline))
#else
#define ALWAYS_INLINE
#endif

typedef uint64_t board_t;
typedef uint16_t row_t;


// ---------------------------------------------------------
// parameters
int max_lookahead = 6;
int max_lookaheads[] = {  // TODO fine tune
  6, 6, 6, 6,
  6, 6, 5, 5,
  5, 5, 5, 5,
  4, 4, 4, 4,
};

float search_threshold = 0.000f;

enum enum_parameters {
	PARAM_STILLALIVE,
	PARAM_FREECELL,
	PARAM_CENTEROFMASS1,
	PARAM_CENTEROFMASS2,
	PARAM_CENTEROFMASS3,
	PARAM_CENTEROFMASS4,
	PARAM_COUNT
};

int parameters[] ={
  9500,
  52,
  21,
  150,
  188,
  80,
};

// ---------------------------------------------------------
// Utility functions for testing and debugging
// Shoudn't be used in final solver code
int my_random_seed = 0;
inline int my_random() {
    my_random_seed = my_random_seed * 1103515245 + 12345;
    return my_random_seed % ((unsigned long)RAND_MAX + 1);
}

double now() {
   return (double)clock() / CLOCKS_PER_SEC;
}

void print_urow(int urow[N]) {
  printf("%d %d %d %d\n", urow[0], urow[1], urow[2], urow[3]);
}

void print_row(row_t r) {
  printf("%04x\n", r);
}

void print_board(board_t b) {
  printf("%04llx\n%04llx\n%04llx\n%04llx\n",
      b & 0xffffll,
      b >> 16 & 0xffffll,
      b >> 32 & 0xffffll,
      b >> 48 & 0xffffll);
}

void print_pretty_board(board_t b) {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      int p = i * 4 + (3 - j);
      int x = b >> (p*4) & 0xf;
      if (x)
        printf("%6d", 1<<x);
      else
        printf("%6s", ".");
    }
    printf("\n");
  }
}

// ---------------------------------------------------------
// Functions to implement solver. Must be highly OPTIMIZED

row_t row_left_table[ROW_NUM];
row_t row_right_table[ROW_NUM];
float my_score_table_L[65536];
float my_score_table_R[65536];
row_t diff_table[65536];
float specialized_table[65536];

inline ALWAYS_INLINE board_t transpose(board_t x) {
  board_t t;
  t = (x ^ (x >> 12)) & 0x0000f0f00000f0f0ull;
  x ^= t ^ (t << 12);
  t = (x ^ (x >> 24)) & 0x00000000ff00ff00ull;
  x ^= t ^ (t << 24);
  return x;
}

inline ALWAYS_INLINE int count_blank(board_t b) {
  b = ~b;
  b &= b >> 2;
  b &= b >> 1;
  b &= 0x1111111111111111ull;
  b = (b * 0x1111111111111111ull) >> 60;
  return b;
}

inline ALWAYS_INLINE board_t do_move_0(board_t b) {
  return
    (board_t)row_left_table[b >> 0 & ROW_MASK] << 0 |
    (board_t)row_left_table[b >> 16 & ROW_MASK] << 16 |
    (board_t)row_left_table[b >> 32 & ROW_MASK] << 32 |
    (board_t)row_left_table[b >> 48 & ROW_MASK] << 48;
}

inline ALWAYS_INLINE board_t do_move_1(board_t b) {
  return
    (board_t)row_right_table[b >> 0 & ROW_MASK] << 0 |
    (board_t)row_right_table[b >> 16 & ROW_MASK] << 16 |
    (board_t)row_right_table[b >> 32 & ROW_MASK] << 32 |
    (board_t)row_right_table[b >> 48 & ROW_MASK] << 48;
}

inline ALWAYS_INLINE board_t do_move_2(board_t b) {
  return transpose(do_move_0(transpose(b)));
}

inline ALWAYS_INLINE board_t do_move_3(board_t b) {
  return transpose(do_move_1(transpose(b)));
}

inline ALWAYS_INLINE board_t do_move(board_t b, int m) {
  switch (m) {
    case 0: return do_move_0(b);
    case 1: return do_move_1(b);
    case 2: return do_move_2(b);
    case 3: return do_move_3(b);
  }
  return 0;
}

float apply_score_table(board_t b, float* table) {
  return
    table[(b >>  0) & ROW_MASK] +
    table[(b >> 16) & ROW_MASK] +
    table[(b >> 32) & ROW_MASK] +
    table[(b >> 48) & ROW_MASK];
}

float eval_monotone(board_t b) {
    float LR = std::max(
        apply_score_table(b, my_score_table_L),
        apply_score_table(b, my_score_table_R));
    board_t t = transpose(b);
    float UD = std::max(
        apply_score_table(t, my_score_table_L),
        apply_score_table(t, my_score_table_R));
    return LR + UD;
}

float eval_smoothness(board_t b) {
  board_t LR = 
    board_t(diff_table[(b >> 0) & ROW_MASK]) << 0 |
    board_t(diff_table[(b >> 16) & ROW_MASK]) << 16 |
    board_t(diff_table[(b >> 32) & ROW_MASK]) << 32 |
    board_t(diff_table[(b >> 48) & ROW_MASK]) << 48;

  board_t t = transpose(b);
  board_t UD = 
    board_t(diff_table[(t >> 0) & ROW_MASK]) << 0 |
    board_t(diff_table[(t >> 16) & ROW_MASK]) << 16 |
    board_t(diff_table[(t >> 32) & ROW_MASK]) << 32 |
    board_t(diff_table[(t >> 48) & ROW_MASK]) << 48;
  UD = transpose(UD);

  int s = 0;
  board_t x = b;
  for (int i = 0; i < 16; i++) {
    int p = abs((1<<(UD&0xf)) - (1<<(x&0xf)));
    int q = abs((1<<(LR&0xf)) - (1<<(x&0xf)));
    s += std::min(p, q);
    UD >>= 4;
    LR >>= 4;
    x >>= 4;
  }
  return -s;
}

#if 0
float eval(board_t b) {
  float score = 0;

  int fill = 16 - count_blank(b);
  score += - fill*fill;
  score += eval_smoothness(b);
  score += eval_monotone(b);

  return score;
}
#else

float eval(board_t b) {
  float score = parameters[PARAM_STILLALIVE];
  int blank = count_blank(b);
  int freecell = parameters[PARAM_FREECELL];
  while (blank && freecell) {
    score += freecell;
    freecell >>= 1;
    blank--;
  }

  board_t t = transpose(b);
  score += 
    (
      abs(apply_score_table(b, specialized_table)) +
      abs(apply_score_table(t, specialized_table))) / (1<<10);
  return score;
}
#endif

float search_min(board_t b, int depth, float nodep);
float search_max(board_t b, int depth, float nodep) {
  float best_score = -1e10;
  for (int m = 0; m < 4; m++) {
    board_t b2 = do_move(b, m);
    if (b == b2)
      continue;
    float s = search_min(b2, depth - 1, nodep);
    if (s > best_score)
      best_score = s;
  }
  return best_score;
}

#include <tr1/unordered_map>
typedef std::tr1::unordered_map<board_t, float> trans_table_t;
typedef std::tr1::unordered_map<board_t, char> trans_table2_t;

trans_table_t cache1;
trans_table2_t cache2;

float search_min(board_t b, int depth, float nodep) {
  if (depth == 0 || nodep < search_threshold)
    return eval(b);

  {
    if (cache1.find(b) != cache1.end())
      return cache1[b];
  }

  int blank = count_blank(b);
  nodep /= blank;

  float score = 0;
  board_t tile = 1;
  board_t tmp = b;
  while (tile) {
    if ((tmp & 0xf) == 0) {
      score += search_max(b | tile, depth, nodep*0.9f) * 0.9f;
      score += search_max(b | tile << 1, depth, nodep*0.1f) * 0.1f;
    }
    tile <<= 4;
    tmp >>= 4;
  }
  float result = score / blank;
  cache1[b] = result;
  return result;
}

bool maybe_dead_maxnode(board_t b, int depth);
bool maybe_dead_minnode(board_t b, int depth) {
  if (depth <= 0)
    return false;

  int blank = count_blank(b);
  if (blank >= depth+1) return false;

  const trans_table2_t::iterator& it = cache2.find(b);
  if (it != cache2.end())
    return it->second;

  board_t tmp = b;
  board_t tile_2 = 1;
  while (tile_2) {
    if ((tmp & 0xf) == 0) {
      if (maybe_dead_maxnode(b | tile_2, depth)) {
  //cache2[b] = true;
        return true;
      }
    }
    tmp >>= 4;
    tile_2 <<= 4;
  }

  cache2[b] = false;
  return false;
}

bool maybe_dead_maxnode(board_t b, int depth) {
  for(int m = 0; m < 4; m++) {
    board_t b2 = do_move(b, m);
    if (b2 == b)
      continue;

    if (!maybe_dead_minnode(b2, depth-1))
      return false;
  }
  return true;
}

char move_str[] = "RLUD";
int root_search_move(board_t b) {
  cache1.clear();
  float best_score = -1e10-1;
  int best_move = 0;

  int badmove[4] = {0};
  int nbadmove = 0;
  cache2.clear();
#if 1
  for(int m = 0; m < 4; m++) {
    if (maybe_dead_minnode(do_move(b, m), 12)) {
      badmove[m] = 1;
      nbadmove++;
    }
  }
#endif

  for (int m = 0; m < 4; m++) {
    if (nbadmove != 4 && badmove[m])
      continue;

    board_t b2 = do_move(b, m);
    if (b == b2)
      continue;

    int lookahead = max_lookahead;
    //lookahead = max_lookaheads[count_blank(b2)];
    float s = search_min(b2, lookahead - 1, 1.0f);
    if (s > best_score) {
      best_score = s;
      best_move = m;
    }
  }
  return best_move;
}

// ---------------------------------------------------------
// Functions to implement solver. Not performance critical.
// For example, used only in init functions.

inline void row_move_left(unsigned row[N]) {
  int ic, oc;
  ic = oc = 0;

  while (ic < N) {
    if (row[ic] == 0) {
      ic++;
      continue;
    }

    if (ic == oc) {
      ic++;
    } else if (row[oc] == 0) {
      row[oc] = row[ic];
      row[ic] = 0;
      ic++;
    } else if (row[oc] == row[ic]) {
      row[oc]++;
      if (row[oc] == 16) {
        // XXX hack
        row[oc]--;
      }
      row[ic] = 0;
      ic++;
      oc++;
    } else {
      oc++;
    }
  }
  oc++;
  while (oc < N)
    row[oc++] = 0;
}

inline row_t row_reverse(row_t r) {
  return (r & 0xf) << 12 | (r & 0xf0) << 4 | (r & 0xf00) >> 4 | (r & 0xf000) >> 12;
}

inline row_t row_pack(unsigned int urow[N]) {
  return urow[0] | urow[1] << 4 | urow[2] << 8 | urow[3] << 12;
}

void build_move_table() {
  for (unsigned row = 0; row <= 0xffff; row++) {
    unsigned urow[N] = {
      row >> 0 & 0xf,
      row >> 4 & 0xf,
      row >> 8 & 0xf,
      row >> 12 & 0xf,
    };

    row_move_left(urow);
    row_t result = row_pack(urow);
    row_left_table[row] = result;
    row_right_table[row_reverse(row)] = row_reverse(result);
  }
}

void build_eval_table() {
  for (unsigned row = 0; row <= 0xffff; row++) {
    unsigned urow[N] = {
      row >> 0 & 0xf,
      row >> 4 & 0xf,
      row >> 8 & 0xf,
      row >> 12 & 0xf,
    };

#if 1
    {
      int L, R;
      int m = 0;
      L = R = 0;
      for (int i = 0; i < 3; i++) {
        if (urow[i] != 0 && urow[i] >= urow[i+1]) {
          m++;
          L += m*m * 4;
        } else {
          L -= abs((urow[i]?1<<urow[i]:0) - (urow[i+1]?1<<urow[i+1]:0)) * 1.5;
          m = 0;
        }
      }

      m = 0;
      for (int i=0;i<3;i++) {
        if (urow[i] <= urow[i+1] && urow[i+1]) {
          m++;
          R += m*m * 4;
        } else {
          R -= abs((urow[i]?1<<urow[i]:0) - (urow[i+1]?1<<urow[i+1]:0)) * 1.5;
          m = 0;
        }
      }
      my_score_table_L[row] = L;
      my_score_table_R[row] = R;
    }
#endif
    {
      int w = 0;
      for (int i = 0; i < 4; i++) {
        int value = urow[i];

        if (value == 0)
          continue;
        int weight3 = value * parameters[PARAM_CENTEROFMASS4];
        int weight2 = value * (parameters[PARAM_CENTEROFMASS3] + weight3);
        int weight1 = value * (parameters[PARAM_CENTEROFMASS2] + weight2);
        int weight0 = value * (parameters[PARAM_CENTEROFMASS1] + weight1);
        w += ((int) (i * 2) - (4 - 1)) * weight0;
      }
      specialized_table[row] = w;
    }

#if 1
    {
      int d[4];
      int x[4] = { 1 << urow[0], 1<<urow[1], 1<<urow[2], 1<<urow[3]};
      d[0] = urow[1];
      d[1] = abs(x[0]-x[1])<abs(x[2]-x[1])?urow[0]:urow[2];
      d[2] = abs(x[1]-x[2])<abs(x[3]-x[2])?urow[1]:urow[3];
      d[3] = urow[2];
      diff_table[row] = d[0] << 0 |
        d[1] << 4 |
        d[2] << 8 |
        d[3] << 12;
    }
#endif
  }
}

void init() {
  build_move_table();
  build_eval_table();
}

// ---------------------------------------------------------
// Functions to implement game runner
int calculate_game_score(board_t b, int num_tile4) {
  int score = 0;
  // 4: 4
  // 8: 8 + 4*2
  // 16: 16 + 8*2 + 4*4
  // k: k*(log(k)-1)
  while (b) {
    int rank = b & 0xf;
    if (rank >= 2) {
      score += (rank - 1) << rank;
    }
    b >>= 4;
  }
  return score - num_tile4 * 4;
}

board_t random_tile(board_t b, int* num_tile4) {
  board_t tile = my_random() % 10 == 0 ? 2 : 1;
  if (tile == 2)
    (*num_tile4)++;

  int blank = count_blank(b);
  if (blank == 0) blank = 16;  // hack for "blank" overflow
  int n = my_random() % blank;
  board_t tmp = b;
  while (1) {
    while (tmp & 0xf) {
      tmp >>= 4;
      tile <<= 4;
    }
    if (n-- == 0)
      break;
    tmp >>= 4;
    tile <<= 4;
  }
  return b | tile;
}

bool is_can_move(board_t b) {
  for (int m = 0; m < 4; m++) {
    if (b != do_move(b, m))
      return true;
  }
  return false;
}

int find_max_tile(board_t b) {
  int r = 0;
  while (b) {
    r = std::max(r, (int)(b&0xf));
    b >>= 4;
  }
  return r;
}

void main_loop() {
  board_t b = 0;
  int num_tile4 = 0;
  b = random_tile(b, &num_tile4);
  b = random_tile(b, &num_tile4);

  int move_count = 0;
  print_pretty_board(b);
  while (is_can_move(b)) {
    int m = root_search_move(b);
    board_t b2 = do_move(b, m);
    if (b == b2) {
      printf("bad move\n");
      break;
    }
    move_count++;
    b = random_tile(b2, &num_tile4);
    printf("step %d, move %c, score=%d\n",
        move_count, move_str[m],
        calculate_game_score(b, num_tile4));
#if 0
    //print_pretty_board(b);
    print_board(b);
    printf("\n");
    printf("\n");
#endif
  }
  printf("final score=%d, moves=%d, max tile=%d\n",
      calculate_game_score(b, num_tile4),
      move_count,
      find_max_tile(b));
}

int main(int argc, char* argv[]) {
  if (argc == 2)
    my_random_seed = atoi(argv[1]);

  init();

  main_loop();

  return 0;
}
