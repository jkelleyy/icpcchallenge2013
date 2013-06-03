
#include "util.h"
#include "game_state.h"
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace std;

#define EMPTY '.'
#define LADDER 'H'
#define BRICK '='
#define GOLD '*'
#define REMOVED_BRICK '-'
#define FILLED_BRICK '+'

//global game state!, make sure to update these on every restart!

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
pair<int,int> enemyLocs[16*25];
string enemyPrograms[16*25];
int enemyMasters[16*25];

static inline void readMap(){
    for(int c=0;c<16;c++){
        gets(&(map[c][0]));
        TRACE("MAP: %s\n",&map[c][0]);
        if(map[c][0]!=EMPTY &&
           map[c][0]!=LADDER &&
           map[c][0]!=BRICK &&
           map[c][0]!=GOLD &&
           map[c][0]!=REMOVED_BRICK)
            c--;
    }
}

static char tempBuffer[1000];

static inline void initGame(){
    scanf(" %d\n",&nrounds);
    readMap();
    scanf(" %d %d ",&currLoc.first,&currLoc.second);
    scanf(" %d %d ",&enemyLoc.first,&enemyLoc.second);
    scanf(" %d\n",&nenemies);
    for(int i=0;i<nenemies;i++){
        //todo, find a better way of dealing with the program.
        scanf(" %d %d %s",&enemyLocs[i].first,&enemyLocs[i].second,tempBuffer);
        enemyPrograms[i] = string(tempBuffer);
        enemyMasters[i] = -1;
    }
    currTurn = -1;

}

enum Action{
    NONE=0,LEFT,RIGHT,DIG_LEFT,DIG_RIGHT,TOP,BOTTOM
};

const char *actionNames[7] = {
    "NONE",
    "LEFT",
    "RIGHT",
    "DIG_LEFT",
    "DIG_RIGHT",
    "TOP",
    "BOTTOM"
};

static inline void act(Action act){
    puts(actionNames[act]);
    fflush(stdout);//don't remove this!
}


bool isSupported(){
    return currLoc.first==15
        || map[currLoc.first+1][currLoc.second]==BRICK
        || map[currLoc.first+1][currLoc.second]==LADDER
        || map[currLoc.first][currLoc.second]==LADDER
        || map[currLoc.first+1][currLoc.second]==FILLED_BRICK;

}

bool isAlive(){
    return currLoc.first!=-1;
}

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

//returns false when finished
bool doTurn(){
    int score[7];//the score for each of the possible commands
    memset(score,0,sizeof(int)*7);
    int nextTurn;
    scanf(" %d",&nextTurn);
    if(nextTurn==-1)
        return false;
    TRACE("TRACE: Starting turn #%d\n",nextTurn);
    if(nextTurn!=currTurn+1)
        WARN("WARNING: Lost Turn(s)! (%d turns lost)\n",nextTurn-currTurn-1);
    currTurn = nextTurn;
    readMap();
    int ignored;
    scanf(" %d %d %d %d",&currLoc.first,&currLoc.second,&ignored,&brickDelay);
    scanf(" %d %d %d %d",&enemyLoc.first,&enemyLoc.second,&ignored,&enemyBrickDelay);
    for(int i=0;i<nenemies;i++){
        scanf(" %d %d %d",&enemyLocs[i].first,&enemyLocs[i].second,&enemyMasters[i]);

        if(enemyLocs[i].first!=-1 && map[enemyLocs[i].first][enemyLocs[i].second]==REMOVED_BRICK){
            map[enemyLocs[i].first][enemyLocs[i].second]=FILLED_BRICK;
        }
    }
    TRACE("POS: %d %d\n",currLoc.first,currLoc.second);
    //do fun stuff!
    for(int i=NONE;i<7;i++){
        if(canDoAction(static_cast<Action>(i)))
            score[i]+=1;
        else
            score[i]=-1;
    }
    score[0]=0;
    //end fun stuff!
    vector<Action> bests;
    int maxScore = 0;
    for(int i=NONE;i<7;i++){

        if(score[i]>maxScore){
            maxScore = score[i];
            bests = vector<Action>();
            bests.push_back(static_cast<Action>(i));
        }
        else if(score[i]==maxScore){
            bests.push_back(static_cast<Action>(i));
        }
    }
    Action a = NONE;
    if(bests.size()!=0){
        a = bests[rand()%bests.size()];
    }
    act(a);
    TRACE("TRACE: Turn #%d finished with action %s with a score of %d\n",currTurn,actionNames[a],maxScore);
    return true;
}

int main(){
    srand(time(NULL));
    initGame();
    while(doTurn());
    TRACE("Game Finished!\n");
}
