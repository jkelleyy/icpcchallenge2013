
#include "util.h"
#include "game_state.h"
#include "points.h"
#include "survival.h"
#include "points.h"
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cassert>

using namespace std;

static inline void readMap(){
    for(int r=0;r<16;r++){
        char buffer[27];
        fgets(buffer,27,stdin);
        if(buffer[0]!=EMPTY &&
           buffer[0]!=LADDER &&
           buffer[0]!=BRICK &&
           buffer[0]!=GOLD &&
           buffer[0]!=REMOVED_BRICK)
            r--;
        else
            for(int c=0;c<25;c++)
                game.map.lookup(r,c) = buffer[c];
    }
}

static char tempBuffer[1000];

static inline void printReachableMatrix(int starti, int startj)
{
	TRACE("Reachable matrix:\n");
	for(int i = 0; i < 16; i++)
	{
		TRACE("reachable ");
		for(int j = 0; j < 25; j++)
			if(reachable[starti][startj][i][j])
				TRACE("1 ");
			else
				TRACE("0 ");
		TRACE("\n");
	}
}

static inline void printComponentMatrix()
{
	TRACE("Component matrix:\n");
	for(int i = 0; i < 16; i++)
	{
		TRACE("comp ");
		for(int j = 0; j < 25; j++)
			TRACE("%2d ", component[i][j]);
		TRACE("\n");
	}
}

static inline void dfs(loc_t start, loc_t curr)
{
	if(reachable[start.first][start.second][curr.first][curr.second])
		return;
	reachable[start.first][start.second][curr.first][curr.second] = true;

	for(int i = 0; i < 7; i++)
		if(canDoAction2(static_cast<Action>(i), curr))
			dfs(start, simulateAction(static_cast<Action>(i), curr));

	return;
}

static inline void computeAllWayReachability()
{
	for(int i = 0; i < 16; i++)
		for(int j = 0; j < 25; j++)
			for(int ii = 0; ii < 16; ii++)
				for(int jj = 0; jj < 25; jj++)
					reachable[i][j][ii][jj] = false;

	for(int i = 0; i < 16; i++)
	{
		for(int j = 0; j < 25; j++)
		{
			loc_t start = make_pair(i, j);
			dfs(start, start);
		}
	}
}

static inline void assignComponents()
{
	for(int i = 0; i < 16; i++)
		for(int j = 0; j < 25; j++)
			component[i][j] = -1;

	int comp = 0;
	for(int i = 0; i < 16; i++)
	{
		for(int j = 0; j < 25; j++)
		{
			if(component[i][j] == -1)
				component[i][j] = comp++;
			for(int ii = 0; ii < 16; ii++)
				for(int jj = 0; jj < 25; jj++)
					if((reachable[i][j][ii][jj]) && (reachable[ii][jj][i][j]))
						component[ii][jj] = component[i][j];
		}
	}
}

static inline void findGoldInComponents()
{
	for(int i = 0; i < 600; i++)
		gold_comp[i] = 0;

	for(int i = 0; i < 16; i++)
		for(int j = 0; j < 25; j++)
			if(component[i][j] != -1)
				if(game.checkMapRaw(i,j) == GOLD)
					gold_comp[component[i][j]]++;
}

static inline void findTotalGoldInMap()
{
	totalGoldOnMap = 0;
	for(int i = 0; i < 16; i++)
		for(int j = 0; j < 25; j++)
			if(game.checkMapRaw(i,j) == GOLD)
				totalGoldOnMap++;
}

static inline void saveFirstMap()
{
	for(int i = 0; i < 16; i++)
		for(int j = 0; j < 25; j++)
			fixedData.baseMap[i][j] = game.checkMapRaw(i,j);

}

