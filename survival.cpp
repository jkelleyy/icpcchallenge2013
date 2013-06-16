#include "survival.h"

#include "game_state.h"
#include "util.h"
#include <queue>
#include <set>
#include <utility>
#include <cassert>
#include <cmath>
#include <cstring>

using namespace std;

struct PredictionState{
    int gold;
    int kills;
    int depth;
    Action startDir;
    World *state;
};

static inline bool isActionReversible(Action a, pair<int,int> loc){
    //assumes the action is possible and that the current locations is supported
    switch(a){
    case NONE:
        return true;
    case DIG_LEFT:
    case DIG_RIGHT:
        return false;//this probably shouldn't be called with these anyways
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
    }
    return canDoActionEnemy(reverseAction(a),loc);
}

#define LEFT_SIDE 0
#define RIGHT_SIDE 1

static inline loc_t moveDir(const loc_t& curr,Action act){
    switch(act){
    case LEFT:
        return make_pair(curr.first,curr.second-1);
    case RIGHT:
        return make_pair(curr.first,curr.second+1);
    case TOP:
        return make_pair(curr.first-1,curr.second);
    case BOTTOM:
        return make_pair(curr.first+1,curr.second);
    default:
        return curr;
    }

}

int enemyDistCache[16][25][16][25];
//return -1 for "infinite" distance
int checkEnemyDist(loc_t enemyLoc, loc_t loc){
    int& val = enemyDistCache[enemyLoc.first][enemyLoc.second][loc.first][loc.second];
    if(enemyLoc==loc)
        return 0;
    if(val!=0)
        return val;
    queue<pair<loc_t,int> > todo;
    todo.push(make_pair(enemyLoc,0));
    bool seen[16][25];
    for(int i=0;i<16;i++)
        for(int j=0;j<25;j++)
            seen[i][j] = false;
    while(!todo.empty()){
        pair<loc_t,int> curr = todo.front();
        todo.pop();
        if(curr.first==loc){
            return val = curr.second;
        }
#define DO_BRANCH(dir)                                          \
        if(canDoActionEnemy(dir,curr.first) &&             \
           isActionReversible(dir,curr.first)){            \
            pair<loc_t,int> next;                               \
            next.second = curr.second+1;                        \
            next.first = moveDir(curr.first,dir);               \
            if(!seen[next.first.first][next.first.second]){     \
                todo.push(next);                           \
                seen[next.first.first][next.first.second]=true; \
            }                                                   \
        }
        DO_BRANCH(TOP);
        DO_BRANCH(RIGHT);
        DO_BRANCH(BOTTOM);
        DO_BRANCH(LEFT);
#undef DO_BRANCH
    }
    return val = -1;
}

Action chaseCache[16][25][16][25][2];
Action computeChaseDir(loc_t enemyLoc, loc_t loc, int startSide){
    Action& val = chaseCache[enemyLoc.first][enemyLoc.second][loc.first][loc.second][startSide];
    if(enemyLoc==loc)
        return NONE;
    if(val!=NONE)
        return val;
    queue<pair<loc_t,Action> > todo;
    todo.push(make_pair(enemyLoc,NONE));
    bool seen[16][25];
    for(int i=0;i<16;i++)
        for(int j=0;j<25;j++)
            seen[i][j] = false;
    while(!todo.empty()){
        pair<loc_t,Action> curr = todo.front();
        todo.pop();
        if(curr.first==loc){
            return val = curr.second;
        }
#define DO_BRANCH(dir)                                          \
        if(canDoActionEnemy(dir,curr.first) &&                  \
           isActionReversible(dir,curr.first)){                 \
            pair<loc_t,Action> next;                            \
            next.second = curr.second;                          \
            if(next.second==NONE) next.second = dir;            \
            next.first = moveDir(curr.first,dir);               \
            if(!seen[next.first.first][next.first.second]){     \
                todo.push(next);                           \
                seen[next.first.first][next.first.second]=true; \
            }                                                   \
        }
        if(startSide==LEFT_SIDE){
            DO_BRANCH(TOP);
            DO_BRANCH(RIGHT);
            DO_BRANCH(BOTTOM);
            DO_BRANCH(LEFT);
        }
        else{
            DO_BRANCH(TOP);
            DO_BRANCH(LEFT);
            DO_BRANCH(BOTTOM);
            DO_BRANCH(RIGHT);
        }
#undef DO_BRANCH
    }
    return val = NONE;
}

