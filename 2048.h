#ifndef GAME_H_
#define GAME_H_
#define NDEBUG       // turn off assert() for debugging
#define EN_PRINT     // turn on printing functions

#ifdef _WIN32
#include <windows.h>
#elif defined(__linux__)
#include <sys/time.h>
#endif

#include <fstream>
#include <iostream>
#include <assert.h>
#include <cstdlib>
#include <cstring>

#define TRUE                true
#define FALSE               false

#define GRID_LENGTH         4
#define GRID_SIZE           16
#define STAGE_NUM           7
#define FIRST_STAGE         2048

#define INITIAL_TILE_NUM    2
#define EMPTY               0
#define NORMAL_TILE         2
#define WILD_CARD_TILE      4

#define TILE_GEN_RATIO      9

#define DIR_NUM             4

#define ERROR_KEY           -1

typedef enum{
    LEFT = 0,
    DOWN,
    RIGHT,
    UP,
    INVALID
} dir_e;

void gotoXY(int xPos, int yPos);
double cpuTime();

class Grid{
    private:
        int   m_data[GRID_SIZE];
        int   m_nEmptyBlk;
        int   m_score;

        // inline
        int&  getEntry(int row, int col);
        int&  getFlipEntry(int row, int col);
        int&  getTransEntry(int row, int col);
        int&  getFlipTransEntry(int row, int col);
    public:
        // inline
        Grid();
        int   operator[](int index);
        int   operator()(int row, int col);
        void  clear();
        void  copy(Grid& grid);
        void  setBlock(int index, int val);
        int   getEmptyBlkNo();
        int   getScore();

        int   getMaxTile();
        bool  shift(dir_e dir);
        void  print(int xPos = 0, int yPos = 0);
};

class Game{
    private:
        Grid  m_grid;
        bool  m_gameOver;
        int   m_nRound;
        int   m_maxScore;
        int   m_scoreSum;
        int   m_maxTile;
        int   m_passCnt[STAGE_NUM];
        int   m_moveCnt;
        double   m_startTime;
        double   m_endTime;

        // inline
        int   getRand();
                
        void  genNewTile();
        void  updateStats();
        void  setGameOver();
        void  dumpLog(const char* fileName);
    public:
        Game();
        ~Game();
        void  reset();

        // inline
        void  getCurrentGrid(Grid& currGrid);
        bool  insertDirection(dir_e dir);
        bool  isGameOver();
        int   getScore();
        int   getMaxScore();         
        void  printGrid(int xPos, int yPos);
};

/*************************************************
                   Grid inline
*************************************************/
// Grid()
// Description: initialize members
inline
Grid::Grid(){
    clear();
}

// operator[]
// Description: returns entry value specified by index
//              returns error key when given index is out of range
// Arguments:
//     index  -  index of entry
// Return Val: entry value specified by index, or ERROR_KEY if out of range
inline
int Grid::operator[](int index){
    if(index < 0 || index > GRID_SIZE - 1){
        assert(FALSE);
        return ERROR_KEY;
    }
    return m_data[index];
}

// operator()
// Description: returns entry value specified by (row, col) coordinates
//              returns error key when given coordinates are out of range
// Arguments:
//     row  -  row number of entry
//     col  -  column number of entry
// Return Val: entry value specified by (row, col) coordinates, or ERROR_KEY if out of range
inline
int Grid::operator()(int row, int col){
    int index = row * GRID_LENGTH + col;
    if(index < 0 || index > GRID_SIZE - 1){
        assert(FALSE);
        return ERROR_KEY;
    }
    return m_data[index];
}

// clear()
// Description: sets all grid entries to EMPTY
inline
void Grid::clear(){
    memset(m_data, EMPTY, sizeof(m_data));
    m_nEmptyBlk = GRID_SIZE;
    m_score = 0;
}

// copy()
// Description: copies contents of given grid into this grid
// Arguments:
//     grid  -  given grid
inline
void Grid::copy(Grid& grid){
    if(this == &grid)  return;
    memcpy(m_data, grid.m_data, sizeof(m_data));
    m_nEmptyBlk = grid.m_nEmptyBlk;
    m_score = grid.m_score;
}

