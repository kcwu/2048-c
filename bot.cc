#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <algorithm>
#include <math.h>

#include "bot.h"

#if 0
#define ALWAYS_INLINE __attribute__((always_inline))
#else
#define ALWAYS_INLINE
#endif

// globals
int max_tile0;

// ---------------------------------------------------------
// parameters
int max_lookahead = 7;

float search_threshold = 0.001f;
int maybe_dead_threshold = 25;

#if 1
typedef int64_t score_t;
// at least 18 for score, 8 for calculation, 10 for fraction?
#define SCORE_BASE (1ll << 30)
#define SCORE(v) (score_t)((v) * (SCORE_BASE))
#define MIN_SCORE SCORE(0)
#else
typedef double score_t;
#define SCORE_BASE 1.0
#define MIN_SCORE -1e10
#define SCORE(v) (score_t)(v)
#endif

float para_reverse_weight = 1.525;
float para_reverse = 2.247;
float para_reverse_4 = 1.0;
float para_reverse_8 = 1.0;
float para_reverse_12 = 1.0;
float para_equal = 0.0;
float para_inc_0 = 0;
float para_inc_1 = 0;
float para_inc_2 = 3.486;
float para_inc_3 = 0;
float para_smooth = 1.971;
float para_smooth_4 = 1.0;
float para_smooth_8 = 1.0;
float para_smooth_12 = 1.0;
float para_blank_1 = 0.0;
float para_blank_2 = 1.0;
float para_blank_3 = 0.0;

static const float SCORE_LOST_PENALTY = 30000.0f;
static const float SCORE_MONOTONICITY_WEIGHT = 1.0f;
static const float SCORE_MERGES_WEIGHT = 10.0f;
static const float SCORE_EMPTY_WEIGHT = 10.0f;

// ---------------------------------------------------------
// Functions to implement solver. Must be highly OPTIMIZED

row_t row_left_table[ROW_NUM];
row_t row_right_table[ROW_NUM];
score_t my_score_table[65536];
score_t min_scores[17];

inline ALWAYS_INLINE board_t transpose(board_t x) {
  board_t t;
  t = (x ^ (x >> 12)) & 0x0000f0f00000f0f0ull;
  x ^= t ^ (t << 12);
  t = (x ^ (x >> 24)) & 0x00000000ff00ff00ull;
  x ^= t ^ (t << 24);
  return x;
}
board_t transpose_ex(board_t x) {
  return transpose(x);
}

