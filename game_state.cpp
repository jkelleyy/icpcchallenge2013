#include "game_state.h"

int nrounds;
int nenemies;
int currTurn;
int missedTurns;
//row column
//the extra +1 is for a null terminator, it makes reading in the data easier
char map[16][25+1];
pair<int,int> currLoc;
pair<int,int> ourSpawn;
int brickDelay;
pair<int,int> enemyLoc;
pair<int,int> enemySpawn;
int enemySpawnDelay;
int enemyBrickDelay;
//we can't have more enemies than grid squares!
enemyInfo enemies [16*25];

bool canDoAction(Action act){
    if(!isAlive() && act!=NONE)
        return false;
    switch(act){
    case NONE:
        return true;
    case LEFT:
        return currLoc.second>0
            && isSupported()
            && !isImpassable(map[currLoc.first][currLoc.second-1]);
    case RIGHT:
        return 24>currLoc.second
            && isSupported()
            && !isImpassable(map[currLoc.first][currLoc.second+1]);
    case DIG_LEFT:
        return brickDelay==0
            && 15>currLoc.first
            && currLoc.second>0
            && isSupported()
            //this seems silly
            && map[currLoc.first+1][currLoc.second]!=FILLED_BRICK
            && map[currLoc.first+1][currLoc.second-1]==BRICK
            && map[currLoc.first][currLoc.second-1]!=BRICK
            && map[currLoc.first][currLoc.second-1]!=LADDER;
    case DIG_RIGHT:
        return brickDelay==0
            && 15>currLoc.first
            && 24>currLoc.second
            && isSupported()
            //this seems silly
            && map[currLoc.first+1][currLoc.second]!=FILLED_BRICK
            && map[currLoc.first+1][currLoc.second+1]==BRICK
            && map[currLoc.first][currLoc.second+1]!=BRICK
            && map[currLoc.first][currLoc.second+1]!=LADDER;
    case TOP:
        return currLoc.first>0
            && map[currLoc.first][currLoc.second]==LADDER
            && !isImpassable(map[currLoc.first-1][currLoc.second]);
    case BOTTOM:
        return 15>currLoc.first
            && map[currLoc.first+1][currLoc.second]==LADDER;
    }
    return false;//should be unreachable
}
