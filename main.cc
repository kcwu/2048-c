#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "bot.h"
#include "util.h"

//#define LOG_MOVES
//#define REPLAY_MOVES

// globals
int flag_verbose;
FILE* log_fp = NULL;

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

  int blank = count_blank_ex(b);
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
  board_t t = transpose_ex(b);
  for (int m = 0; m < 4; m++) {
    if (b != do_move_ex(b, t, m))
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
    board_t b2 = do_move_ex(b, transpose_ex(b), m);
    if (b == b2) {
      printf("bad move\n");
      break;
    }
    move_count++;
    my_random_seed ^= murmur3_simplified_ex(b);
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
      find_max_tile_ex(b));
#if defined(REPLAY_MOVES) || defined(LOG_MOVES)
  fclose(log_fp);
#endif

#if 0
  extern int64_t min_scores[17];
  for (int i = 0; i <= 16; i++)
    printf("min score %d: %ld\n", i, min_scores[i]);
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
            &maybe_dead_threshold) == 3) {
        } else if (sscanf(optarg, "reverse=%f,%f,%f,%f,%f",
              &para_reverse_weight,
              &para_reverse,
              &para_reverse_4,
              &para_reverse_8,
              &para_reverse_12)==5) {
        } else if (sscanf(optarg, "equal=%f",
              &para_equal)==1) {
        } else if (sscanf(optarg, "inc=%f,%f,%f,%f",
              &para_inc_0,
              &para_inc_1,
              &para_inc_2,
              &para_inc_3) == 4) {
        } else if (sscanf(optarg, "smooth=%f,%f,%f,%f",
              &para_smooth,
              &para_smooth_4,
              &para_smooth_8,
              &para_smooth_12) == 4) {
        } else if (sscanf(optarg, "blank=%f,%f,%f",
              &para_blank_1,
              &para_blank_2,
              &para_blank_3) == 3) {
        } else {
          printf("bad arg -p %s", optarg);
          return -1;
        }
        break;
      default: /* '?' */
        printf("unknown option '%c'\n", opt);
        break;
    }
  }

  init_bot();

  main_loop();

  return 0;
}
