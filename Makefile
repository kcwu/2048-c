#CXX42_32 = ./g++-4.2.2-i686
CXX42_64 = ./g++-4.2.2
CXX46_64=g++-4.6
CXX=$(CXX46_64)
#CXXFLAGS=-O2 -Wall -Wextra
CXXFLAGS=-O3 -Wall -Wextra -static

all: 2048 micro_optimize integration bot_opt.bin

2048: Makefile util.cc util.h bot.cc bot.h main.cc
	$(CXX) $(CXXFLAGS) -o 2048 bot.cc main.cc util.cc
integration: Makefile util.cc util.h bot.cc bot.h integration.cc 2048.cpp 2048.h
	$(CXX) -g $(CXXFLAGS) -o integration 2048.cpp bot.cc util.cc integration.cc

bot.o: bot.h bot.cc Makefile
	$(CXX46_64) -fno-stack-protector -fno-common -O3 -fomit-frame-pointer -fno-PIC -c -static bot.cc

bot_opt.bin: extract.py bot.o Makefile
	python extract.py bot.o

micro_optimize: micro_optimize.cc
	$(CXX) $(CXXFLAGS) -o micro_optimize micro_optimize.cc
