#include <utility>
#include <string>

using namespace std;

#define EMPTY '.'
#define LADDER 'H'
#define BRICK '='
#define GOLD '*'
#define REMOVED_BRICK '-'
#define FILLED_BRICK '+'

//extern the global state
//the actually declarations for all of these are in game_state.cpp
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

//bunch of tiny utility functions

static inline bool isImpassable(char c){
    return c==BRICK || c==FILLED_BRICK;
}

static inline bool isSolid(char c){
    return c==BRICK || c==LADDER || c==FILLED_BRICK;
}

static inline bool isSupported(){
    return currLoc.first==15
        || isSolid(map[currLoc.first+1][currLoc.second])
        || map[currLoc.first][currLoc.second]==LADDER;
}

static inline bool isAlive(){
    return currLoc.first!=-1;
}

static inline int distSq(const pair<int,int>& a, const pair<int,int>& b){
    return (b.first-a.first)*(b.first-a.first)+(b.second-a.second)*(b.second-a.second);
}

enum Action{
    NONE=0,LEFT,RIGHT,DIG_LEFT,DIG_RIGHT,TOP,BOTTOM
};

static const char *actionNames[7] = {
    "NONE",
    "LEFT",
    "RIGHT",
    "DIG_LEFT",
    "DIG_RIGHT",
    "TOP",
    "BOTTOM"
};

bool canDoAction(Action act);
