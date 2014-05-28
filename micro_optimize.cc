#pragma GCC optimize (3)
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>


#define GCC_VERSION (__GNUC__ * 10000 \
    + __GNUC_MINOR__ * 100 \
    + __GNUC_PATCHLEVEL__)
static int seed = 0;

int myrand() {   
    // rand formula from FreeBSD rand.c 
    seed = seed * 1103515245 + 12345;
    return seed % ((unsigned long)RAND_MAX + 1);
}   

uint64_t transpose0(uint64_t b) {
  uint64_t r = 0;

  int shifts[] = {
    0, 3, 6, 9,
    -3, 0, 3, 6,
    -6, -3, 0, 3,
    -9, -6, -3, 0
  };
  uint64_t mask = 0xf;
  for (int i = 0; i < 16; i++) {
    int s = shifts[i]*4;
    if(s > 0) {
      r |= (b & mask) << s;
    } else {
      r |= (b & mask) >> (-s);
    }
    mask <<= 4;
  }
  return r;
}

uint64_t transpose1(uint64_t x) {
  // 19 op
  uint64_t a0 = x & 0xf0000f0000f0000full;
  uint64_t a3 = x & 0x0000f0000f0000f0ull;
  uint64_t a6 = x & 0x00000000f0000f00ull;
  uint64_t a9 = x & 0x000000000000f000ull;
  uint64_t b3 = x & 0x0f0000f0000f0000ull;
  uint64_t b6 = x & 0x00f0000f00000000ull;
  uint64_t b9 = x & 0x000f000000000000ull;
  return a0 | a3 << 12 | a6 << 24 | a9 << 36 | b3 >> 12 | b6 >> 24 | b9 >> 36;
}

uint64_t transpose2(uint64_t x) {
  // 18 op
  uint64_t a0 = x & 0xf0000f0000f0000full;
  uint64_t a3 = x & 0x0000f0000f0000f0ull;
  uint64_t a6 = x & 0x00000000f0000f00ull;
  uint64_t c9 = x & 0x000f00000000f000ull;
  uint64_t b3 = x & 0x0f0000f0000f0000ull;
  uint64_t b6 = x & 0x00f0000f00000000ull;
  return a0 | a3 << 12 | a6 << 24 | c9 << 36 | b3 >> 12 | b6 >> 24 | c9 >> 36;
}

void print_matrix(uint64_t x) {
    printf("%04lx\n%04lx\n%04lx\n%04lx\n-\n", x&0xffff, x>>16&0xffff, x>>32&0xffff, x>>48&0xffff);
}

// ~5.1s
uint64_t transpose3(uint64_t x) {
  uint64_t a, b, c;
  // 18 op
  a = (x ^ (x >> 12)) & 0x0000f0000f0000f0ull;
  b = (x ^ (x >> 24)) & 0x00000000f0000f00ull;
  c = (x ^ (x >> 36)) & 0x000000000000f000ull;
  x = x ^ a ^ (a << 12) ^ b ^ (b << 24) ^ c ^ (c << 36);
  return x;
}

// ~4.9s
uint64_t transpose4(uint64_t x) {
  // 14 op
	uint64_t a1 = x & 0xF0F00F0FF0F00F0FULL;
	uint64_t a2 = x & 0x0000F0F00000F0F0ULL;
	uint64_t a3 = x & 0x0F0F00000F0F0000ULL;
	uint64_t a = a1 | (a2 << 12) | (a3 >> 12);
	uint64_t b1 = a & 0xFF00FF0000FF00FFULL;
	uint64_t b2 = a & 0x00FF00FF00000000ULL;
	uint64_t b3 = a & 0x00000000FF00FF00ULL;
	return b1 | (b2 >> 24) | (b3 << 24);
}

uint64_t transpose5(uint64_t x) {
  // 12 op
  uint64_t t;
  t = (x ^ (x >> 12)) & 0x0000f0f00000f0f0ull;
  x ^= t ^ (t << 12);
  t = (x ^ (x >> 24)) & 0x00000000ff00ff00ull;
  x ^= t ^ (t << 24);
  return x;
}

// -------------------------------------------------------
uint64_t count_blank0(uint64_t b) {
  int c = 0;
  while (b) {
    if (b & 0xf)
      c++;
    b >>= 4;
  }
  return 16 - c;
}

uint64_t count_blank1(uint64_t x) {
  x |= (x >> 2) & 0x3333333333333333ULL;
  x |= (x >> 1);
  x = ~x & 0x1111111111111111ULL;
  x += x >> 32;
  x += x >> 16;
  x += x >>  8;
  x += x >>  4;
  return x & 0xf;
}

const uint64_t k4 = uint64_t(0x0f0f0f0f0f0f0f0f);
const uint64_t kf = uint64_t(0x0101010101010101);
uint64_t count_blank2(uint64_t b) {
  b = ~b;
  b &= b >> 2;
  b &= b >> 1;
  b &= 0x1111111111111111ull;

  b = (b       +  (b >> 4)) & k4 ;
  b = (b * kf) >> 56;

  return b;
}

uint64_t count_blank3(uint64_t b) {
  b = ~b;
  b &= b >> 2;
  b &= b >> 1;
  b &= 0x1111111111111111ull;
  b = (b * 0x1111111111111111ull) >> 60;

  return b;
}
 
#if GCC_VERSION >= 40400
// gcc supports target attribute since 4.4
__attribute__ ((__target__ ("popcnt")))
uint64_t count_blank4(uint64_t b) {
  b = ~b;
  b &= b >> 2;
  b &= b >> 1;
  b &= 0x1111111111111111ull;

  return __builtin_popcountll(b);
}
#endif

uint64_t (*funcs[])(uint64_t) = {
#if 1
  transpose0,
  transpose1,
  transpose2,
  transpose3,
  transpose4,
  transpose5,
#endif
#if 0
  count_blank0,
  count_blank1,
  count_blank2,
  count_blank3,
#if GCC_VERSION >= 40400
  count_blank4,
#endif
#endif
  NULL
};

int main(void) {

  for (int fi = 0; funcs[fi]; fi++) {
    uint64_t (*func)(uint64_t) = funcs[fi];

    seed = 0;
    uint64_t s = 0;
    clock_t t0 = clock();
    for (int i = 0; i < 100000; i++) {
      uint64_t b;

      for (int j = 0; j < 10001; j++) {
      b = ((uint64_t)myrand()) << 40 ^ ((uint64_t)myrand() << 16)  ^ myrand();
        b = func(b);
        s += b;
      }
    }
    clock_t t1 = clock();

    printf("%d %lx %ld clocks\n", fi, s, t1-t0);
  }
  return 0;
}

