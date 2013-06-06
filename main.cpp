
#include "util.h"
#include "game_state.h"
#include "survival.h"
#include "points.h"
#include "watchdog.h"
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

using namespace std;

static inline void readMap(){
    for(int c=0;c<16;c++){
        gets(&(map[c][0]));
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
    ourSpawn = currLoc;
    brickDelay = 0;
    scanf(" %d %d ",&enemyLoc.first,&enemyLoc.second);
    enemySpawn = enemyLoc;
    enemySpawnDelay = 0;
    enemyBrickDelay = 0;
    scanf(" %d\n",&nenemies);
    for(int i=0;i<nenemies;i++){
        //todo, find a better way of dealing with the program.
        scanf(" %d %d %s",&enemies[i].loc.first,&enemies[i].loc.second,tempBuffer);
        enemies[i].spawn = enemies[i].loc;
        enemies[i].program = string(tempBuffer);
        enemies[i].master = -1;
        enemies[i].isTrapped = false;
        enemies[i].spawnDelay = 0;
        enemies[i].distSq = distSq(enemies[i].loc,currLoc);
        enemies[i].distSqToOpponent = distSq(enemies[i].loc,enemyLoc);
    }
    currTurn = -1;

}

static inline void act(Action act){
    puts(actionNames[act]);
    fflush(stdout);//don't remove this!
}



//returns false when finished
static bool doTurn(){
    int nextTurn;
    scanf(" %d",&nextTurn);
    if(nextTurn==-1)
        return false;
    TRACE("TRACE: Starting turn #%d\n",nextTurn);
    missedTurns = nextTurn - currTurn -1;
    if(missedTurns!=0)
        WARN("WARNING: Lost Turn(s)! (%d turns lost)\n",missedTurns);
    currTurn = nextTurn;
    readMap();
    scanf(" %d %d %*d %d",&currLoc.first,&currLoc.second,&brickDelay);
    scanf(" %d %d %*d %d",&enemyLoc.first,&enemyLoc.second,&enemyBrickDelay);
    if(enemyLoc.first==-1){
        if(enemySpawnDelay==0){
            enemySpawnDelay = max(49-missedTurns,1);
        }
        else{
            enemySpawnDelay--;
            if(enemySpawnDelay==0)
                enemySpawnDelay=1;
        }
    }
    else{
        enemySpawnDelay = 0;
    }
    for(int i=0;i<nenemies;i++){
        scanf(" %d %d %d",&enemies[i].loc.first,&enemies[i].loc.second,&enemies[i].master);

        if(enemies[i].loc.first!=-1 && (map[enemies[i].loc.first][enemies[i].loc.second]==REMOVED_BRICK || map[enemies[i].loc.first][enemies[i].loc.second]==FILLED_BRICK)){
            map[enemies[i].loc.first][enemies[i].loc.second]=FILLED_BRICK;
            enemies[i].isTrapped = true;
        }
        else{
            enemies[i].isTrapped = false;
        }
        if(enemies[i].loc.first==-1){
            enemies[i].distSq = -1;
            enemies[i].distSqToOpponent = -1;
            if(enemies[i].spawnDelay==0){
                enemies[i].spawnDelay=max(24-missedTurns,1);
            }
            else{
                enemies[i].spawnDelay--;
                if(enemies[i].spawnDelay==0)
                    enemies[i].spawnDelay = 1;
            }
            enemies[i].chaseState = PATROL;
        }
        else{
            enemies[i].spawnDelay=0;
            if(currLoc.first!=-1 && !enemies[i].isTrapped)
                enemies[i].distSq = distSq(enemies[i].loc,currLoc);
            else
                enemies[i].distSq = -1;
            if(enemyLoc.first!=-1 && !enemies[i].isTrapped)
                enemies[i].distSqToOpponent = distSq(enemies[i].loc,enemyLoc);
            else
                enemies[i].distSqToOpponent = -1;
            switch(computeChaseState(i).first){
            case RED:
                enemies[i].chaseState = CHASE_RED;
                break;
            case BLUE:
                enemies[i].chaseState = CHASE_BLUE;
                break;
            case NOONE:
                //TODO deal with return to patrol
                if(enemies[i].chaseState == CHASE_RED ||
                   enemies[i].chaseState == CHASE_BLUE){
                    enemies[i].chaseState = UNKNOWN;
                }
                break;
            }

        }
    }
    for(int i=0;i<16;i++){
        TRACE("MAP: %s\n",&map[i][0]);
    }

    TRACE("POS: %d %d\n",currLoc.first,currLoc.second);

    //actual ai starts here
    int survivalScore[7];
    memset(survivalScore,0,sizeof(int)*7);
    scoreSurvival(survivalScore);

    //TODO add points score

    state s = pointsScore();
    TRACE("TRACE: Action: %d pos: %d %d depth: %d\n",static_cast<int>(s.first),s.pos.first,s.pos.second,s.depth);
    if(s.first!=NONE)
        survivalScore[s.first]+=50;

    vector<Action> bests;
    int maxScore = 0;
    for(int i=NONE;i<7;i++){
        if(survivalScore[i]>maxScore){
            maxScore = survivalScore[i];
            bests.clear();
            bests.push_back(static_cast<Action>(i));
        }
        else if(survivalScore[i]==maxScore){
            bests.push_back(static_cast<Action>(i));
        }
    }
    Action a = NONE;
    if(bests.size()!=0){
        a = bests[rand()%bests.size()];
        TRACE("CANDIDATES:");
        for(int i=0;i<bests.size();i++){
            TRACE(" %s",actionNames[bests[i]]);
        }
        TRACE("\n");
    }
    act(a);
    TRACE("TRACE: Turn #%d finished with action %s with a score of %d\n",currTurn,actionNames[a],maxScore);

    return true;
}

int main(){
    srand(time(NULL));
    startWatchdog();
    initGame();
    while(doTurn());
    TRACE("Game Finished!\n");
}
