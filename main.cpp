
#include "util.h"
#include "game_state.h"
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>

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
    scanf(" %d %d ",&enemyLoc.first,&enemyLoc.second);
    scanf(" %d\n",&nenemies);
    for(int i=0;i<nenemies;i++){
        //todo, find a better way of dealing with the program.
        scanf(" %d %d %s",&enemies[i].loc.first,&enemies[i].loc.second,tempBuffer);
        enemies[i].program = string(tempBuffer);
        enemies[i].master = -1;
        enemies[i].isTrapped = false;
    }
    currTurn = -1;

}

static inline void act(Action act){
    puts(actionNames[act]);
    fflush(stdout);//don't remove this!
}

#define NEG_INF -100000000

//returns false when finished
static bool doTurn(){
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
        scanf(" %d %d %d",&enemies[i].loc.first,&enemies[i].loc.second,&enemies[i].master);

        if(enemies[i].loc.first!=-1 && map[enemies[i].loc.first][enemies[i].loc.second]==REMOVED_BRICK){
            map[enemies[i].loc.first][enemies[i].loc.second]=FILLED_BRICK;
            enemies[i].isTrapped = true;
        }
        else{
            enemies[i].isTrapped = false;
        }
    }
    for(int i=0;i<16;i++){
        TRACE("MAP: %s\n",&map[i][0]);
    }

    TRACE("POS: %d %d\n",currLoc.first,currLoc.second);
    //do fun stuff!
    for(int i=NONE;i<7;i++){
        if(canDoAction(static_cast<Action>(i)))
            score[i]+=1;
        else
            score[i]=NEG_INF;
    }
    //don't dig unless there's a reason to
    score[DIG_LEFT]-=1;
    score[DIG_RIGHT]-=1;
    if(isAlive()){
        for(int i=0;i<nenemies;i++){
            if(enemies[i].loc.first!=-1 && !enemies[i].isTrapped){
                TRACE("DIST: %d\n",distSq(currLoc,enemies[i].loc));
                switch(distSq(currLoc,enemies[i].loc)){
                case 1:
                    //run away
                    score[NONE] = NEG_INF;
                    if(currLoc.first==enemies[i].loc.first){
                        if(currLoc.second>enemies[i].loc.second){
                            score[RIGHT] += 10;
                            score[DIG_LEFT] += 1;
                            score[LEFT] = NEG_INF;
                        }
                        else{
                            score[LEFT] += 10;
                            score[DIG_RIGHT] += 1;
                            score[RIGHT] = NEG_INF;
                        }
                    }
                    else{
                        score[RIGHT] += 5;
                        score[LEFT] += 5;
                        if(currLoc.first>enemies[i].loc.first){
                            score[TOP]=NEG_INF;
                            score[BOTTOM] +=5;
                        }
                        else{
                            score[BOTTOM]=NEG_INF;
                            score[TOP] +=5;
                        }
                    }
                    break;
                case 2:
                    //also runaway
                    if(currLoc.second>enemies[i].loc.second){
                        score[RIGHT] += 10;
                        score[LEFT] = NEG_INF;
                    }
                    else{
                        score[LEFT] += 10;
                        score[RIGHT] = NEG_INF;
                    }
                    if(currLoc.first>enemies[i].loc.first){
                        score[TOP] = NEG_INF;
                        score[BOTTOM] += 10;
                    }
                    else{
                        score[BOTTOM] = NEG_INF;
                        score[TOP] += 10;
                    }
                    break;
                case 4:
                    //try and kill
                    if(currLoc.second>enemies[i].loc.second){
                        score[DIG_LEFT] += 20;
                        score[LEFT] = NEG_INF;
                    }
                    else if(currLoc.second<enemies[i].loc.second){
                        score[DIG_RIGHT] += 20;
                        score[RIGHT] = NEG_INF;
                    }
                    else if (currLoc.first>enemies[i].loc.first){
                        score[TOP] = NEG_INF;
                        score[BOTTOM] += 10;
                    }
                    else{
                        score[BOTTOM] = NEG_INF;
                        score[TOP] += 10;
                    }
                    break;
                }
            }
        }
    }

    score[0]=0;
    //end fun stuff!
    vector<Action> bests;
    int maxScore = 0;
    for(int i=NONE;i<7;i++){

        if(score[i]>maxScore){
            maxScore = score[i];
            bests.clear();
            bests.push_back(static_cast<Action>(i));
        }
        else if(score[i]==maxScore){
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
    initGame();
    while(doTurn());
    TRACE("Game Finished!\n");
}
