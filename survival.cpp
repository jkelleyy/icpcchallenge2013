#include "survival.h"
#include "game_state.h"
#include "util.h"
#include <queue>
#include <set>
#include <utility>

using namespace std;

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
            //the positive score condition makes sure that we can actually
            //do that action
			/*
			pair<int,int> next = simulateAction(static_cast<Action>(i), currLoc);
			pair<int,int> next2 = simulateAction(NONE, next); //to account for falling
			
			//TODO: fix below code, shouldn't need >= 0 below it.
			for(int ii = 0; ii < 20; ii++)
			{
				next2 = simulateAction(NONE, next2); //to account for falling
			}
			
			if((next2.first >= 0) && (next2.second>= 0))
			{
				int oldComponent = component[currLoc.first][currLoc.second];
				int newComponent = component[next2.first][next2.second];
			
				bool shouldChangeComponent = true;
				if((oldComponent >= 0) && (newComponent >= 0) && (oldComponent != newComponent))
				{
					shouldChangeComponent = false;
					if(oldComponent < max_gold_comp)
						shouldChangeComponent = true;
					if(gold_comp[newComponent] >= gold_comp[oldComponent])
						shouldChangeComponent = true;
					if(gold_comp[newComponent] >= totalGoldOnMap / 2)
						shouldChangeComponent = true;
				}
            
				if(shouldChangeComponent)
				{
					TRACE("compo: changing from %d to %d\n", oldComponent, newComponent);
				}
				else
				{
					//score[i] -= 1000;
					TRACE("compo: avoiding changing from %d to %d\n", oldComponent, newComponent);
				}
			}
			*/
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
