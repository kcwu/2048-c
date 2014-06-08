#include "2048.h"
/*************************************************
                   Grid public
*************************************************/
// getMaxTile()
// Description: return largest tile number in grid
// Return Val: largest tile number in grid
int Grid::getMaxTile(){
    int max = 0;
    for(int i = 0;i < GRID_SIZE;i++){
        if(max < m_data[i])  max = m_data[i];
    }
    return max;
}

// shift()
// Description: shift tiles in given direction
// Arguments:
//     dir   -   shift direction
// Return Val: returns TRUE if shift is possible, FALSE if not
bool Grid::shift(dir_e dir){
    int nTile;
    int lastMerge;
    bool isShifted = FALSE;
    int shiftBuff[GRID_LENGTH];

    int& (Grid::* getDirEntry)(int, int) = NULL;
        
    assert(dir == LEFT || dir == DOWN || dir == RIGHT || dir == UP);

    if(dir == LEFT)          getDirEntry = &Grid::getEntry;
    else if(dir == DOWN)     getDirEntry = &Grid::getFlipTransEntry;
    else if(dir == RIGHT)    getDirEntry = &Grid::getFlipEntry;
    else                     getDirEntry = &Grid::getTransEntry;

    assert(getDirEntry != NULL);

    for(int i = 0;i < GRID_LENGTH;i++){
        nTile = lastMerge = 0;
        memset(shiftBuff, EMPTY, sizeof(shiftBuff));
        for(int j = 0;j < GRID_LENGTH;j++){
            if((this->*getDirEntry)(i, j) == EMPTY)
                continue;
            if(nTile > lastMerge && (this->*getDirEntry)(i, j) == shiftBuff[nTile - 1]){
                shiftBuff[nTile - 1] *= 2;
                lastMerge = nTile;
                isShifted = TRUE;
                m_nEmptyBlk++;
                m_score += shiftBuff[nTile - 1];
            }
            else{
                if(nTile != j)  isShifted = TRUE;
                shiftBuff[nTile++] = (this->*getDirEntry)(i, j);
            }
        }
        for(int j = 0;j < GRID_LENGTH;j++)
            (this->*getDirEntry)(i, j) = shiftBuff[j];
    }

   return isShifted;
}

// print()
// Description: prints grid at given origin (xPos, yPos)
// Arguments:
//     xPos  -  x coordinate of origin
//     yPos  -  y coordinate of origin
void Grid::print(int xPos, int yPos){
#ifdef EN_PRINT
    for(int i = 0;i < GRID_LENGTH;i++){
        for(int j = 0;j < GRID_LENGTH;j++){
            int x = xPos + 6 * j;
            int y = yPos + 2 * i;
            gotoXY(x,y);
            std::cout<<"      ";
            gotoXY(x,y);
            std::cout<<m_data[i*GRID_LENGTH + j];
        }
    }
#endif
}

/*************************************************
                   Game public
*************************************************/

// Game()
// Description: initialize game record & reset game
Game::Game(){
    m_nRound = 0;
    m_moveCnt = 0;
    m_maxScore = 0;
    m_scoreSum = 0;
    m_maxTile = 0;
    memset(m_passCnt, 0, sizeof(m_passCnt));
    m_gameOver = FALSE;

    for(int i = 0;i < INITIAL_TILE_NUM;i++)
        genNewTile();

    m_startTime = cpuTime();
}

// ~Game()
// Description: game ends, calculate & dump results to log
Game::~Game(){
    m_endTime = cpuTime();
    updateStats();
    dumpLog("open_src_version.log");
}

// Reset()
// Description: prepare variables for new game
void Game::reset(){
    updateStats();
    m_grid.clear();
    m_gameOver = FALSE;
    
    for(int i = 0;i < INITIAL_TILE_NUM;i++)
        genNewTile();
}

/*************************************************
                   Game private
*************************************************/