// setBlock()
// Description: sets value of specified block
// Arguments:
//     index  -  index of block
//	   val    -  specified value that will be assigned to block
inline
void Grid::setBlock(int index, int val){
    if(index < 0 || index >= GRID_SIZE){
        assert(FALSE);
        return;
    }
    if(m_data[index] == EMPTY && val != EMPTY)
        m_nEmptyBlk--;
    else if(m_data[index] != EMPTY && val == EMPTY)
        m_nEmptyBlk++;
    m_data[index] = val;
}

// getEmptyBlkNo()
// Description: return number of empty blocks in grid
// Return Val: number of empty blocks in given grid
inline
int Grid::getEmptyBlkNo(){
    return m_nEmptyBlk;
}

// getScore()
// Description: return current score in grid
// Return Val: current score in grid
inline
int Grid::getScore(){
    return m_score;
}

// getEntry()
// Description: get entry of original grid
// Arguments:
//     row  -  row number of entry
//     col  -  col number of entry
// Return Val: entry of original grid
inline
int& Grid::getEntry(int row, int col){
    return m_data[row*GRID_LENGTH + col];
}

// getFlipEntry()
// Description: get entry of horizontally flipped grid
// Arguments:
//     row  -  row number of entry
//     col  -  col number of entry
// Return Val: entry of horizontally flipped grid
inline
int& Grid::getFlipEntry(int row, int col){
    return m_data[row*GRID_LENGTH + (GRID_LENGTH - 1 - col)];
}

// getTransEntry()
// Description: get entry of transposed grid
// Arguments:
//     row  -  row number of entry
//     col  -  col number of entry
// Return Val: entry of transposed grid
inline
int& Grid::getTransEntry(int row, int col){
    return m_data[col*GRID_LENGTH + row];
}

// getFlipTransEntry()
// Description: get entry of horizontally flipped, transposed grid
// Arguments:
//     row  -  row number of entry
//     col  -  col number of entry
// Return Val: entry of horizontally flipped, transposed grid
inline
int& Grid::getFlipTransEntry(int row, int col){
    return m_data[(GRID_LENGTH - 1 - col)*GRID_LENGTH + row];
}

/*************************************************
                   Game inline
*************************************************/
// getRand()
// Description: return pseudo random integer value between "0 to RAND_MAX"
// Return Val: pseudo random integer value between "0 to RAND_MAX"
inline
int Game::getRand(){
    return rand();
}

// isGameOver()
// Description: check if game is over (cannot shift in any direction)
// Return Val: returns TRUE if game is over, FALSE if not
inline
bool Game::isGameOver(){
    return m_gameOver;
}

// getScore()
// Description: gets the current score of the game
// Return Val: current score of game
inline
int Game::getScore(){
    return m_grid.getScore();
}

// getScore()
// Description: gets the highest score ever achieved
// Return Val: highest score
inline
int Game::getMaxScore(){
    int currentScore = getScore();
    if(currentScore > m_maxScore)
        return currentScore;
    else
        return m_maxScore;
}

// insertDirection()
// Description: shift tiles in given direction and generate new tile randomly
//              if cannot be shifted, will not generate new tile
// Arguments:
//     dir  -  shift direction
//
// Return Val: returns TRUE if shift was successful and tile was generated, FALSE if not
inline
bool Game::insertDirection(dir_e dir){
    if(!m_grid.shift(dir))
        return FALSE;

    genNewTile();
    m_moveCnt++;
    setGameOver();
    return TRUE;
}

// getCurrentGrid()
// Description: copy contents of game grid into given grid
// Arguments:
//     currGrid  -  game grid will be copied into this grid
inline
void Game::getCurrentGrid(Grid& currGrid){
    currGrid.copy(m_grid);
}

// printGrid()
// Description: prints the game grid at the given (x,y) coordinates
// Arguements:
//      xPos  -  x coordinate
//      yPos  -  y coordinate 
inline
void Game::printGrid(int xPos, int yPos){
    m_grid.print(xPos, yPos);
}

#endif
