#ifndef BOT_OPT_H
#define BOT_OPT_H
#include "common.h"

typedef void (*init_bot_func_t)(void);
typedef int (*root_search_move_func_t)(board_t);
typedef void (*cache1_clear_func_t)(void);
typedef int* max_lookahead_ptr_t;
typedef int* maybe_dead_threshold_ptr_t;
typedef float* search_threshold_ptr_t;

extern init_bot_func_t init_bot_func;
extern root_search_move_func_t root_search_move_func;
extern cache1_clear_func_t cache1_clear_func;
extern max_lookahead_ptr_t max_lookahead_ptr;
extern maybe_dead_threshold_ptr_t maybe_dead_threshold_ptr;
extern search_threshold_ptr_t search_threshold_ptr;

extern void load_code();
#endif