//very rarely, this will get the person being chased wrong
//if we and the enemy are the same distance and various other things
//match up, but either case, it will predict the next move correctly
ChaseInfo computeChase(const World& world,int i){
    if(!world.enemies[i].isAlive()){
        ChaseInfo info;
        info.startDir = NONE;
        info.pathLength = 0;
        info.target = NOONE;
        return info;
    }
    int ourDist;
    if(world.isAlive())
        ourDist = checkEnemyDist(world.enemies[i].getLoc(),world.currLoc);
    else
        ourDist = -1;
    int oppDist;
    if(world.enemyLoc.first==-1)
        oppDist = checkEnemyDist(world.enemies[i].getLoc(),world.enemyLoc);
    else
        oppDist = -1;
    ChaseInfo ret;
    int ourRadius =
        world.enemies[i].getMaster()==RED ? 4 :
        world.enemies[i].getMaster()==BLUE ? 8 : 5;
    int oppRadius =
        world.enemies[i].getMaster()==RED ? 8 :
        world.enemies[i].getMaster()==BLUE ? 4 : 5;
    if(oppDist<0)
        oppDist = 100;
    if(ourDist<0)
        ourDist = 100;

    if(ourDist<=ourRadius && (ourDist<oppDist || oppDist>oppRadius)){
        ret.pathLength = ourDist;
        ret.target = RED;
        ret.startDir = computeChaseDir(world.enemies[i].getLoc(),world.currLoc,world.enemies[i].info->spawn.second<12 ? LEFT_SIDE : RIGHT_SIDE);
    }
    else if(oppDist<=oppRadius && (oppDist<ourDist || ourDist>ourRadius)){
        ret.pathLength = oppDist;
        ret.target = BLUE;
        ret.startDir = computeChaseDir(world.enemies[i].getLoc(),world.enemyLoc,world.enemies[i].info->spawn.second<12 ? LEFT_SIDE : RIGHT_SIDE);
    }
    else if(oppDist<=oppRadius && ourDist<=ourRadius){
        assert(ourDist==oppDist);
        ret.pathLength = ourDist;
        if(world.enemies[i].info->spawn.second<12){
            Action a1 = computeChaseDir(world.enemies[i].getLoc(),world.currLoc,LEFT_SIDE);
            Action a2 = computeChaseDir(world.enemies[i].getLoc(),world.enemyLoc,LEFT_SIDE);
            if(a1==TOP){
                ret.startDir = TOP;
                ret.target = RED;
            }
            else if(a2==TOP){
                ret.startDir = TOP;
                ret.target = BLUE;
            }
            else if(a1==RIGHT){
                ret.startDir = RIGHT;
                ret.target = RED;
            }
            else if(a2==RIGHT){
                ret.startDir = RIGHT;
                ret.target = BLUE;
            }
            else if(a1==BOTTOM){
                ret.startDir = BOTTOM;
                ret.target = RED;
            }
            else if(a2==BOTTOM){
                ret.startDir = BOTTOM;
                ret.target = BLUE;
            }
            else if(a1==LEFT){
                ret.startDir = LEFT;
                ret.target = RED;
            }
            else if(a2==LEFT){
                ret.startDir = LEFT;
                ret.target = BLUE;
            }
        }
        else{
            Action a1 = computeChaseDir(world.enemies[i].getLoc(),world.currLoc,RIGHT_SIDE);
            Action a2 = computeChaseDir(world.enemies[i].getLoc(),world.enemyLoc,RIGHT_SIDE);
            if(a1==TOP){
                ret.startDir = TOP;
                ret.target = RED;
            }
            else if(a2==TOP){
                ret.startDir = TOP;
                ret.target = BLUE;
            }
            else if(a1==LEFT){
                ret.startDir = LEFT;
                ret.target = RED;
            }
            else if(a2==LEFT){
                ret.startDir = LEFT;
                ret.target = BLUE;
            }
            else if(a1==BOTTOM){
                ret.startDir = BOTTOM;
                ret.target = RED;
            }
            else if(a2==BOTTOM){
                ret.startDir = BOTTOM;
                ret.target = BLUE;
            }
            else if(a1==RIGHT){
                ret.startDir = RIGHT;
                ret.target = RED;
            }
            else if(a2==RIGHT){
                ret.startDir = RIGHT;
                ret.target = BLUE;
            }
        }
    }
    else{
        ret.pathLength = 0;
        ret.target = NOONE;
        ret.startDir = NONE;
    }
    return ret;
}

