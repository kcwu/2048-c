#ifndef BOT_OPT_H
#define BOT_OPT_H
#include "bot.h"

typedef void (*init_bot_func_t)(void);
typedef int (*root_search_move_func_t)(board_t);
typedef int* max_lookahead_ptr_t;

extern init_bot_func_t init_bot_func;
extern root_search_move_func_t root_search_move_func;
extern max_lookahead_ptr_t max_lookahead_ptr;

extern void load_code();
#endif
