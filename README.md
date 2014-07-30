2048-c
======

This is my 2048 puzzle AI module written in C for a [2048 contest].

The time limit of move is average 0.01 seconds for 100 games.

[2048 contest]: http://2048-botcontest.twbbs.org/

Warning: be careful with numbers in log
---------------------------------------
Don't believe performance numbers (time) in git commit log because
 1. Hyperthreading make time measurement difficult
 2. I often change compiler version but didn't keep note in the commit log.
 3. The machine was often busy on other tasks at the same time the tests ran.

Don't believe score numbers in git commit log because
 1. I used bad random number generator before commit 9a21b5. It always generates tile 2 before the fix.

So, read numbers carefully. Only compare those numbers relatively, don't trust the absolute values.


Algorithm
---------
- Overall
  + The heuristic function is based on my [2048-python], which is highly inspired by ovolve's description in [stackoverflow 2048].
  + Search algorithm is highly inspired by nneonneo's 2048 AI. Although I wrote from scratch, I wrote mine after read his code and borrowed several ideas.
- Search:
  + expectiminimax. It can search 10+ steps ahead by pruning low possibility nodes.
  + helper minimax search (only consider tile-2) to avoid dead. It can search 20+ steps ahead.
  + includes xificurk's [adaptive depth search]
- Eval: inspired from the stackoverflow article, combined with monotoneness,
  smoothness, and number of blank tile.
  + Weighting of these scores are tuned by hand first. And minor tuned by
    automatic program. I don't know the reason of those magic numbers ;)
- Optimization:
  + Similar to, or borrowed from nneonneo/2048-ai
    * Bit operations
    * Move and score by table lookup
    * Cut some 4 tile since the probability is low
  + Enhanced or different to nneonneo/2048-ai
    * Faster bit operations.
    * Cache: use more than one hash table. Fixed size hash table to avoid allocation. Also take 'depth' into consideration.
    * Use 64 bits for score calculation. (32 bits are not enough and may loss some precision)
  + Optimization for contest:
    * time control (average 100 moves per sec)
    * embed prebuild optimized (gcc4.6 -O3) binary. Because the contest host use gcc4.2 -O2.

[2048-python]: https://github.com/kcwu/2048-python
[stackoverflow 2048]: http://stackoverflow.com/a/22389702
[adaptive depth search]: https://github.com/nneonneo/2048-ai/pull/27

Result
------
tag _contest-version_ is the version I submitted for contest.

The contest [result] (100 runs):

    max score=625260
    avg score=277965.00
    max tile=32768
    2048 rate=100%
    4096 rate=100%
    8192 rate=96%
    16384 rate=67%
    32768 rate=2%

p.s. After _contest-version_, I made some improvements but the program become much slower.

[result]: http://2048-botcontest.twbbs.org/download/stats.htm

Credits
-------
- Algorithm and evaluation ideas from
  + ovolve's nice answer on stackoverflow http://stackoverflow.com/a/22389702
  + nneonneo's 2048 AI https://github.com/nneonneo/2048-ai
  + xificurk https://github.com/nneonneo/2048-ai/pull/27
- Load shellcode
  + I extended feliam's code to x86 64 https://github.com/feliam/mkShellcode
- My hash function is from Murmurhash3 mix function https://code.google.com/p/smhasher/wiki/MurmurHash3
- Bit tricks from
  + http://chessprogramming.wikispaces.com/Population+Count
  + http://icodeguru.com/Embedded/Hacker's-Delight/048.htm


License
-------
BSD license
