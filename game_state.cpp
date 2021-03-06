#include "game_state.h"
#include "util.h"

StaticWorldData fixedData;
World game;

char originalMap[16][26];
bool reachable[16][26][16][26];
int depth[16][26];
pair<int,int> earliest_parent[16][26];
int gold_comp[600];
int totalGoldOnMap;
int max_gold_comp;
int component[16][26];
vector<int > rlocs;
vector<int> clocs;
vector<Action> actions;
    
pair<int, int> simulateAction(Action act,const pair<int,int>& loc){
	if(!isAlive())
		return make_pair<int,int>(-1,-1);
	//Falling
	if(!isSupported(loc))
		return make_pair(loc.first+1,loc.second);
	switch(act){
	case NONE:
	case DIG_LEFT:
	case DIG_RIGHT:
		return loc;
	case LEFT:
		return make_pair(loc.first,loc.second-1);
	case RIGHT:
		return make_pair(loc.first,loc.second+1);
	case TOP:
		return make_pair(loc.first-1,loc.second);
	case BOTTOM:
		return make_pair(loc.first+1,loc.second);
	}

	return loc;
}

//this just has the change that if there is wall below, you can go down. used in finding traps, not in general.
bool canDoAction2(Action act,const pair<int,int>& loc){
	if(act != BOTTOM)
		return canDoAction(act, loc);

	return (!isSupported(loc)) 
		|| (canDoAction(LEFT, loc) && canDoAction(DIG_RIGHT, simulateAction(LEFT, loc)))
		|| (canDoAction(RIGHT, loc) && canDoAction(DIG_LEFT, simulateAction(RIGHT, loc)));

}

bool World::canDoActionRaw(Action act, const pair<int,int>& loc){
    switch(act){
    case NONE:
        return true;
    case LEFT:
        return isSupported(loc)
            && !isImpassable(checkMapSafe(loc.first,loc.second-1));
    case RIGHT:
        return isSupported(loc)
            && !isImpassable(checkMapSafe(loc.first,loc.second+1));
    case DIG_LEFT:
        return isSupported(loc)
            && checkMapSafe(loc.first+1,loc.second)!=FILLED_BRICK
            && checkMapSafe(loc.first+1,loc.second-1)==BRICK
            && checkMapSafe(loc.first,loc.second-1)!=BRICK
            && checkMapSafe(loc.first,loc.second-1)!=LADDER;
    case DIG_RIGHT:
        return isSupported(loc)
            && checkMapSafe(loc.first+1,loc.second)!=FILLED_BRICK
            && checkMapSafe(loc.first+1,loc.second+1)==BRICK
            && checkMapSafe(loc.first,loc.second+1)!=BRICK
            && checkMapSafe(loc.first,loc.second+1)!=LADDER;
    case TOP:
        return loc.first>0
            && isSupported(loc)//can't move while falling
            && map[loc.first][loc.second]==LADDER
            && !isImpassable(map[loc.first-1][loc.second]);
    case BOTTOM:
        return isSupported(loc)//can't move while falling
            && !isImpassable(checkMapSafe(loc.first+1,loc.second));
    }
    WARN("WARN: Unreachable code reached!\n");
    return false;//should be unreachable
}

bool World::canDoActionPlayer(Action act,const pair<int,int>& loc, int curBrickDelay){
    if(!isAlive() && act!=NONE)
        return false;
    if((act==DIG_LEFT || act==DIG_RIGHT) && curBrickDelay!=0)
        return false;
    return canDoActionRaw(act,loc);
}

bool World::canDoActionOpponent(Action act,const pair<int,int>& loc){
    if(enemyLoc.first!=-1 && act!=NONE)
        return false;
    if((act==DIG_LEFT || act==DIG_RIGHT) && enemyBrickDelay!=0)
        return false;
    return canDoActionRaw(act,loc);
}

bool World::canDoActionEnemy(Action act,const pair<int,int>& loc){
    switch(act){
    case NONE:
        return true;
    case LEFT:
        return isSupportedEnemy(loc)
            && !isImpassable(checkMapSafe(loc.first,loc.second-1));
    case RIGHT:
        return isSupportedEnemy(loc)
            && !isImpassable(checkMapSafe(loc.first,loc.second+1));
    case DIG_LEFT:
    case DIG_RIGHT:
        return false;
    case TOP:
        return loc.first>0
            && isSupportedEnemy(loc)//can't move while falling
            && map[loc.first][loc.second]==LADDER
            && !isImpassable(map[loc.first-1][loc.second]);
    case BOTTOM:
        return isSupportedEnemy(loc)//can't move while falling
            && !isImpassable(checkMapSafe(loc.first+1,loc.second));
    }
    WARN("WARN: Unreachable code reached!\n");
    return false;//should be unreachable
}
