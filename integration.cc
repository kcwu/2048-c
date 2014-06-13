#include <signal.h>
#include <setjmp.h>

#include "common.h"
#include "2048.h"
#include "util.h"
#include "bot_opt.h"

//#define PRINT

init_bot_func_t init_bot_func;
root_search_move_func_t root_search_move_func;
cache1_clear_func_t cache1_clear_func;
max_lookahead_ptr_t max_lookahead_ptr;
maybe_dead_threshold_ptr_t maybe_dead_threshold_ptr;
search_threshold_ptr_t search_threshold_ptr;

int min_xi = 3;
int init_xi = 8;
int max_xi = 15;
bool emergency;
int xi;
int xis[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
int flag_verbose;

board_t convert_grid_to_board(Grid g) {
  board_t b = 0;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      int x = g(i, j);
      board_t c = 0;
      if (x != EMPTY) {
        while (x) {
          x >>= 1;
          c++;
        }
        c--;
        if (c == 16)  // HACK for 65536
          c--;
      }
      b |= c << (4*(i*4+3-j));
    }
  }
  return b;
}

int move_count_for_threshold[100];
double time_for_threshold[100];
double avg_time_for_threshold[100];
double total_time;
int total_move;
double time_limit = 0.01;

static int find_max_tile(board_t b) {
  int r = 0;
  while (b) {
    r = std::max(r, (int)(b&0xf));
    b >>= 4;
  }
  return r;
}
int effective_max_depth(board_t b) {
  int m = find_max_tile(b);
  if (m >= 14 && *max_lookahead_ptr >= 13)
    return (*max_lookahead_ptr);
  return std::min(m - 2, *max_lookahead_ptr);
}

void time_record(int move_count, double t, board_t b) {
  total_time += t;
  total_move += move_count;
  if (flag_verbose) {
    printf("move %d, t %f, avg %f\n", move_count, t, t/move_count*1000);
    printf("over all %d, t %f, avg %f\n", total_move, total_time, total_time/total_move*1000);
  }
  int exi = effective_max_depth(b);
  move_count_for_threshold[exi] += move_count;
  time_for_threshold[exi] += t;
  avg_time_for_threshold[exi] = time_for_threshold[exi] / move_count_for_threshold[exi];

  if (flag_verbose) {
    printf("\t");
    for (int i = min_xi; i <= max_xi; i++) {
      printf("%d:%.2f, ",
          xis[i], avg_time_for_threshold[i]*1000);
    }
    printf("\n");
  }
}

void time_control(int n, int idx) {
  if (flag_verbose)
    printf("%d/%d\n", idx, n);
  if (idx == 0) {
    // use default
    xi = init_xi;
    if (flag_verbose)
      printf("use %d\n", xis[xi]);
    *max_lookahead_ptr = xis[xi];
    return;
  }

  int remain = n - idx;
  int old_xi = xi;
  xi = init_xi;

  double worst_factor = 1.0 * (remain + 3) / remain;

  if (!emergency) {
    for (int i = min_xi; i <= max_xi; i++) {
      // don't explore unknown if remain too less
      if (remain < n/10 &&
          i > min_xi &&
          move_count_for_threshold[i-1] > 0 &&
          move_count_for_threshold[i] == 0)
        break;

      bool enough_try = move_count_for_threshold[i] > total_move/idx* (idx < 15? 5:3);
      double estimate = total_time/total_move*idx + avg_time_for_threshold[i]  * worst_factor * remain;

      if (!enough_try || estimate < time_limit * n)
        xi = i;

      if (estimate >= time_limit * n)
        break;

      // don't inc twice
      if (i > min_xi &&
          move_count_for_threshold[i-1] > 0 &&
          move_count_for_threshold[i] == 0) {
        break;
      }
    }
  }

  if (old_xi != xi) {
    if (flag_verbose)
      printf("switch from %d to %d\n", xis[old_xi], xis[xi]);
    *max_lookahead_ptr = xis[xi];
  } else {
    if (flag_verbose)
      printf("still use %d\n", xis[xi]);
  }
}

