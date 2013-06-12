#include "survival.h"
#include "game_state.h"
#include "util.h"
#include <queue>
#include <set>
#include <utility>
#include <cassert>

using namespace std;

//copy over only the data relevant to prediction
void copyPredictionData(EnemyInfo& dest, const EnemyInfo& src){
    dest.loc = src.loc;
    dest.spawnDelay = src.spawnDelay;
    dest.isTrapped = src.isTrapped;
    dest.chaseState = src.chaseState;
    dest.chaseInfo = src.chaseInfo;
    dest.chaseStack = src.chaseStack;
    dest.patrolIndex = src.patrolIndex;
    dest.lastMove = src.lastMove;
}

//note this is very specialized
//returns NEG_INF on certain death, otherwise it returns a safety score
//with 0 being safe, and negative scores being different levels of unsafeness
int predictFall(pair<int,int> nextLoc){
    TRACE("BEGIN PREDICTION\n");
    EnemyInfo* predictedEnemies = new EnemyInfo[nenemies];
    for(int i=0;i<nenemies;i++)
        copyPredictionData(predictedEnemies[i],enemies[i]);
    bool first = true;
    while(!isSupported(nextLoc)){
        if(!first)
            nextLoc.first++;
        for(int i=0;i<nenemies;i++){
            if(predictedEnemies[i].isTrapped)
                continue;
            if(predictedEnemies[i].spawnDelay==0 &&
               !predictedEnemies[i].isAlive()){
                predictedEnemies[i].loc = enemies[i].spawn;
            }
            if(predictedEnemies[i].spawnDelay!=0){
                predictedEnemies[i].spawnDelay--;
            }
            else if(!isSupportedEnemy(predictedEnemies[i].loc)){
                predictedEnemies[i].loc.first++;
            }
            else if(predictedEnemies[i].lastMove!=NONE){
                predictedEnemies[i].lastMove = NONE;
                if(predictedEnemies[i].chaseState==CHASE_RED){
                    if(predictedEnemies[i].chaseStack.empty())
                        predictedEnemies[i].chaseState = PATROL;
                    else
                        predictedEnemies[i].chaseState = RETURN_TO_PATROL;
                }
            }
            else{
                switch(predictedEnemies[i].chaseState){

                case CHASE_BLUE:
                    //let's just ignore these for now
                    break;
                case CHASE_RED:{
                    //assert(first);
                    Action move = predictedEnemies[i].chaseInfo.startDir;
                    predictedEnemies[i].loc = simulateAction(move,predictedEnemies[i].loc);
                    predictedEnemies[i].chaseStack.push_front(move);
                    predictedEnemies[i].chaseState = RETURN_TO_PATROL;
                    predictedEnemies[i].lastMove = move;
                    break;
                }
                case RETURN_TO_PATROL:{
                    Action move = predictedEnemies[i].chaseStack.front();
                    predictedEnemies[i].chaseStack.pop_front();
                    move = reverseAction(move);
                    predictedEnemies[i].loc = simulateAction(move,predictedEnemies[i].loc);
                    predictedEnemies[i].lastMove = move;
                    if(predictedEnemies[i].chaseStack.empty())
                        predictedEnemies[i].chaseState = PATROL;
                    break;
                }
                case PATROL:{
                    Action move = enemies[i].program[predictedEnemies[i].patrolIndex];
                    predictedEnemies[i].patrolIndex++;
                    predictedEnemies[i].patrolIndex%=enemies[i].program.size();
                    predictedEnemies[i].loc = simulateAction(move,predictedEnemies[i].loc);
                    predictedEnemies[i].lastMove = move;
                    break;
                }
                case UNKNOWN:
                    break;//can't really do much in this case...
                }
            }
            TRACE("PREDICT: %d -> %d %d\n",i,predictedEnemies[i].loc.first,predictedEnemies[i].loc.second);
            if(predictedEnemies[i].loc==nextLoc){
                delete[] predictedEnemies;
                return NEG_INF;//death!
            }
        }
        first = false;
        TRACE("PREDICTION: us -> %d %d\n",nextLoc.first,nextLoc.second);

    }
    delete[] predictedEnemies;
    return 0;
}

static bool isSafeFall(pair<int,int> loc){
    while(loc.first<15 && !isSolid(map[loc.first+1][loc.second])){
        loc.first++;
    }
    //we are either on the last row or above a solid block, now check sides
    if(map[loc.first][loc.second]!=REMOVED_BRICK)
        return true;
    if(loc.second>0 && map[loc.first][loc.second-1]!=REMOVED_BRICK && !isImpassable(map[loc.first][loc.second-1]))
        return true;
    if(loc.second<24 && map[loc.first][loc.second+1]!=REMOVED_BRICK && !isImpassable(map[loc.first][loc.second+1]))
        return true;
    return false;
}

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
    return !isSafeFall(loc);
}

