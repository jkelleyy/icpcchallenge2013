#include "survival.h"
#include "game_state.h"
#include "util.h"

//don't do stupid things like falling into traps...
//assume that moving in that direction is valid...
//
static bool isTrap(Action dir){
    pair<int,int> loc = currLoc;
    switch(dir){
    case DIG_LEFT:
    case DIG_RIGHT:
        //these can't really be traps in the sense we care about
        return false;
    case LEFT:
        loc.second--;
        break;
    case RIGHT:
        loc.second++;
        break;
    case TOP:
        loc.first--;
        break;
    case BOTTOM:
        loc.first++;
        break;
    case NONE:
        break;
    }
    while(loc.first<15 && !isSolid(map[loc.first+1][loc.second])){
        loc.first++;
    }
    //we are either on the last row or above a solid block, now check sides
    if(map[loc.first][loc.second]!=REMOVED_BRICK)
        return false;
    if(loc.second>0 && !isImpassable(map[loc.first][loc.second-1]))
        return false;
    if(loc.second<24 && !isImpassable(map[loc.first][loc.second+1]))
        return false;
    return true;
}

void scoreSurvival(int *score){
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
                    if(currLoc.second>enemies[i].loc.second){
                        TRACE("HI %d\n",currTurn);
                        score[RIGHT] += 10;
                        score[DIG_LEFT] += 1;
                        score[LEFT] = NEG_INF;
                    }
                    else{
                        TRACE("HI2 %d\n",currTurn);
                        score[LEFT] += 10;
                        score[DIG_RIGHT] += 1;
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
        for(int i=NONE;i<7;i++){
            //the positive score condition makes sure that we can actually
            //do that action
            if(score[i]>0 && isTrap(static_cast<Action>(i)))
                score[i]-=200;//really bad, but not instant death,
        }
        //make sure not to walk into spawning enemies
        for(int i=0;i<nenemies;i++){
            if(enemies[i].spawnDelay==1){
                switch(distSq(enemies[i].spawn,currLoc)){
                case 0:
                    score[NONE]=NEG_INF;
                    score[DIG_LEFT] -= 100;
                    score[DIG_RIGHT] -= 100;
                    break;
                case 1:
                    if(enemies[i].spawn.first<currLoc.first){
                        score[TOP]=NEG_INF;
                    }
                    else if(enemies[i].spawn.first>currLoc.first){
                        score[BOTTOM]=NEG_INF;
                    }
                    else if(enemies[i].spawn.second<currLoc.second){
                        score[LEFT]=NEG_INF;
                    }
                    else{
                        score[RIGHT]=NEG_INF;
                    }
                }
            }
        }
    }


}