void main_loop(int n) {
  Game myGame;

  bool isGameOver;

#ifdef PRINT
  (void)system("clear");
  gotoXY(5,0);
  std::cout<<"Previous";
  gotoXY(35,0);
  std::cout<<"Current";
  myGame.printGrid(35,2);
#endif

  for(int i = 0; i < n; i++){
    double t1 = now();
    time_control(n, i);
    isGameOver = false;
    int move_count = 0;
    while(!isGameOver){

      dir_e dir = INVALID;
      Grid grid;
      myGame.getCurrentGrid(grid);
      board_t b = convert_grid_to_board(grid);
      //print_pretty_board(b);

      int m = root_search_move_func(b);
      switch (move_str[m]) {
        case 'L': dir = LEFT; break;
        case 'R': dir = RIGHT; break;
        case 'U': dir = UP; break;
        case 'D': dir = DOWN; break;
      }


#ifdef PRINT
      gotoXY(5,10);
      myGame.printGrid(5,2);
#endif

      move_count++;
      if (!myGame.insertDirection(dir)) {
        printf("invalid move??\n");
        break;
      }
      isGameOver = myGame.isGameOver();

#ifdef PRINT
      myGame.printGrid(35,2);
      gotoXY(0,15); 
      printf("  Round:    %d      \n", i+1);
      printf("  Score:    %d      \n", myGame.getScore());
      printf("  Max Score: %d      \n", myGame.getMaxScore());
#endif
    }
    if (flag_verbose)
      printf("  Score:    %d      \n", myGame.getScore());
#ifdef PRINT
    myGame.printGrid(35,2);
#endif
    double t2 = now();
    {
      Grid grid;
      myGame.getCurrentGrid(grid);
      board_t b = convert_grid_to_board(grid);
      time_record(move_count, t2-t1, b);
    }
    if(i < n - 1)  myGame.reset();
  }
}

// put bot.h after mainloop to avoid call main api directly
#define INTEGRATION
#include "bot.h"

board_t sample_boards[] = {
  0x6621533332211111ull, 0x8643553333101100ull, 0x9653531032001001ull, 0x9601843032232112ull,
  0xa122544022013100ull, 0xa710730044212200ull, 0xa710852163334221ull, 0xa752954323321211ull,
  0xa841972253113210ull, 0xa942296435421201ull, 0x012215218531b532ull, 0x411053109410b430ull,
  0x331114219663b733ull, 0x121155229763b812ull, 0x01124321a532b753ull, 0x22005421a732b831ull,
  0x11232312a653b953ull, 0x32016510a850b911ull, 0x210041005211c532ull, 0x222143327411c731ull,
  0x001021038732c742ull, 0x210023107642c953ull, 0x123112328753c943ull, 0x310144206631ca22ull,
  0x030122218544ca61ull, 0x021207538754ca14ull, 0x142133439831ca22ull, 0x121321419880ca10ull,
  0x131145216542cb63ull, 0x310025118742cb43ull, 0x014120629621cb42ull, 0x110163319843cb53ull,
  0x12004331a621cb32ull, 0x10104110a832cb54ull, 0x21111222a884cb31ull, 0x11217642a954cb01ull,
  0x53228732a921cb51ull, 0x32203431b731cb12ull, 0x111222250448106dull, 0x012101231231269dull,
  0x111123334477119dull, 0x001201710278149dull, 0x10210232236734adull, 0x00121023357834adull,
  0x12114632016901adull, 0x20012356238923adull, 0x12320448258924adull, 0x00131124024823bdull,
  0x10001711147812bdull, 0x10130146227914bdull, 0x00251147128924bdull, 0x30001341137a21bdull,
  0x02240345158a25bdull, 0x22214234149a15bdull, 0x22413338149a12bdull, 0x32111278279a31bdull,
  0x32242335144702cdull, 0x01231334047814cdull, 0x23212234346935cdull, 0x12231334248916cdull,
  0x10022321345a34cdull, 0x11222234148a11cdull, 0x21121667028a20cdull, 0x14122456159a26cdull,
  0x00232425279a38cdull, 0x13341145226b11cdull, 0x11210026138b35cdull, 0x22232445119b00cdull,
  0x33332477029b01cdull, 0x11221778029b00cdull, 0x0023126712ab24cdull, 0x0322147805ab11cdull,
  0x2234066912ab00cdull, 0x1265138922ab11cdull, 0x2201125113c323cdull, 0x001300121267246eull,
  0x112323441268017eull, 0x001001223559237eull, 0x000211232179158eull, 0x21231234135a026eull,
  0x20010233016a058eull, 0x01010132225a439eull, 0x01200033128a239eull, 0x23212244368a179eull,
  0x11002312234b457eull, 0x12440455216b128eull, 0x12232445226b019eull, 0x20102320248b469eull,
  0x00140125221b34aeull, 0x10211132228b14aeull, 0x13111265278b34aeull, 0x11330167239b01aeull,
  0x02340278139b15aeull, 0x31121324045c106eull, 0x13113235334c158eull, 0x11210132135c439eull,
  0x00001022248c349eull, 0x13201800148c219eull, 0x00111232337c56aeull, 0x02111322258c57aeull,
};