inline ALWAYS_INLINE int count_blank(board_t b) {
  b = ~b;
  b &= b >> 2;
  b &= b >> 1;
  b &= 0x1111111111111111ull;
  b = (b * 0x1111111111111111ull) >> 60;
  return b;
}
int count_blank_ex(board_t b) {
  return count_blank(b);
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

inline ALWAYS_INLINE board_t do_move(board_t b, board_t t, int m)  {
  switch (m) {
    case 0: return do_move_0(b);
    case 1: return do_move_1(b);
    case 2: return do_move_2(b, t);
    case 3: return do_move_3(b, t);
  }
  return 0;
}

board_t do_move_ex(board_t b, board_t t, int m)  {
  return do_move(b, t, m);
}

score_t apply_score_table(board_t b, score_t* table) {
  return
    table[(b >>  0) & ROW_MASK] +
    table[(b >> 16) & ROW_MASK] +
    table[(b >> 32) & ROW_MASK] +
    table[(b >> 48) & ROW_MASK];
}

score_t eval_xificurk(board_t b, board_t t) {
    return
        apply_score_table(b, my_score_table) + 
        apply_score_table(t, my_score_table) +
        SCORE(SCORE_LOST_PENALTY);
}

score_t eval(board_t b) {
  board_t t = transpose(b);
  score_t score = 0;

  score += eval_xificurk(b, t);

  return score;
}

inline uint64_t rotl64 ( uint64_t x, int8_t r ) {
  return (x << r) | (x >> (64 - r));
}

// simplified from MurmurHash3_x64_128's fmix
inline uint32_t murmur3_simplified(uint64_t x) {
  x *= 0xff51afd7ed558ccdull;
  x ^= x >> 33;
  x *= 0xc4ceb9fe1a85ec53ull;
  x ^= x >> 33;
  return x;
}
uint32_t murmur3_simplified_ex(uint64_t x) {
  return murmur3_simplified(x);
}

inline int find_max_tile(board_t b) {
  int r = 0;
  while (b) {
    r = std::max(r, (int)(b&0xf));
    b >>= 4;
  }
  return r;
}
int find_max_tile_ex(board_t b) {
  return find_max_tile(b);
}

struct local_cache1_value_t {
  board_t b;
  score_t s;
};

local_cache1_value_t local_cache1[16 /*max depth*/][/*key*/32][4];

inline int local_cache1_key(int tileidx, int tilev, int m) {
  if (m < 2)
    return tileidx / 4 + tilev * 4 + m * 8;
  else
    return tileidx % 4 + tilev * 4 + m * 8;
}

inline int local_cache1_get(int depth, int key, board_t b, score_t* s) {
  for (int i = 0; i < 4; i++)
    if (local_cache1[depth][key][i].b == b) {
      *s = local_cache1[depth][key][i].s;
      return true;
    }
  return false;
}

inline void local_cache1_set(int depth, int key, int tileidx, int m, board_t b, score_t s) {
  if (m < 2) {
    local_cache1[depth][key][tileidx % 4].b = b;
    local_cache1[depth][key][tileidx % 4].s = s;
  } else {
    local_cache1[depth][key][tileidx / 4].b = b;
    local_cache1[depth][key][tileidx / 4].s = s;
  }
}

score_t search_min(board_t b, int depth, float nodep /*, int n2, int n4*/);
score_t search_max(board_t b, int depth, int tileidx, int tilev, float nodep /*, int n2, int n4*/) {
  score_t best_score = min_scores[max_tile0];
  board_t t = transpose(b);
  for (int m = 0; m < 4; m++) {
    board_t b2 = do_move(b, t, m);
    if (b == b2)
      continue;
    int key = local_cache1_key(tileidx, tilev, m);

    score_t s;
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
  float p;
  score_t s;
};

// cache1 size: 50%=5643, 99%=21640, 99.9%=29024, max=39907
#define CACHE1_KEY_SIZE 65536
cache1_value_t cache1[CACHE1_KEY_SIZE];
inline int cache1_key_hash(board_t b) {
  return murmur3_simplified(b) % CACHE1_KEY_SIZE;
}

inline void cache1_set(int key, board_t b, int depth, float nodep, score_t s) {
  cache1_value_t& value = cache1[key];
  value.b = (b & ~0xff) | depth;
  value.p = nodep;
  value.s = s;
}

inline bool cache1_get(int key, board_t b, int depth, float nodep, score_t* s) {
  cache1_value_t& value = cache1[key];
  if ((value.b >> 8) == (b >> 8) &&
    (value.b & 0xff) >= unsigned(depth) &&
    value.p >= nodep &&
    1) {
    *s = value.s;
    return true;
  }
  return false;
}

void cache1_clear() {
  for (int i = 0; i < CACHE1_KEY_SIZE; i++)
    cache1[i].b = 0;
  //memset(&cache1, 0, sizeof(cache1));
}

score_t search_min(board_t b, int depth, float nodep /*, int n2, int n4*/) {
//  if (depth == 0 || (n4 > 0 || n2 > 4))
  if (depth == 0 || nodep < search_threshold)
    return eval(b);

  int key = cache1_key_hash(b);
  score_t s;
  if (cache1_get(key, b, depth, nodep, &s))
    return s;

  int blank = count_blank(b);

  float nodep2 = nodep / blank;
  score_t score = 0;
  board_t tile = 1;
  board_t tmp = b;
  int idx = 0;
//  bool with_tile4 = (n4 == 0 && n2 <= 2);
  bool with_tile4 = true;  // todo
  while (tile) {
    if ((tmp & 0xf) == 0) {
      if (with_tile4) {
        score += search_max(b | tile, depth, idx, 0, nodep2 * 0.9f /*, n2+1, n4*/) * 9;//0.9f;
        score += search_max(b | tile << 1, depth, idx, 1, nodep2 * 0.1f /*, n2, n4+1*/) * 1;//0.1f;
      } else {
        score += search_max(b | tile, depth, idx, 0, nodep2 * 0.9f /*, n2+1, n4*/) * 10;
      }
    }
    tile <<= 4;
    tmp >>= 4;
    idx++;
  }
  score_t result = score / blank / 10;
  cache1_set(key, b, depth, nodep, result);
  return result;
}

struct cache2_value_t {
  board_t b;
};
// With depth_to_dead_of_blank heuristic, 64k is enough for maybe_dead_threshold=24
#define CACHE2_KEY_SIZE 65536
cache2_value_t cache2[CACHE2_KEY_SIZE];

inline int cache2_key_hash(board_t b) {
  return murmur3_simplified(b) % CACHE2_KEY_SIZE;
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

int count_diff_tile(board_t b) {
  int mask = 0;
  while (b) {
    mask |= 1 << (b & 0xf);
    b >>= 4;
  }
  int count = 0;
  for (int i = 1; i < 16; i++)
    if (mask >> i & 1) {
      count++;
    }
  return count;
}

int root_search_move(board_t b) {
  score_t best_score = min_scores[max_tile0]-1;
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
    score_t s = search_min(b2, lookahead - 1, 1.0 /*, 0, 0*/);
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
#if 0
  while (oc < N)
    row[oc++] = 0;
#else
  // to avoid gcc 4.8 to use memset()
  if (oc == 1)
    row[1] = row[2] = row[3] = 0;
  if (oc == 2)
    row[2] = row[3] = 0;
  if (oc == 3)
    row[3] = 0;
#endif
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
      int empty = 0;
      int merges = 0;

      int prev = 0;
      int counter = 0;
      for (int i = 0; i < 4; ++i) {
        int rank = urow[i];
        if (rank == 0) {
          empty++;
        } else {
          if (prev == rank) {
            counter++;
          } else if (counter > 0) {
            merges += 1 + counter;
            counter = 0;
          }
          prev = rank;
        }
      }
      if (counter > 0) {
        merges += 1 + counter;
      }

      int monotonicity_left = 0;
      int monotonicity_right = 0;
      for (int i = 1; i < 4; ++i) {
        if (urow[i-1] > urow[i]) {
          monotonicity_left += pow(urow[i-1], 3) - pow(urow[i], 3);
        } else {
          monotonicity_right += pow(urow[i], 3) - pow(urow[i-1], 3);
        }
      }

      my_score_table[row] = SCORE(SCORE_EMPTY_WEIGHT * empty + SCORE_MERGES_WEIGHT * merges -
        SCORE_MONOTONICITY_WEIGHT * std::min(monotonicity_left, monotonicity_right));
    }
#endif

  }

}

void init_bot() {
  build_move_table();
  build_eval_table();
}

