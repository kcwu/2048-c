#CXX42_32 = ./g++-4.2.2-i686
CXX42_64 = ./g++-4.2.2
CXX46_64=g++-4.6

CXX=$(CXX42_64)
CXXFLAGS=-O2 -Wall -Wextra

#CXX=$(CXX46_64)
#CXXFLAGS=-O3 -Wall -Wextra -static

COMMON_CC=util.cc bot.cc
COMMON_H=util.h bot.h common.h

all: 2048 micro_optimize integration bot_opt.bin

2048: $(COMMON_CC) $(COMMON_H) Makefile main.cc
	$(CXX) $(CXXFLAGS) -o 2048 $(COMMON_CC) main.cc
integration: $(COMMON_CC) $(COMMON_H) Makefile integration.cc 2048.cpp 2048.h bot_opt.cc bot_opt.h
	$(CXX) $(CXXFLAGS) -o integration 2048.cpp $(COMMON_CC) integration.cc bot_opt.cc

bot.o: common.h bot.h bot.cc Makefile
	$(CXX46_64) -fno-stack-protector -fno-common -O3 -fomit-frame-pointer -fno-PIC -c -static bot.cc

bot_opt.cc: extract.py bot.o Makefile
	python extract.py bot.o

micro_optimize: micro_optimize.cc
	$(CXX) $(CXXFLAGS) -o micro_optimize micro_optimize.cc

DIST_DIR=2048_bot_cpp
dist: integration
	rm -rf $(DIST_DIR)
	mkdir $(DIST_DIR)
	cp $(COMMON_CC) $(COMMON_H) integration.cc bot_opt.cc bot_opt.h $(DIST_DIR)
	zip -r 2048_bot_cpp.zip $(DIST_DIR)