jmp_buf saved_state;

void handle_signal(int sig) {
  (void)sig;
  longjmp(saved_state, 1);
}

double test_run() {
  int n_sample = sizeof(sample_boards)/sizeof(sample_boards[0]);
    double t0 = now();
    cache1_clear_func();
    for (int i = 0; i < n_sample; i++) {
      root_search_move_func(sample_boards[i]);
    }
    double t1 = now();
    return (t1 - t0) / n_sample;
}

void init() {
  signal(SIGSEGV, handle_signal);
  signal(SIGBUS, handle_signal);
  signal(SIGILL, handle_signal);
  signal(SIGALRM, handle_signal);
  if (setjmp(saved_state) == 0)
    load_code();
  else {
    printf("something wrong, recovered\n");
    init_bot_func = NULL;
  }

  if (max_lookahead_ptr == NULL ||
      init_bot_func == NULL ||
      root_search_move_func == NULL ||
      maybe_dead_threshold_ptr == NULL ||
      search_threshold_ptr == NULL ||
      cache1_clear_func == NULL) {
    printf("code not loaded\n");
    max_lookahead_ptr = &max_lookahead;
    init_bot_func = init_bot;
    root_search_move_func = root_search_move;
    maybe_dead_threshold_ptr = &maybe_dead_threshold;
    search_threshold_ptr = &search_threshold;
    cache1_clear_func = &cache1_clear;
  }
  alarm(5);
  init_bot_func();
  alarm(0);


  // init time control & test run loaded code
  *max_lookahead_ptr = 8;
  float mid = 0.1;
  float left = mid;
  float right = mid;
  double magic = 0.5*time_limit;
  float to_threshold = mid;

  while (1) {
    *search_threshold_ptr = mid;
    double t = test_run();
    if (flag_verbose)
      printf("threshold=%f, t=%f %c\n", mid, t*1000, t<magic?'*':' ');
    if (t < magic) {
      to_threshold = mid;
      left = mid;
      right = mid / 2;
      mid /= 2;
    } else
      break;
  }

  do {
    mid = (left+right)/2;
    *search_threshold_ptr = mid;
    double t = test_run();
    if (flag_verbose)
      printf("threshold=%f, t=%f %c\n", mid, t*1000, t<magic?'*':' ');

    if (t < magic) {
      left = mid;
      to_threshold = mid;
    } else
      right = mid;
  } while (left-right > 0.00001);

  *search_threshold_ptr = to_threshold;

  if (flag_verbose)
    printf("search_threshold=%f\n", mid);

  signal(SIGSEGV, SIG_DFL);
  signal(SIGBUS, SIG_DFL);
  signal(SIGILL, SIG_DFL);
  signal(SIGALRM, SIG_DFL);
}

int main(int argc, char* argv[]){
  int opt;
  int n = 100;
  while ((opt = getopt(argc, argv, "n:t:v")) != -1) {
    switch (opt) {
      case 'n':
        n = atoi(optarg);
        break;
      case 't':
        time_limit = atof(optarg);
        break;
      case 'v':
        flag_verbose = 1;
    }
  }

  init();

  main_loop(n);

  return 0;
}
