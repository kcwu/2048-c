#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#include "bot.h"
#include "util.h"

char move_str[5] = "RLUD";

// ---------------------------------------------------------
// Utility functions for testing and debugging
// Shoudn't be used in final solver code
int my_random_seed = 0;
int my_random() {
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
