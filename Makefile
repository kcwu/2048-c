CXX = ./g++-4.2.2-i686
CXX=g++-4.6
CXXFLAGS=-O2 -Wall -Wextra
CXXFLAGS=-O3 -Wall -Wextra -static

all: 2048 micro_optimize

2048: 2048.cc Makefile
	$(CXX) $(CXXFLAGS) -o 2048 2048.cc
micro_optimize: micro_optimize.cc
	$(CXX) $(CXXFLAGS) -o micro_optimize micro_optimize.cc
