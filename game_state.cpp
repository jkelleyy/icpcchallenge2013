#include "game_state.h"

int nrounds;
int nenemies;
int currTurn;
//row column
//the extra +1 is for a null terminator, it makes reading in the data easier
char map[16][25+1];
pair<int,int> currLoc;
int brickDelay;
pair<int,int> enemyLoc;
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
            && map[currLoc.first][currLoc.second-1]!=BRICK;
    case RIGHT:
        return 24>currLoc.second
            && isSupported()
            && map[currLoc.first][currLoc.second+1]!=BRICK;
        //dig right and dig left seem flipped
    case DIG_LEFT:
        return brickDelay==0
            && 15>currLoc.first
            && currLoc.second>0
            && isSupported()
            && map[currLoc.first+1][currLoc.second-1]==BRICK
            && map[currLoc.first][currLoc.second-1]!=BRICK
            && map[currLoc.first][currLoc.second-1]!=LADDER;
    case DIG_RIGHT:
        return brickDelay==0
            && 15>currLoc.first
            && 24>currLoc.second
            && isSupported()
            && map[currLoc.first+1][currLoc.second+1]==BRICK
            && map[currLoc.first][currLoc.second+1]!=BRICK
            && map[currLoc.first][currLoc.second+1]!=LADDER;
    case TOP:
        return currLoc.first>0
            && map[currLoc.first][currLoc.second]==LADDER
            && map[currLoc.first-1][currLoc.second]!=BRICK;
    case BOTTOM:
        return 15>currLoc.first
            && map[currLoc.first+1][currLoc.second]==LADDER;
    }
    return false;//should be unreachable
}