static inline void initGame(){
    scanf(" %d\n",&fixedData.nrounds);
    readMap();
    scanf(" %hhd %hhd ",&game.currLoc.first,&game.currLoc.second);
    fixedData.ourSpawn = game.currLoc;
    game.brickDelay = 0;
    scanf(" %hhd %hhd ",&game.enemyLoc.first,&game.enemyLoc.second);
    fixedData.enemySpawn = game.enemyLoc;
    game.enemySpawnDelay = 0;
    game.enemyBrickDelay = 0;
    scanf(" %d\n",&fixedData.nenemies);
    game.enemies.resize(fixedData.nenemies);
    for(int i=0;i<fixedData.nenemies;i++){
        SharedEnemyInfo* sinfo = new SharedEnemyInfo;
        game.enemies[i].info = sinfo;
        scanf(" %hhd %hhd %s",&sinfo->spawn.first,&sinfo->spawn.second,tempBuffer);
        game.enemies[i].setLoc(game.enemies[i].info->spawn);
        sinfo->program.resize(strlen(tempBuffer));
        for(int j=0;j<game.enemies[i].info->program.size();j++){
            switch(tempBuffer[j]){
            case 'R':
                sinfo->program[j] = RIGHT;
                break;
            case 'L':
                sinfo->program[j] = LEFT;
                break;
            case 'T':
                sinfo->program[j] = TOP;
                break;
            case 'B':
                sinfo->program[j] = BOTTOM;
                break;
            }
        }
        game.enemies[i].setMaster(NOONE);
        game.enemies[i].setIsTrapped(false);
        game.enemies[i].setSpawnDelay(0);
        game.enemies[i].setChaseState(PATROL);
    }
    game.currTurn = -1;
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
    loc_t oldLoc = game.enemies[i].getLoc();
    loc_t newLoc;
    int master;
    scanf(" %hhd %hhd %d",&newLoc.first,&newLoc.second,&master);
    game.enemies[i].setLoc(newLoc);
    game.enemies[i].setMaster(static_cast<Player>(master));
    if(newLoc.first!=-1 && oldLoc.first!=-1 && oldLoc!=newLoc){
        if(newLoc.first<oldLoc.first){
            game.enemies[i].setLastMove(TOP);
        }
        else if (newLoc.first>oldLoc.first){
            //even if it's falling we should interpret that as a "BOTTOM"
            //since that's what the enemies program does
            game.enemies[i].setLastMove(BOTTOM);
        }
        else if(newLoc.second<oldLoc.second){
            game.enemies[i].setLastMove(LEFT);
        }
        else{
            game.enemies[i].setLastMove(RIGHT);
        }
    }
    else{
        game.enemies[i].setLastMove(NONE);
    }
    if(game.checkMapSafe(newLoc)==REMOVED_BRICK || game.checkMapSafe(newLoc)==FILLED_BRICK){
        game.map.lookup(newLoc)=FILLED_BRICK;
        game.enemies[i].setIsTrapped(true);
    }
    else{
        game.enemies[i].setIsTrapped(false);
    }

}
static void processEnemy2(int i){
    if(!game.enemies[i].isAlive()){
        if(game.enemies[i].getSpawnDelay()==0){
            game.enemies[i].setSpawnDelay(max(24-game.missedTurns,1));
        }
        else{
            game.enemies[i].setSpawnDelay(max(game.enemies[i].getSpawnDelay()-1,1));
        }
        game.enemies[i].setChaseState(PATROL);
        game.enemies[i].patrolIndex = 0;
    }
    else{
        game.enemies[i].setSpawnDelay(0);
        switch(game.enemies[i].getChaseState()){
        case CHASE_BLUE:
        case CHASE_RED:
            if(game.enemies[i].getLastMove()!=NONE)
                game.enemies[i].chaseStack.push(game.enemies[i].getLastMove());
            break;
        case PATROL:
            if(game.enemies[i].getLastMove()!=NONE){
                game.enemies[i].patrolIndex++;
                game.enemies[i].patrolIndex %= game.enemies[i].info->program.size();
            }
            break;
        case RETURN_TO_PATROL:
            //assert(!enemies[i].chaseStack.empty());
            if(game.enemies[i].getLastMove()!=NONE)
                game.enemies[i].chaseStack.pop();
            if(game.enemies[i].chaseStack.empty())
                game.enemies[i].setChaseState(PATROL);
            break;
        }
        ChaseInfo info = computeChase(game,i);
        switch(info.target){
        case RED:
            game.enemies[i].setChaseState(CHASE_RED);
            break;
        case BLUE:
            game.enemies[i].setChaseState(CHASE_BLUE);
            break;
        case NOONE:
            switch(game.enemies[i].getChaseState()){
            case CHASE_RED:
            case CHASE_BLUE:
                if(game.enemies[i].chaseStack.empty())
                    game.enemies[i].setChaseState(PATROL);
                else
                    game.enemies[i].setChaseState(RETURN_TO_PATROL);
                break;
            }

        }
    }
}

