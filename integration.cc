#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#define INTEGRATION
#include "2048.h"
#include "bot.h"
#include "util.h"

//#define PRINT

void (*init_bot_func)(void);
int (*root_search_move_func)(board_t);
int* max_lookahead_ptr;

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
    isGameOver = false;
    while(!isGameOver){

      dir_e dir = INVALID;
      Grid grid;
      myGame.getCurrentGrid(grid);
      board_t b = convert_grid_to_board(grid);

      int m = root_search_move(b);
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

      myGame.insertDirection(dir);
      isGameOver = myGame.isGameOver();

#ifdef PRINT
      myGame.printGrid(35,2);
      gotoXY(0,15); 
      printf("  Round:    %d      \n", i+1);
      printf("  Score:    %d      \n", myGame.getScore());
      printf("  Max Score: %d      \n", myGame.getMaxScore());
#endif

    }
#ifdef PRINT
    myGame.printGrid(35,2);
#endif
    if(i < n - 1)  myGame.reset();
  }
}

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

#if 1
void load_code() {
  int fd;
    off_t size;
    void* p;

  fd = open ("bot_opt.bin", O_RDONLY);
  size = lseek (fd, 0, SEEK_END);
  lseek (fd, 0, SEEK_SET);
  // TODO find address
  p = mmap ((void*)0x10000000, size, PROT_EXEC | PROT_READ | PROT_WRITE,
      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  read (fd, p, size);
  close (fd);

  FILE* fp = fopen("bot_opt.info", "r");
  char line[1024];
  while (fgets(line, sizeof(line), fp)) {
    int addr;
    //printf("line %s\n", line);
    if (sscanf(line, ".text init_bot %d", &addr) == 1) {
      init_bot_func = (void(*)(void))((char*)p + addr);
    }
    if (sscanf(line, ".text root_search_move %d", &addr) == 1) {
      root_search_move_func = (int(*)(board_t))((char*)p + addr);
    }
    if (sscanf(line, ".data max_lookahead %d", &addr) == 1) {
      max_lookahead_ptr = (int*)((char*)p + addr);
    }
    if (sscanf(line, "reloc %d", &addr) == 1) {
      *(int64_t*)((char*)p + addr) += (int64_t)p;
    }
  }
  fclose(fp);
}
#endif

void init() {
#if 1
  load_code();
#endif
  if (max_lookahead_ptr == NULL ||
      init_bot_func == NULL ||
      root_search_move_func == NULL) {
    printf("code not loaded\n");
    max_lookahead_ptr = &max_lookahead;
    init_bot_func = init_bot;
    root_search_move_func = root_search_move;
  }
  init_bot_func();

  int n_sample = sizeof(sample_boards)/sizeof(sample_boards[0]);

  int to_lookahead = 6;
  for (int lookahead = to_lookahead; lookahead <= 8; lookahead++) {
    double t0 = now();
    *max_lookahead_ptr = lookahead;
    for (int i = 0 ; i < n_sample; i++) {
      root_search_move_func(sample_boards[i]);
    }
    double t1 = now();
    printf("%d %f\n", lookahead, t1-t0);
    if (t1-t0 < 0.9) {
      to_lookahead = lookahead;
    } else {
      break;
    }
  }
  *max_lookahead_ptr = to_lookahead;

}

int main(int argc, char* argv[]){
  int opt;
  int n = 100;
  while ((opt = getopt(argc, argv, "n:")) != -1) {
    switch (opt) {
      case 'n':
        n = atoi(optarg);
        break;
    }
  }

  init();

  main_loop(n);

  return 0;
}