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

void scoreSurvival(int *score){
    for(int i=NONE;i<7;i++){
        if(canDoAction(static_cast<Action>(i)))
            score[i]+=1;
        else
            score[i]=NEG_INF;
    }
    for(int i=0;i<nenemies;i++){
        TRACE("CHASE: %s\n",chaseStateNames[enemies[i].chaseState]);
        if(enemies[i].chaseState==CHASE_RED){
            score[DIG_LEFT]=min(0,score[DIG_LEFT]);
            score[DIG_RIGHT]=min(0,score[DIG_RIGHT]);
            //break;
        }

    }
    if(isAlive()){
        for(int i=0;i<nenemies;i++){
            if(enemies[i].loc.first!=-1 && !enemies[i].isTrapped){
                TRACE("DIST: %d\n",enemies[i].distSq);
                switch(enemies[i].distSq){
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
                        if(!isImpassable(map[currLoc.first][currLoc.second-1])){
                            score[DIG_LEFT] += 30;
                            score[LEFT] = NEG_INF;
                        }
                    }
                    else if(currLoc.second<enemies[i].loc.second){
                        if(!isImpassable(map[currLoc.first][currLoc.second+1])){
                            score[DIG_RIGHT] += 30;
                            score[RIGHT] = NEG_INF;
                        }
                    }
                    else if (currLoc.first>enemies[i].loc.first){
                        if(!isImpassable(map[currLoc.first-1][currLoc.second])){
                            score[TOP] = NEG_INF;
                            score[BOTTOM] += 10;
                        }
                    }
                    else{
                        if(!isImpassable(map[currLoc.first+1][currLoc.second])){
                            score[BOTTOM] = NEG_INF;
                            score[TOP] += 10;
                        }
                    }
                    break;
                }
            }
        }
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

static bool isActionReversible(Action a, pair<int,int>& loc){
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
    return isSupported(loc) || (loc.first<15 && map[loc.first+1][loc.second]==REMOVED_BRICK);
}

struct info{
    pair<int,int> loc;
    int depth;
    Action startDir;
};

//figure out who an enemy is chasing
//and which direction it will chase in
pair<int,Action> computeChaseState(int enemyId){
    set<pair<int,int> >seen;
    queue<info> locs;
    info start;
    start.loc = enemies[enemyId].loc;
    if(!isSupported(start.loc)){
        return make_pair(NOONE,NONE);//can't really chase while falling...
    }
    start.depth = 0;
    start.startDir = NONE;
    seen.insert(start.loc);
    locs.push(start);
    while(!locs.empty()){
        info curr = locs.front();
        locs.pop();
        if((enemies[enemyId].master==NOONE && curr.depth>5) || curr.depth>8){
            return make_pair(NOONE,NONE);
        }
        if(curr.loc==currLoc){
            switch(enemies[enemyId].master){
            case RED:
                if(curr.depth>4){
                    break;
                }
                else{
                    return make_pair(RED,curr.startDir);
                }
            case BLUE:
                if(curr.depth>8){
                    //um.... shouldn't happen
                    WARN("WARN: Unreachable code reached!\n");
                    break;
                }
                else{
                    return make_pair(RED,curr.startDir);
                }
            case NOONE:
                if(curr.depth>5){
                    WARN("WARN: Unreachable code reached!\n");
                    break;
                }
                else{
                    return make_pair(RED,curr.startDir);
                }
            }
        }
        if(curr.loc==enemyLoc){
            switch(enemies[enemyId].master){
            case RED:
                if(curr.depth>8){
                    WARN("WARN: Unreachable code reached!\n");
                    break;
                }
                else{
                    return make_pair(BLUE,curr.startDir);
                }
            case BLUE:
                if(curr.depth>4){
                    break;
                }
                else{
                    return make_pair(BLUE,curr.startDir);
                }
            case NOONE:
                if(curr.depth>5){
                    WARN("WARN: Unreachable code reached!\n");
                    break;
                }
                else{
                    return make_pair(BLUE,curr.startDir);
                }
            }
        }
        else{
#define DO_BRANCH(dir)                                                  \
            if(canDoAction(dir,curr.loc) && isActionReversible(dir,curr.loc)){ \
                info next;                                              \
                next.depth = curr.depth+1;                              \
                next.loc = simulateAction(dir,curr.loc);                \
                next.startDir = curr.startDir;                          \
                if(next.startDir==NONE) next.startDir = dir;            \
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
    return make_pair(NOONE,NONE);
}