static void setSquareDelays()
{
	for(int i = 0; i < 16; i++)
		for(int j = 0; j < 25; j++)
			if(originalMap[i][j]==GOLD && game.checkMapRaw(i,j) !=GOLD)
			{
				if(game.timeout[i][j]==0)
					game.timeout[i][j]= 150;
				else
					game.timeout[i][j]--;
			}
			else if (originalMap[i][j]==BRICK && game.checkMapRaw(i,j)!=BRICK)
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
    game.missedTurns = nextTurn - game.currTurn -1;
    if(game.missedTurns!=0)
        WARN("WARNING: Lost Turn(s)! (%d turns lost)\n",game.missedTurns);
    game.currTurn = nextTurn;
    readMap();

	//setSquareDelays();

    scanf(" %hhd %hhd %d %hhd",&game.currLoc.first,&game.currLoc.second,&currScore,&game.brickDelay);
    scanf(" %hhd %hhd %d %hhd",&game.enemyLoc.first,&game.enemyLoc.second,&enemyScore,&game.enemyBrickDelay);
    if(game.enemyLoc.first==-1){
        if(game.enemySpawnDelay==0){
            game.enemySpawnDelay = max(49-game.missedTurns,1);
        }
        else{
            game.enemySpawnDelay--;
            if(game.enemySpawnDelay==0)
                game.enemySpawnDelay=1;
        }
    }
    else{
        game.enemySpawnDelay = 0;
    }
    //these two loops MUST be separate, the second depends on the first finishing
    for(int i=0;i<fixedData.nenemies;i++){
        processEnemy1(i);
    }
    for(int i=0;i<fixedData.nenemies;i++){
        processEnemy2(i);
    }

    for(int i=0;i<16;i++){
        TRACE("MAP: ");
        for(int j=0;j<25;j++){
            TRACE("%c",game.checkMapRaw(i,j));
        }
        TRACE("\n");
    }

    TRACE("POS: %d %d\n",game.currLoc.first,game.currLoc.second);

    //actual ai starts here
    int survivalScore[7];
    memset(survivalScore,0,sizeof(int)*7);
    predict(survivalScore);


    //TODO add points score
    bool hasChaser = false;
    for(int i=0;i<fixedData.nenemies;i++){
        //if(game.enemies[i].getChaseState()==CHASE_RED && game.enemies[i].chaseInfo.pathLength<5){
        //    hasChaser = true;
        //    break;
        //}
    }

    vector<state> states = pointsScore(5);
    state s= states.size()>1?states[1]:states[0];
    TRACE("State 1 DIST: %d\n",s.depth);
    for(int i =2; i <states.size();i++)
    {
	    TRACE("STATE: %d DIST %d\n",i,states[i].depth);
        if((s.first==DIG_LEFT || s.first==DIG_RIGHT) && hasChaser){
            s = states[i];
        }
	    else if(states[i].depth-states[i-1].depth<=5)
		    s = states[i];
	    else
		    break;

    }
    if(s.first==DIG_LEFT || s.first==DIG_RIGHT){
	    TRACE("ORDERED TO DIG!\n");
        if(hasChaser){
            s.first = NONE;
        }
    }
    TRACE("TRACE: Action: %s pos: %d %d depth: %d\n",actionNames[s.first],s.pos.first,s.pos.second,s.depth);
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
    TRACE("TRACE: Turn #%d finished with action %s with a score of %d\n",game.currTurn,actionNames[a],maxScore);

    return true;
}

int main(){
    srand(time(NULL));
    initSurvival();
    initGame();
    while(doTurn());
    TRACE("Game Finished!\n");
    //printf("%ld\n",sizeof(World));
    //printf("%ld\n",sizeof(vector<EnemyInfo>));
    //printf("%ld\n",sizeof(EnemyInfo));
    return 0;
}
