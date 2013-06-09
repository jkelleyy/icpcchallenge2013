#ifndef BISTROMATHICS_GAME_STATE_H
#define BISTROMATHICS_GAME_STATE_H 1

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
extern int missedTurns;
extern char map[16][26];

extern int component[16][26];
extern int gold_comp[600];
extern int max_gold_comp;
extern int totalGoldOnMap;
extern bool reachable[16][26];
extern int depth[16][26];
extern pair<int,int> earliest_parent[16][26];

extern pair<int,int> currLoc;
extern pair<int,int> ourSpawn;
extern int currScore;
extern int brickDelay;
extern pair<int,int> enemyLoc;
extern pair<int,int> enemySpawn;
extern int enemyScore;
//if we lose some turns this could be an estimate,
//if it's 0, then the enemy is alive
extern int enemySpawnDelay;
extern int enemyBrickDelay;
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
#define RED 0
#define BLUE 1
#define NOONE -1


struct EnemyInfo{
    pair<int,int> loc;
    pair<int,int> spawn;
    int spawnDelay;//same comment as for enemySpawnDelay
    string program;
    int master;
    bool isTrapped;
    int distSq;
    int distSqToOpponent;
    ChaseState chaseState;
};
extern EnemyInfo enemies[16*25];


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

enum Action{
    NONE=0,LEFT,RIGHT,DIG_LEFT,DIG_RIGHT,TOP,BOTTOM
};

struct state
{
	Action first;
	pair<int,int> pos;
	int depth;
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

bool canDoActionRaw(Action act, const pair<int,int>& loc);
bool canDoActionPlayer(Action act, const pair<int,int>& loc = currLoc);
bool canDoActionOpponent(Action act, const pair<int,int>& loc = enemyLoc);
bool canDoActionEnemy(Action act, const pair<int,int>& loc);

//dont use this one if possible, it will be removed soon.
static inline bool canDoAction(Action act,const pair<int,int>& loc = currLoc){
    canDoActionPlayer(act,loc);
}
bool canDoAction2(Action act,const pair<int,int>& loc = currLoc);
pair<int,int> simulateAction(Action act,const pair<int,int>& loc);
#endif
