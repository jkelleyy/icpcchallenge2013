
#include "util.h"
#include "game_state.h"
#include "points.h"
#include "survival.h"
#include "points.h"
#include "map_component.h"
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cassert>

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

static inline void saveFirstMap()
{
	for(int i = 0; i < 16; i++)
		for(int j = 0; j < 26; j++)
			originalMap[i][j] = map[i][j];
	
}

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
        enemies[i].program.resize(strlen(tempBuffer));
        for(int j=0;j<enemies[i].program.size();j++){
            switch(tempBuffer[j]){
            case 'R':
                enemies[i].program[j] = RIGHT;
                break;
            case 'L':
                enemies[i].program[j] = LEFT;
                break;
            case 'T':
                enemies[i].program[j] = TOP;
                break;
            case 'B':
                enemies[i].program[j] = BOTTOM;
                break;
            }
        }
        enemies[i].master = NOONE;
        enemies[i].isTrapped = false;
        enemies[i].spawnDelay = 0;
        enemies[i].distSq = distSq(enemies[i].loc,currLoc);
        enemies[i].distSqToOpponent = distSq(enemies[i].loc,enemyLoc);
    }
    currTurn = -1;
    currScore = 0;
    enemyScore = 0;
	computeAllWayReachability();
	assignComponents();
	findTotalGoldInMap();
	findGoldInComponents();
	printComponentMatrix();

	TRACE("Total gold = %d\n", totalGoldOnMap);
	for(int i = 0; i < 85; i++)
		if(gold_comp[i] > 1)
			TRACE("Gold in %d = %d\n", i, gold_comp[i]);
	saveFirstMap();
}

static inline void act(Action act){
    puts(actionNames[act]);
    fflush(stdout);//don't remove this!
}

//process the input for one of the enemies
static void processEnemy1(int i){
    pair<int,int> oldLoc = enemies[i].loc;
    int master;
    scanf(" %d %d %d",&enemies[i].loc.first,&enemies[i].loc.second,&master);
    enemies[i].master = static_cast<Player>(master);
    if(enemies[i].loc.first!=-1 && oldLoc.first!=-1 && oldLoc!=enemies[i].loc){
        if(enemies[i].loc.first<oldLoc.first){
            enemies[i].lastMove = TOP;
        }
        else if (enemies[i].loc.first>oldLoc.first){
            //even if it's falling we should interpret that as a "BOTTOM"
            //since that's what the enemies program does
            enemies[i].lastMove = BOTTOM;
        }
        else if(enemies[i].loc.second<oldLoc.second){
            enemies[i].lastMove = LEFT;
        }
        else{
            enemies[i].lastMove = RIGHT;
        }
    }
    else{
        enemies[i].lastMove = NONE;
    }
    if(enemies[i].loc.first!=-1 && (map[enemies[i].loc.first][enemies[i].loc.second]==REMOVED_BRICK || map[enemies[i].loc.first][enemies[i].loc.second]==FILLED_BRICK)){
        map[enemies[i].loc.first][enemies[i].loc.second]=FILLED_BRICK;
        enemies[i].isTrapped = true;
    }
    else{
        enemies[i].isTrapped = false;
    }

}
static void processEnemy2(int i){
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
        enemies[i].patrolIndex = 0;
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
        pair<int,ChaseInfo> info = computeChaseState(i);
        enemies[i].chaseInfo = info.second;
        switch(enemies[i].chaseState){
        case CHASE_BLUE:
        case CHASE_RED:
            if(enemies[i].lastMove!=NONE)
                enemies[i].chaseStack.push_front(enemies[i].lastMove);
            break;
        case PATROL:
            if(enemies[i].lastMove!=NONE){
                enemies[i].patrolIndex++;
                enemies[i].patrolIndex %= enemies[i].program.size();
            }
            break;
        case RETURN_TO_PATROL:
            //assert(!enemies[i].chaseStack.empty());
            if(enemies[i].lastMove!=NONE)
                enemies[i].chaseStack.pop_front();
            if(enemies[i].chaseStack.empty())
                enemies[i].chaseState = PATROL;

            break;
        }

        switch(info.first){
        case RED:
            enemies[i].chaseState = CHASE_RED;
            break;
        case BLUE:
            enemies[i].chaseState = CHASE_BLUE;
            break;
        case NOONE:
            switch(enemies[i].chaseState){
            case CHASE_RED:
            case CHASE_BLUE:
                if(enemies[i].chaseStack.empty())
                    enemies[i].chaseState = PATROL;
                else
                    enemies[i].chaseState = RETURN_TO_PATROL;
                break;
            }

        }
    }
}

static void setSquareDelays()
{
	for(int i = 0; i < 16; i++)
		for(int j = 0; j < 26; j++)
			if(originalMap[i][j]==GOLD && game.map[i][j] !=GOLD)
			{
				if(game.timeout[i][j]==0)
					game.timeout[i][j]= 150;
				else
					game.timeout[i][j]--;
			}
			else if (originalMap[i][j]==BRICK && game.map[i][j]!=BRICK)
			{
				if(game.timeout[i][j]==0)
					game.timeout[i][j]=25;
				else
					game.timeout[i][j]--;
			}
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
   
	setSquareDelays();   
    
    scanf(" %d %d %d %d",&currLoc.first,&currLoc.second,&currScore,&brickDelay);
    scanf(" %d %d %d %d",&enemyLoc.first,&enemyLoc.second,&enemyScore,&enemyBrickDelay);
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
    //these two loops MUST be separate, the second depends on the first finishing
    for(int i=0;i<nenemies;i++){
        processEnemy1(i);
    }
    for(int i=0;i<nenemies;i++){
        processEnemy2(i);
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

    vector<state> states = pointsScore(5);
    state s= states.size()>1?states[1]:states[0];
    TRACE("State 1 DIST: %d\n",s.depth);
    for(int i =2; i <states.size();i++)
    {
	    TRACE("STATE: %d DIST %d\n",i,states[i].depth);
	    if(states[i].depth-states[i-1].depth<=5)
		    s = states[i];
	    else
		    break;

    }
    if(s.first==DIG_LEFT || s.first==DIG_RIGHT)
	    TRACE("ORDERED TO DIG!\n");
    TRACE("TRACE: Action: %d pos: %d %d depth: %d\n",static_cast<int>(s.first),s.pos.first,s.pos.second,s.depth);
    if(s.first!=NONE)
        survivalScore[s.first]+=500;

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

	if(shouldSuicide(currLoc))
	{
		TRACE("saurabh says dieeeeee\n");
		a = getSuicidalMove(currLoc);
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
    return 0;
}