static void stepEnemy(World * world, int i){
    ChaseInfo info;
    info.target=NOONE;
    if(world->enemies[i].getSpawnDelay()==0){
        if(!world->enemies[i].isAlive()){
            world->enemies[i].setLoc(world->enemies[i].info->spawn);
        }
    }
    else{
        world->enemies[i].setSpawnDelay(world->enemies[i].getSpawnDelay()-1);
    }
    if(world->enemies[i].isFalling()){
        world->enemies[i].setLastMove(BOTTOM);
    }
    else if(world->enemies[i].didMove()){
        world->enemies[i].setLastMove(NONE);
        return;
    }
    if(distSq(world->enemies[i].getLoc(),world->currLoc)<=64)
        info = computeChase(*world,i);
    Action move;
    switch(info.target){
    case RED:
    case BLUE:
        move = info.startDir;
        world->enemies[i].chaseStack.push(move);
        break;
    case NOONE:
        if(world->enemies[i].chaseStack.empty()){
            move = world->enemies[i].info->program[world->enemies[i].patrolIndex];
            world->enemies[i].patrolIndex++;
            world->enemies[i].patrolIndex%=world->enemies[i].info->program.size();
        }
        else{
            move = world->enemies[i].chaseStack.peek();
            world->enemies[i].chaseStack.pop();
        }
        break;
    }
    world->enemies[i].setLastMove(move);
    world->enemies[i].setLoc(simulateAction(move,world->enemies[i].getLoc()));
    return;
}

PredictionState stateTransition(PredictionState start,Action act){
    PredictionState ret = start;
    ret.depth++;
    ret.state = new World;
    *ret.state = *start.state;
    for(int i=0;i<fixedData.nenemies;i++){
        if(ret.state->enemies[i].isAlive() && !ret.state->enemies[i].isTrapped()){
            stepEnemy(ret.state,i);
            if(ret.state->enemies[i].getLoc()==ret.state->currLoc &&
               ret.state->enemies[i].getLastMove()==reverseAction(act) &&
               act!=NONE){
                //killed!
                ret.state->currLoc = make_pair(-1,-1);
            }
            if(ret.state->enemies[i].isTrapped())
                ret.kills++;
        }
    }


    //update map
    if(ret.state->isAlive()){
        ret.state->currLoc = simulateAction(act,ret.state->currLoc);
        switch(act){
        case DIG_LEFT:
            ret.state->map.lookup(ret.state->currLoc.first+1,ret.state->currLoc.second-1) = REMOVED_BRICK;
            break;
        case DIG_RIGHT:
            ret.state->map.lookup(ret.state->currLoc.first+1,ret.state->currLoc.second+1) = REMOVED_BRICK;
            break;
        }
    }
    //note, I'm not going to bother with restoring bricks and gold since the
    //simulation is so short
    if(ret.state->checkMapSafe(ret.state->currLoc)==GOLD){
        ret.gold++;
        ret.state->map.lookup(ret.state->currLoc)=EMPTY;
    }
    for(int i=0;i<fixedData.nenemies;i++){
        if(ret.state->enemies[i].getLoc()==ret.state->currLoc){
            ret.state->currLoc = make_pair(-1,-1);
        }
    }
    if(ret.state->brickDelay!=0)
        ret.state->brickDelay--;
    if(ret.state->checkMapSafe(ret.state->currLoc)==REMOVED_BRICK &&
       isSolid(ret.state->checkMapSafe(ret.state->currLoc.first+1,ret.state->currLoc.second)) &&
       isSolid(checkBaseMapSafe(ret.state->currLoc.first,ret.state->currLoc.second+1)) &&
       isSolid(checkBaseMapSafe(ret.state->currLoc.first,ret.state->currLoc.second-1))){
        //effectively dead
        ret.state->currLoc = make_pair(-1,-1);
    }
    return ret;
}

