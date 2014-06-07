#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <algorithm>

#define N 4
#define ROW_NUM 65536
#define ROW_MASK 0xffffll

//#define LOG_MOVES
//#define REPLAY_MOVES

#if 0
#define ALWAYS_INLINE __attribute__((always_inline))
#else
#define ALWAYS_INLINE
#endif

typedef uint64_t board_t;
typedef uint16_t row_t;

// globals
FILE* log_fp = NULL;
int flag_verbose;
int max_tile0;

// ---------------------------------------------------------
// parameters
int max_lookahead = 8;
int max_lookaheads[] = {  // TODO fine tune
  6, 6, 6, 6,
  6, 6, 5, 5,
  5, 5, 5, 5,
  4, 4, 4, 4,
};

float search_threshold = 0.006f;
int maybe_dead_threshold = 20;

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
uint16_t diff_table[65536][4];

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

inline ALWAYS_INLINE board_t do_move_2(board_t b, board_t t) {
  (void) b;
  return transpose(do_move_0(t));
}

inline ALWAYS_INLINE board_t do_move_3(board_t b, board_t t) {
  (void) b;
  return transpose(do_move_1(t));
}

inline ALWAYS_INLINE board_t do_move(board_t b, board_t t, int m) {
  switch (m) {
    case 0: return do_move_0(b);
    case 1: return do_move_1(b);
    case 2: return do_move_2(b, t);
    case 3: return do_move_3(b, t);
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

inline float eval_monotone(board_t b, board_t t) {
    float LR = std::max(
        apply_score_table(b, my_score_table_L),
        apply_score_table(b, my_score_table_R));
    float UD = std::max(
        apply_score_table(t, my_score_table_L),
        apply_score_table(t, my_score_table_R));
    return LR + UD;
}

static inline void copy_row_to_col(const uint16_t a[4], uint16_t b[16]) {
  b[0] = a[0];
  b[4] = a[1];
  b[8] = a[2];
  b[12] = a[3];
}

inline float eval_smoothness(board_t b, board_t t) {
  uint16_t diff_LR[16];
  memcpy(diff_LR+0, diff_table[(b >> 0) & ROW_MASK], sizeof(diff_table[0]));
  memcpy(diff_LR+4, diff_table[(b >> 16) & ROW_MASK], sizeof(diff_table[0]));
  memcpy(diff_LR+8, diff_table[(b >> 32) & ROW_MASK], sizeof(diff_table[0]));
  memcpy(diff_LR+12, diff_table[(b >> 48) & ROW_MASK], sizeof(diff_table[0]));
  uint16_t diff_UD[16];
  copy_row_to_col(diff_table[(t >> 0) & ROW_MASK], diff_UD+0);
  copy_row_to_col(diff_table[(t >> 16) & ROW_MASK], diff_UD+1);
  copy_row_to_col(diff_table[(t >> 32) & ROW_MASK], diff_UD+2);
  copy_row_to_col(diff_table[(t >> 48) & ROW_MASK], diff_UD+3);

  int s = 0;
  for (int i = 0; i < 16; i++)
    s += std::min(diff_LR[i], diff_UD[i]);
  return -s;
}

float eval(board_t b) {
  board_t t = transpose(b);
  float score = 0;

  int fill = 16 - count_blank(b);
  score += - fill*fill;
  score += eval_smoothness(b, t);
  score += eval_monotone(b, t);

  return score;
}

inline uint64_t rotl64 ( uint64_t x, int8_t r ) {
  return (x << r) | (x >> (64 - r));
}

inline uint64_t fmix ( uint64_t k ) {
  k ^= k >> 33;
  k *= 0xff51afd7ed558ccdull;
  k ^= k >> 33;
  k *= 0xc4ceb9fe1a85ec53ull;
  k ^= k >> 33;
  return k;
}

inline uint64_t murmur128_64_to_32(uint64_t x) {
  uint64_t seed = 0;
  uint64_t h1 = seed;
  uint64_t h2 = seed;
  uint64_t c1 = 0x87c37b91114253d5ull;
  uint64_t c2 = 0x4cf5ad432745937full;
  uint64_t k1 = x;

  k1 *= c1; k1  = rotl64(k1,31); k1 *= c2; h1 ^= k1;
  h1 ^= 8; h2 ^= 8;
  h1 += h2;
  h2 += h1;
  h1 = fmix(h1);
  h2 = fmix(h2);
  h1 += h2;
  return h1;
}

int find_max_tile(board_t b) {
  int r = 0;
  while (b) {
    r = std::max(r, (int)(b&0xf));
    b >>= 4;
  }
  return r;
}

struct local_cache1_value_t {
  board_t b;
  float s;
};

local_cache1_value_t local_cache1[16 /*max depth*/][/*key*/32][4];

inline int local_cache1_key(int tileidx, int tilev, int m) {
  if (m < 2)
    return tileidx / 4 + tilev * 4 + m * 8;
  else
    return tileidx % 4 + tilev * 4 + m * 8;
}

inline int local_cache1_get(int depth, int key, board_t b, float* s) {
  for (int i = 0; i < 4; i++)
    if (local_cache1[depth][key][i].b == b) {
      *s = local_cache1[depth][key][i].s;
      return true;
    }
  return false;
}

inline void local_cache1_set(int depth, int key, int tileidx, int m, board_t b, float s) {
  if (m < 2) {
    local_cache1[depth][key][tileidx % 4].b = b;
    local_cache1[depth][key][tileidx % 4].s = s;
  } else {
    local_cache1[depth][key][tileidx / 4].b = b;
    local_cache1[depth][key][tileidx / 4].s = s;
  }
}

float search_min(board_t b, int depth, float nodep /*, int n2, int n4*/);
float search_max(board_t b, int depth, int tileidx, int tilev, float nodep /*, int n2, int n4*/) {
  float best_score = -1e10;
  board_t t = transpose(b);
  for (int m = 0; m < 4; m++) {
    board_t b2 = do_move(b, t, m);
    if (b == b2)
      continue;
    int key = local_cache1_key(tileidx, tilev, m);

    float s;
    if (!local_cache1_get(depth, key, b2, &s)) {
      s = search_min(b2, depth - 1, nodep /*, n2, n4*/);
      local_cache1_set(depth, key, tileidx, m, b2, s);
    }

    if (s > best_score)
      best_score = s;
  }
  return best_score;
}

struct cache1_value_t {
  board_t b;
  float s;
};

// cache1 size: 50%=5643, 99%=21640, 99.9%=29024, max=39907
#define CACHE1_KEY_SIZE 65536
cache1_value_t cache1[CACHE1_KEY_SIZE];
inline int cache1_key_hash(board_t b) {
  return murmur128_64_to_32(b) % CACHE1_KEY_SIZE;
}

inline void cache1_set(int key, board_t b, int depth, float s) {
  cache1_value_t& value = cache1[key];
  value.b = (b & ~0xff) | depth;
  value.s = s;
}

inline bool cache1_get(int key, board_t b, int depth, float* s) {
  cache1_value_t& value = cache1[key];
  if ((value.b >> 8) == (b >> 8) &&
    (value.b & 0xff) >= unsigned(depth) &&
    1) {
    *s = value.s;
    return true;
  }
  return false;
}

void cache1_clear() {
}

float search_min(board_t b, int depth, float nodep /*, int n2, int n4*/) {
//  if (depth == 0 || (n4 > 0 || n2 > 4))
  if (depth == 0 || nodep < search_threshold)
    return eval(b);

  int key = cache1_key_hash(b);
  float s;
  if (cache1_get(key, b, depth, &s))
    return s;

  int blank = count_blank(b);

  float nodep2 = nodep / blank;
  float score = 0;
  board_t tile = 1;
  board_t tmp = b;
  int idx = 0;
//  bool with_tile4 = (n4 == 0 && n2 <= 2);
  bool with_tile4 = true;  // todo
  while (tile) {
    if ((tmp & 0xf) == 0) {
      if (with_tile4) {
        score += search_max(b | tile, depth, idx, 0, nodep2 * 0.9f /*, n2+1, n4*/) * 0.9f;
        score += search_max(b | tile << 1, depth, idx, 1, nodep2 * 0.1f /*, n2, n4+1*/) * 0.1f;
      } else {
        score += search_max(b | tile, depth, idx, 0, nodep2 * 0.9f /*, n2+1, n4*/);
      }
    }
    tile <<= 4;
    tmp >>= 4;
    idx++;
  }
  float result = score / blank;
  cache1_set(key, b, depth, result);
  return result;
}

struct cache2_value_t {
  board_t b;
};
// With depth_to_dead_of_blank heuristic, 64k is enough for maybe_dead_threshold=24
#define CACHE2_KEY_SIZE 65536
cache2_value_t cache2[CACHE2_KEY_SIZE];

inline int cache2_key_hash(board_t b) {
  return murmur128_64_to_32(b) % CACHE2_KEY_SIZE;
}

inline void cache2_set(int key, board_t b, int depth) {
  cache2_value_t& value = cache2[key];
  value.b = (b & ~0xff) | depth;
}

inline bool cache2_get(int key, board_t b, int depth) {
  cache2_value_t& value = cache2[key];
  if ((value.b >> 8) == (b >> 8))
    return (value.b & 0xff) >= unsigned(depth);
  return false;
}

void cache2_clear() {
}

// data collected from 1200 runs
int depth_to_dead_of_blank[17] = {
  0,
  1, 2, 3, 6,
};

bool maybe_dead_maxnode(board_t b, int depth);
bool maybe_dead_minnode(board_t b, int depth) {
  if (depth <= 0)
    return false;

  int blank = count_blank(b);
#if 1
  // experimental data
  if (blank >= 5)
    return false;
  if (depth_to_dead_of_blank[blank] > depth)
    return false;
#else
  // theoretical bound
  if (blank >= depth+1) return false;
#endif

  int key = cache2_key_hash(b);
  if (cache2_get(key, b, depth))
    return false;

  board_t tmp = b;
  board_t tile_2 = 1;
  while (tile_2) {
    if ((tmp & 0xf) == 0) {
      if (maybe_dead_maxnode(b | tile_2, depth)) {
        return true;
      }
    }
    tmp >>= 4;
    tile_2 <<= 4;
  }

  cache2_set(key, b, depth);
  return false;
}

bool maybe_dead_maxnode(board_t b, int depth) {
  board_t t = transpose(b);
  for(int m = 0; m < 4; m++) {
    board_t b2 = do_move(b, t, m);
    if (b2 == b)
      continue;

    if (!maybe_dead_minnode(b2, depth-1))
      return false;
  }
  return true;
}

char move_str[] = "RLUD";
int root_search_move(board_t b) {
  cache1_clear();
  float best_score = -1e10-1;
  int best_move = 0;

  max_tile0 = find_max_tile(b);
  int badmove[4] = {0};
  int nbadmove = 0;
  cache2_clear();
  board_t t = transpose(b);
#if 1
  for(int m = 0; m < 4; m++) {
    board_t b2 = do_move(b, t, m);
    int threshold = maybe_dead_threshold;
    if (b == b2 || maybe_dead_minnode(b2, threshold)) {
      badmove[m] = 1;
      nbadmove++;
    }
  }
#endif

  for (int m = 0; m < 4; m++) {
    if (nbadmove != 4 && badmove[m])
      continue;

    board_t b2 = do_move(b, t, m);
    if (b == b2)
      continue;

    int lookahead = max_lookahead;
    //lookahead = max_lookaheads[count_blank(b2)];
    float s = search_min(b2, lookahead - 1, 1.0 /*, 0, 0*/);
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

#if 1
    {
      int d[4];
      int x[4] = { 1 << urow[0], 1<<urow[1], 1<<urow[2], 1<<urow[3]};
      d[0] = abs(x[1]-x[0]);
      d[1] = std::min(abs(x[0]-x[1]),abs(x[2]-x[1]));
      d[2] = std::min(abs(x[1]-x[2]),abs(x[3]-x[2]));
      d[3] = abs(x[2]-x[3]);
      diff_table[row][0] = d[0];
      diff_table[row][1] = d[1];
      diff_table[row][2] = d[2];
      diff_table[row][3] = d[3];
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
  board_t t = transpose(b);
  for (int m = 0; m < 4; m++) {
    if (b != do_move(b, t, m))
      return true;
  }
  return false;
}

void main_loop() {
  double t0 = now();
  char log_filename[1024];
  sprintf(log_filename, "log/%d.txt", my_random_seed);

  board_t b = 0;
  int num_tile4 = 0;
  b = random_tile(b, &num_tile4);
  b = random_tile(b, &num_tile4);

#if defined(REPLAY_MOVES)
  log_fp = fopen(log_filename, "r");
#elif defined(LOG_MOVES)
  log_fp = fopen(log_filename, "w");
#endif

  int move_count = 0;
  if (flag_verbose)
    print_pretty_board(b);
  while (is_can_move(b)) {
    int m = 0;
#if defined(REPLAY_MOVES)
    if (!log_fp || fscanf(log_fp, "move %d\n", &m) != 1) {
      printf("bad replay\n");
      break;
    }
#elif defined(LOG_MOVES)
    m = root_search_move(b);
    if (log_fp) fprintf(log_fp, "move %d\n", m);
#else
    m = root_search_move(b);
#endif
    board_t b2 = do_move(b, transpose(b), m);
    if (b == b2) {
      printf("bad move\n");
      break;
    }
    move_count++;
    my_random_seed ^= murmur128_64_to_32(b);
    b = random_tile(b2, &num_tile4);
    if (flag_verbose) {
      printf("step %d, move %c, score=%d\n",
          move_count, move_str[m],
          calculate_game_score(b, num_tile4));
      print_pretty_board(b);
    }
#if 0
    print_board(b);
    printf("\n");
    printf("\n");
#endif
  }
  double t1 = now();
  printf("time=%f, final score=%d, moves=%d, max tile=%d\n",
      t1-t0,
      calculate_game_score(b, num_tile4),
      move_count,
      find_max_tile(b));
#if defined(REPLAY_MOVES) || defined(LOG_MOVES)
  fclose(log_fp);
#endif
}

int main(int argc, char* argv[]) {
  int opt;
  while ((opt = getopt(argc, argv, "vs:p:")) != -1) {
    switch (opt) {
      case 'v':
        flag_verbose = 1;
        break;
      case 's':
        my_random_seed = atoi(optarg);
        break;
      case 'p':
        if (sscanf(optarg, "%d,%f,%d",
            &max_lookahead,
            &search_threshold,
            &maybe_dead_threshold) != 3) {
          printf("bad arg -p %s", optarg);
          return -1;
        }
        break;
      default: /* '?' */
        printf("unknown option '%c'\n", opt);
        break;
    }
  }

  init();

  main_loop();

  return 0;
}