// genNewTile()
// Description: generate a new tile in a random empty block
void Game::genNewTile(){ 
    int randBlk = getRand() % m_grid.getEmptyBlkNo() + 1;
    
    for(int i = 0;i < GRID_SIZE;i++){             
        if(m_grid[i] == EMPTY)
            randBlk--;
            
        if(randBlk == 0){
            if(getRand() % (TILE_GEN_RATIO + 1) + 1 > TILE_GEN_RATIO)
                m_grid.setBlock(i, WILD_CARD_TILE);
            else
                m_grid.setBlock(i, NORMAL_TILE);
            return;
        }  
    }
    assert(FALSE);
}

// updateStats()
// Description: update game statistics at end of round
void Game::updateStats(){
    m_nRound++;
    int score = m_grid.getScore();
    m_scoreSum += score;
    if(score > m_maxScore)
        m_maxScore = score;
    int maxTile = m_grid.getMaxTile();
    if(maxTile > m_maxTile)
        m_maxTile = maxTile;
 
    for(int i = 0;i < STAGE_NUM;i++){
        if(maxTile >= (FIRST_STAGE << i))
            m_passCnt[i]++;
    }
}

// setGameOver()
// Description: determine whether any further shift can be made
void Game::setGameOver(){
    if(m_grid.getEmptyBlkNo() > 0){
        m_gameOver = FALSE;
        return;
    }
    
    for(int i = 0;i < GRID_LENGTH;i++){
        for(int j = 0;j < GRID_LENGTH;j++){
            if((i < GRID_LENGTH - 1) && m_grid(i,j) == m_grid(i+1,j)){
                m_gameOver = FALSE;
                return;
            }
            if((j < GRID_LENGTH - 1) && m_grid(i,j) == m_grid(i,j+1)){
                m_gameOver = FALSE;
                return;
            }
        }
    }
    m_gameOver = TRUE;
}

// dumpLog()
// Description: dump results into log
// Arguements:
//      fileName  -  log file name, will overwrite if already exists
void Game::dumpLog(const char* fileName){
    std::ofstream logFile(fileName, std::ios::out);
    logFile<<"#Rounds: "<<m_nRound<<"\n";
    logFile<<"Highest Score: "<<m_maxScore<<"\n";
    logFile<<"Average Score: "<< (double)m_scoreSum / m_nRound<<"\n";
    logFile<<"Max Tile: "<<m_maxTile<<'\n';
    for(int i = 0;i < STAGE_NUM;i++){
        logFile<< (FIRST_STAGE << i) <<" Rate: "<<(double)m_passCnt[i]*100/m_nRound<<"%\n";
    }
    logFile<<"Move Count: "<<m_moveCnt<<'\n';
    logFile<<"Time: "<<m_endTime - m_startTime<<'\n';
    logFile.close();
}

/*************************************************
                     Printing
*************************************************/
// gotoXY()
// Description: move cursor to (xPos, yPos)
// Arguments:
//     xPos  -  x coordinate
//     yPos  -  y coordinate
void gotoXY(int xPos, int yPos){
#ifdef EN_PRINT
  #ifdef _WIN32
  COORD scrn;
  HANDLE hOuput = GetStdHandle(STD_OUTPUT_HANDLE);
  scrn.X = xPos; scrn.Y = yPos;
  SetConsoleCursorPosition(hOuput,scrn);
  #elif defined(__linux__)
  printf("\033[%d;%df", yPos, xPos);
  fflush(stdout);
  #endif
#endif
}

/*************************************************
                  Time Measuring
*************************************************/
double cpuTime(){
#ifdef _WIN32
    FILETIME a,b,c,d;
    if (GetProcessTimes(GetCurrentProcess(),&a,&b,&c,&d) != 0)
        return (double)(d.dwLowDateTime | ((unsigned long long)d.dwHighDateTime << 32)) * 0.0000001;
    else
        return 0;

#elif defined(__linux__)
    return (double)clock() / CLOCKS_PER_SEC;
#endif
}

