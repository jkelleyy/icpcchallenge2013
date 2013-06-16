
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

static inline void saveFirstMap()
{
	for(int i = 0; i < 16; i++)
		for(int j = 0; j < 25; j++)
			fixedData.baseMap[i][j] = game.checkMapRaw(i,j);
	for(int i = 0; i < 16; i++)
		for(int j = 0; j < 25; j++)
		{
			game.timeout[i][j] =0;
			originalMap[i][j] = game.checkMapRaw(i,j);
		}
}

static inline void initGame(){
    ourLastMove = NONE;
    scanf(" %d\n",&fixedData.nrounds);
    readMap();
    int loc_first,loc_second;
    //portability
    scanf(" %d %d ",&loc_first,&loc_second);
    game.currLoc.first = loc_first;
    game.currLoc.second = loc_second;
    //scanf(" %hhd %hhd ",&game.currLoc.first,&game.currLoc.second);
    fixedData.ourSpawn = game.currLoc;
    game.brickDelay = 0;
    scanf(" %d %d ",&loc_first,&loc_second);
    game.enemyLoc.first = loc_first;
    game.enemyLoc.second = loc_second;
    //scanf(" %hhd %hhd ",&game.enemyLoc.first,&game.enemyLoc.second);
    fixedData.enemySpawn = game.enemyLoc;
    game.enemySpawnDelay = 0;
    game.enemyBrickDelay = 0;
    scanf(" %d\n",&fixedData.nenemies);
    game.enemies.resize(fixedData.nenemies);
    for(int i=0;i<fixedData.nenemies;i++){
        SharedEnemyInfo* sinfo = new SharedEnemyInfo;
        game.enemies[i].info = sinfo;
        scanf(" %d %d %s",&loc_first,&loc_second,tempBuffer);
        sinfo->spawn.first = loc_first;
        sinfo->spawn.second = loc_second;
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
        game.enemies[i].patrolIndex = 0;
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
    int loc_first,loc_second;
    scanf(" %d %d %d",&loc_first,&loc_second,&master);
    newLoc.first = loc_first;
    newLoc.second = loc_second;
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
    TRACE("NEXT TURN: %d\n",nextTurn);
    if(nextTurn==-1)
        return false;
    TRACE("TRACE: Starting turn #%d\n",nextTurn);
    game.missedTurns = nextTurn - game.currTurn -1;
    if(game.missedTurns!=0)
        WARN("WARNING: Lost Turn(s)! (%d turns lost)\n",game.missedTurns);
    game.currTurn = nextTurn;
    readMap();

    setSquareDelays();
    int temp;
    int loc_first,loc_second;
    scanf(" %d %d %d %d",&loc_first,&loc_second,&currScore,&temp);
    game.currLoc.first = loc_first;
    game.currLoc.second = loc_second;
    game.brickDelay = temp;
    scanf(" %d %d %d %d",&loc_first,&loc_second,&enemyScore,&temp);
    game.enemyLoc.first = loc_first;
    game.enemyLoc.second = loc_second;
    game.enemyBrickDelay = temp;
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
    double survivalScore[7];
    memset(survivalScore,0,sizeof(double)*7);
    predict(survivalScore);


    //TODO add points score
    vector<state> states = pointsScore(5);
    //for(int i =1; i <states.size();i++)
//	    survivalScore[states[i].first]+=100/states[i].depth*i;

   	//state s = states.size()-1;
    state s= states.size()>1?states[1]:states[0];
    TRACE("State 1 DIST: %d\n",s.depth);
    int sd = 1;
    for(int i =2; i <states.size();i++)
    {
    		TRACE("TRACE: CONSIDER Action: %s pos: %d %d depth: %d\n",actionNames[static_cast<int>(states[i].first)],states[i].pos.first,states[i].pos.second,states[i].depth);
	    if(states[i].depth-s.depth<10){// && states[i].cost<=s.cost)){// &&states[i].cost<=s.cost) || states[i].cost<s.cost)
		    s = states[i];
		    sd = i;
	    }
	    else
		    break;

    }
    if(s.first==DIG_LEFT || s.first==DIG_RIGHT)
	    TRACE("ORDERED TO DIG!\n");
    TRACE("TRACE: Action: %s pos: %d %d depth: %d\n",actionNames[static_cast<int>(s.first)],s.pos.first,s.pos.second,s.depth);
    //if(s.first!=NONE)
        survivalScore[s.first]+=20;

    vector<Action> bests;
    double maxScore = 0;
#define SCORE_EPSILON 1
    for(int i=NONE;i<7;i++){
        if(survivalScore[i]>maxScore+SCORE_EPSILON){
            maxScore = survivalScore[i];
            bests.clear();
            bests.push_back(static_cast<Action>(i));
        }
        else if(survivalScore[i]>=maxScore-SCORE_EPSILON){
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

	if(shouldSuicide(game.currLoc))
	{
		TRACE("saurabh says dieeeeee\n");
		a = getSuicidalMove(game.currLoc);
	}
    ourLastMove = a;
    act(a);
    TRACE("TRACE: Turn #%d finished with action %s with a score of %f\n",game.currTurn,actionNames[a],maxScore);

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
