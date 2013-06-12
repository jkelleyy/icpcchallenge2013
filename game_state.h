#ifndef BISTROMATHICS_GAME_STATE_H
#define BISTROMATHICS_GAME_STATE_H 1

#include <utility>
#include <string>
#include <list>

using namespace std;

#define EMPTY '.'
#define LADDER 'H'
#define BRICK '='
#define GOLD '*'
#define REMOVED_BRICK '-'
#define FILLED_BRICK '+'


enum ChaseState{
    //UNKNOWN is if we lose track and can't figure out what the enemy is doing
    //if it's dead, use PATROL
    CHASE_RED,CHASE_BLUE,RETURN_TO_PATROL,PATROL,UNKNOWN
};

static const char* chaseStateNames[] = {
    "CHASE_RED",
    "CHASE_BLUE",
    "RETURN_TO_PATROL",
    "PATROL",
    "UNKNOWN"
};

//find a better place for these...
enum Player{
    NOONE = -1,RED=0,BLUE=1
};

enum Action{
    NONE=0,LEFT,RIGHT,DIG_LEFT,DIG_RIGHT,TOP,BOTTOM
};

struct ChaseInfo{
    int pathLength;
    Action startDir;
    Action attackDir;
};


struct EnemyInfo{
    pair<int,int> loc;
    pair<int,int> spawn;
    int spawnDelay;//same comment as for enemySpawnDelay
    string program;
    Player master;
    bool isTrapped;
    Action lastMove;
    int distSq;
    int distSqToOpponent;
    ChaseState chaseState;
    ChaseInfo chaseInfo;
    list<Action> chaseStack;
    int patrolIndex;
};

//things that don't change
class StaticWorldData{
public:
    int nrounds;
    int nenemies;
    pair<int,int> enemySpawn;
    pair<int,int> ourSpawn;
};

extern StaticWorldData fixedData;

//these things change every turn
class World{
public:
    int currTurn;
    int missedTurns;
    char map[16][26];
    pair<int,int> currLoc;
    int currScore;
    int brickDelay;
    pair<int,int> enemyLoc;
    int enemyScore;
    int enemySpawnDelay;
    int enemyBrickDelay;
    EnemyInfo enemies[16*25];
};

extern World game;

//extern the global state
//the actually declarations for all of these are in game_state.cpp
static int& nrounds = fixedData.nrounds;
static int& nenemies = fixedData.nenemies;
static int& currTurn = game.currTurn;
static int& missedTurns = game.missedTurns;
static char (&map)[16][26] = game.map;

extern int component[16][26];
extern int gold_comp[600];
extern int max_gold_comp;
extern int totalGoldOnMap;
extern bool reachable[16][26][16][26];
extern int depth[16][26];
extern pair<int,int> earliest_parent[16][26];

static pair<int,int>& currLoc = game.currLoc;
static pair<int,int>& ourSpawn = fixedData.ourSpawn;
static int& currScore = game.currScore;
static int& brickDelay = game.brickDelay;
static pair<int,int>& enemyLoc = game.enemyLoc;
static pair<int,int>& enemySpawn = fixedData.enemySpawn;
static int& enemyScore = game.enemyScore;
//if we lose some turns this could be an estimate,
//if it's 0, then the enemy is alive
static int& enemySpawnDelay = game.enemySpawnDelay;
static int& enemyBrickDelay = game.enemyBrickDelay;
static EnemyInfo (&enemies)[16*25] = game.enemies;


//returns '\0' if it's off the map
static inline char checkMapSafe(int r, int c){
    if(r>=0 && r<16 && c>=0 && c<25)
        return map[r][c];
    return '\0';
}

static inline char checkMapSafe(const pair<int,int>& loc){
    return checkMapSafe(loc.first,loc.second);
}

//bunch of tiny utility functions

static inline bool isImpassable(char c){
    return c==BRICK || c==FILLED_BRICK || c=='\0';
}

static inline bool isSolid(char c){
    return c==BRICK || c==LADDER || c==FILLED_BRICK || c=='\0';
}

static inline bool isBrick(const pair<int,int>& loc){
    return map[loc.first][loc.second] == BRICK;
}

static inline bool isSupported(const pair<int,int>& loc = currLoc){
    return isSolid(checkMapSafe(loc.first+1,loc.second)) || map[loc.first][loc.second]==LADDER;
}

static inline bool isAlive(){
    return currLoc.first!=-1;
}

static inline int distSq(const pair<int,int>& a, const pair<int,int>& b){
    return (b.first-a.first)*(b.first-a.first)+(b.second-a.second)*(b.second-a.second);
}

static inline Action reverseAction(Action a){
    switch(a){
    case NONE:
        return NONE;
    case LEFT:
        return RIGHT;
    case RIGHT:
        return LEFT;
    case DIG_LEFT:
        return DIG_RIGHT;
    case DIG_RIGHT:
        return DIG_LEFT;
    case TOP:
        return BOTTOM;
    case BOTTOM:
        return TOP;
    }
}


static const char *actionNames[7] = {
    "NONE",
    "LEFT",
    "RIGHT",
    "DIG_LEFT",
    "DIG_RIGHT",
    "TOP",
    "BOTTOM"
};

bool canDoActionRaw(Action act, const pair<int,int>& loc);
bool canDoActionPlayer(Action act, const pair<int,int>& loc = currLoc);
bool canDoActionOpponent(Action act, const pair<int,int>& loc = enemyLoc);
bool canDoActionEnemy(Action act, const pair<int,int>& loc);

//dont use this one if possible, it will be removed soon.
static inline bool canDoAction(Action act,const pair<int,int>& loc = currLoc){
    return canDoActionPlayer(act,loc);
}
bool canDoAction2(Action act,const pair<int,int>& loc = currLoc);
pair<int,int> simulateAction(Action act,const pair<int,int>& loc);
#endif