#define DEPTH_FUNC(val)
#define KILL_FUNC(val)
#define COIN_FUNC(val)
#define ENEMY_DIST_FUNC(val)


int scoreState(PredictionState state){
    //TODO
    if(state.depth>0)
        return 50*state.depth;
    else
        return -100;
}

//a list of valid actions in order of preference
//and a "score"
//note, normally the score is monotoic, but that's not a strict requirement
//we prefer non-death paths, but sometimes the score is higher on a death path
void predict(int *scores){
    //true if there is a completely safe path out using this move
    PredictionState best[7];
    queue<PredictionState> todo;
    PredictionState start;
    for(int i=0;i<7;i++){
        best[i].gold = 0;
        best[i].kills = 0;
        best[i].depth = 0;
        best[i].startDir = NONE;
        best[i].state = NULL;
    }
    start.gold = 0;
    start.kills = 0;
    start.depth = 0;
    start.startDir = NONE;
    start.state = new World;
    *start.state = game;
    start.state->enemyLoc = make_pair(-1,-1);
    todo.push(start);
    while(!todo.empty()){
        PredictionState curr = todo.front();
        todo.pop();
        if(curr.depth>3){
            delete curr.state;
            while(!todo.empty()){
                delete todo.front().state;
                todo.pop();
            }
            break;
        }
        for(int i=NONE;i<7;i++){
            if(curr.state->canDoActionPlayer(static_cast<Action>(i))){
                PredictionState newState = stateTransition(curr,static_cast<Action>(i));
                if(curr.depth==0){
                    newState.startDir=static_cast<Action>(i);
                }
                if(curr.state->isAlive())
                    todo.push(newState);
                else{
                    delete newState.state;
                }
            }
        }
        if(curr.startDir!=NONE){
            //TODO prevent memleak
            if(curr.depth>best[curr.startDir].depth){
                if(best[curr.startDir].state)
                    delete best[curr.startDir].state;
                best[curr.startDir] = curr;
            }
            else if (curr.kills>best[curr.startDir].kills){
                if(best[curr.startDir].state)
                    delete best[curr.startDir].state;
                best[curr.startDir] = curr;
            }
            else if (curr.gold>best[curr.startDir].gold){
                if(best[curr.startDir].state)
                    delete best[curr.startDir].state;
                best[curr.startDir] = curr;
            }
            else{
                delete curr.state;
            }
        }
        else{
            delete curr.state;
        }
    }
    for(int i=0;i<7;i++){
        TRACE("DEPTH: %d\n",best[i].depth);
        scores[i] = scoreState(best[i]);
        if(best[i].state!=NULL)
            delete best[i].state;
    }
    for(int i=0;i<7;i++){
        TRACE("SCORE: %s %d\n",actionNames[i],scores[i]);
    }
}

void initSurvival(){
    memset(chaseCache,0,sizeof(chaseCache));
    memset(enemyDistCache,0,sizeof(enemyDistCache));
}