struct Info{
    pair<int,int> loc;
    ChaseInfo info;
};

static bool isActionReversible(Action a, pair<int,int> loc){
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

int goldInCurrComponent(pair<int,int> loc)
{
	int comp = component[loc.first][loc.second];

	int gold = 0;
	for(int i = 0; i < 16; i++)
		for(int j = 0; j < 26; j++)
			if(component[i][j] == comp)
				if(map[i][j] == GOLD)
					gold++;
	return gold;
}

bool isSuicidal(Action action, pair<int,int> loc)
{
	pair<int,int> next = simulateAction(action, loc);
	while(next.first<15 && !isSolid(map[next.first+1][next.second]))
		next.first++;
	if(map[next.first][next.second] == REMOVED_BRICK)
		return true;
	return false;
}

//figure out who an enemy is chasing
//and which direction it will chase in
pair<int,ChaseInfo> computeChaseState(int enemyId){
    set<pair<int,int> >seen;
    queue<Info> locs;
    Info start;
    start.loc = enemies[enemyId].loc;
    start.info.pathLength = 0;
    start.info.startDir = NONE;
    start.info.attackDir = NONE;
    if(!isSupported(start.loc)){
        return make_pair(NOONE,start.info);
    }
    seen.insert(start.loc);
    locs.push(start);
    while(!locs.empty()){
        Info curr = locs.front();
        locs.pop();
        if((enemies[enemyId].master==NOONE && curr.info.pathLength>5) || curr.info.pathLength>8){
            return make_pair(NOONE,start.info);
        }
        if(curr.loc==currLoc){
            TRACE("RED!\n");
            switch(enemies[enemyId].master){
            case RED:
                if(curr.info.pathLength>4){
                    break;
                }
                else{
                    return make_pair(RED,curr.info);
                }
            case BLUE:
                if(curr.info.pathLength>8){
                    //um.... shouldn't happen
                    WARN("WARN: Unreachable code reached!\n");
                    break;
                }
                else{
                    return make_pair(RED,curr.info);
                }
            case NOONE:
                if(curr.info.pathLength>5){
                    WARN("WARN: Unreachable code reached!\n");
                    break;
                }
                else{
                    return make_pair(RED,curr.info);
                }
            }
        }
        if(curr.loc==enemyLoc){
            switch(enemies[enemyId].master){
            case RED:
                if(curr.info.pathLength>8){
                    WARN("WARN: Unreachable code reached!\n");
                    break;
                }
                else{
                    return make_pair(BLUE,curr.info);
                }
            case BLUE:
                if(curr.info.pathLength>4){
                    break;
                }
                else{
                    return make_pair(BLUE,curr.info);
                }
            case NOONE:
                if(curr.info.pathLength>5){
                    WARN("WARN: Unreachable code reached!\n");
                    break;
                }
                else{
                    return make_pair(BLUE,curr.info);
                }
            }
        }
        else{
#define DO_BRANCH(dir)                                                  \
            if(canDoActionEnemy(dir,curr.loc) &&                        \
               isActionReversible(dir,curr.loc)){                       \
                Info next;                                              \
                next.info.pathLength = curr.info.pathLength+1;          \
                switch(dir){                                            \
                case LEFT:                                              \
                    next.loc = make_pair(curr.loc.first,curr.loc.second-1); \
                    break;                                              \
                case RIGHT:                                             \
                    next.loc = make_pair(curr.loc.first,curr.loc.second+1); \
                    break;                                              \
                case TOP:                                               \
                    next.loc = make_pair(curr.loc.first-1,curr.loc.second); \
                    break;                                              \
                case BOTTOM:                                            \
                    next.loc = make_pair(curr.loc.first+1,curr.loc.second); \
                    break;                                              \
                }                                                       \
                next.info.startDir = curr.info.startDir;                \
                next.info.attackDir = reverseAction(dir);               \
                if(next.info.startDir==NONE) next.info.startDir = dir;  \
                if(seen.find(next.loc)==seen.end()){                    \
                    locs.push(next);                                    \
                    seen.insert(next.loc);                              \
                }                                                       \
            }
            //note the ordering here is important, don't mess it up
            if(enemies[enemyId].spawn.second<12){
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
    }
    return make_pair(NOONE,start.info);
}

void scoreSurvival(int *score){
    for(int i=NONE;i<7;i++){
        if(canDoAction(static_cast<Action>(i)))
            score[i]+=100;
        else
            score[i]=NEG_INF;
    }
    for(int i=0;i<nenemies;i++){
        TRACE("CHASE: %s %d last:%s\n",chaseStateNames[enemies[i].chaseState],enemies[i].chaseInfo.pathLength,actionNames[enemies[i].lastMove]);
        if(enemies[i].chaseState==CHASE_RED ||
           (enemies[i].chaseState==CHASE_BLUE && enemyLoc==currLoc)){
            if(enemies[i].chaseInfo.pathLength==1){
                score[enemies[i].chaseInfo.attackDir]=NEG_INF;
                score[NONE] = NEG_INF;
                score[DIG_LEFT] -= 100;
                score[DIG_RIGHT] -= 100;
            }
            else if(enemies[i].chaseInfo.pathLength==2 && enemies[i].lastMove==NONE){
                score[enemies[i].chaseInfo.attackDir]=NEG_INF;
            }
            else{
                //score[enemies[i].chaseInfo.attackDir]-=10-enemies[i].chaseInfo.pathLength;
            }
            if(enemies[i].chaseInfo.attackDir!=LEFT)
                score[LEFT]+=1;
            if(enemies[i].chaseInfo.attackDir!=RIGHT)
                score[RIGHT]+=1;
            if(enemies[i].chaseInfo.attackDir!=TOP)
                score[TOP]+=1;
            if(enemies[i].chaseInfo.attackDir!=BOTTOM)
                score[BOTTOM]+=1;


            if(enemies[i].chaseInfo.pathLength==2 || (enemies[i].chaseInfo.pathLength==1 && enemies[i].lastMove!=NONE)){
                switch(enemies[i].chaseInfo.attackDir){
                case LEFT:
                    score[DIG_LEFT] += 10;
                    break;
                case RIGHT:
                    score[DIG_RIGHT] += 10;
                    break;
                default:
                    break;
                }
            }
        }

    }
    if(isAlive()){
        for(int i=NONE;i<7;i++){
			if((goldInCurrComponent(currLoc) == 0) && (gold_comp[component[currLoc.first][currLoc.second]] < (totalGoldOnMap / 2)))
				if(isSuicidal(static_cast<Action>(i), currLoc))
					score[i] += 1000;
            if(score[i]>0 && isTrap(static_cast<Action>(i)))
				score[i]-=200;//really bad, but not instant death,
            pair<int,int> newLoc = simulateAction(static_cast<Action>(i),currLoc);
            if(score[i]>=0){
                score[i]+=predictFall(newLoc);
            }

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
        //try not to get stuck from digging
        //this is a bit terrible
        //TODO reduce copy paste
        if(canDoAction(DIG_LEFT)){
            if(currLoc.first>=14 || !isSafeFall(make_pair(currLoc.first+2,currLoc.second-1))){

                //search right for an outlet
                bool hasOutlet = false;
                pair<int,int> loc = currLoc;
                int stepCount=0;
                while(loc.second<25 && !isImpassable(map[loc.first][loc.second])){
                    if(stepCount>5){
                        hasOutlet = true;
                        break;
                    }
                    if(map[loc.first][loc.second]==LADDER || map[loc.first+1][loc.second]==LADDER){
                        hasOutlet=true;
                        break;
                    }
                    if(!isSupported(loc)){
                        if(isSafeFall(loc))
                            hasOutlet = true;
                        break;
                    }
                    loc.second++;
                    stepCount++;
                }
                if(!hasOutlet){
                    score[DIG_LEFT] -= 5;
                }
            }
        }
        if(canDoAction(DIG_RIGHT)){
            if(currLoc.first>=14 || !isSafeFall(make_pair(currLoc.first+2,currLoc.second+1))){
                //search left for an outlet
                bool hasOutlet = false;
                pair<int,int> loc = currLoc;
                int stepCount=0;
                while(loc.second>=0 && !isImpassable(map[loc.first][loc.second])){
                    if(stepCount>5){
                        hasOutlet = true;
                        break;
                    }
                    if(map[loc.first][loc.second]==LADDER || map[loc.first+1][loc.second]==LADDER){
                        hasOutlet=true;
                        break;
                    }
                    else if(!isSupported(loc)){
                        if(isSafeFall(loc))
                            hasOutlet = true;
                        break;
                    }
                    loc.second--;
                    stepCount++;
                }
                if(!hasOutlet){
                    score[DIG_RIGHT] -= 5;
                }
            }
        }
    }
    score[NONE]=0;//prefer anything do doing nothing
}
