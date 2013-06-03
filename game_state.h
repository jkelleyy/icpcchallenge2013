#include <utility>
#include <string>

using namespace std;

//extern the global state
//the actually declarations for all of these are in main.cpp
extern int nrounds;
extern int nenemies;
extern int currTurn;
extern char map[16][26];
extern pair<int,int> currLoc;
extern int brickDelay;
extern pair<int,int> enemyLoc;
extern int enemyBrickDelay;
struct enemyInfo{
    pair<int,int> loc;
    string program;
    int master;
    bool isTrapped;
};

extern enemyInfo enemies[16*25];
