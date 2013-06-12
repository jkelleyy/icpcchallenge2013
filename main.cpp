
#include "util.h"
#include "game_state.h"
#include "survival.h"
#include "points.h"
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

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

static inline void printReachableMatrix()
{
	TRACE("Reachable matrix:\n");
	for(int i = 0; i < 16; i++)
	{
		TRACE("reachable ");
		for(int j = 0; j < 26; j++)
			if(reachable[i][j])
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
		for(int j = 0; j < 26; j++)
			TRACE("%2d ", component[i][j]);
		TRACE("\n");
	}
}

static inline void printParentDepthMatrix()
{
	TRACE("Depth matrix:\n");
	for(int i = 0; i < 16; i++)
	{
		TRACE("depth_matrix ");
		for(int j = 0; j < 26; j++)
		{
			if(depth[i][j] == POS_INF)
				TRACE(" - ");
			else
				TRACE("%2d ", depth[i][j]);
		}
		TRACE("\n");
	}
}

static inline void printEarliestParents()
{
	TRACE("Earliest Parent matrix:\n");
	for(int i = 0; i < 16; i++)
	{
		TRACE("earliest_parent ");
		for(int j = 0; j < 26; j++)
			TRACE("%2d,%2d ", earliest_parent[i][j].first, earliest_parent[i][j].second);
		TRACE("\n");
	}
}

static inline void dfs(pair<int,int> curr, int curr_comp)
{
	int curr_x = curr.first;
	int curr_y = curr.second;

	if(reachable[curr_x][curr_y])
		return;
	
	reachable[curr_x][curr_y] = true;
	depth[curr_x][curr_y] = curr_comp;

	int most_gold = 0;
	for(int i = 0; i < 7; i++)
	{
		if(canDoAction2(static_cast<Action>(i), curr))
		{
			pair<int,int> next = simulateAction(static_cast<Action>(i), curr);
			dfs(next, ++curr_comp);
			pair<int,int> parent_ = earliest_parent[next.first][next.second];
			if(depth[curr_x][curr_y] > depth[parent_.first][parent_.second])
			{
				depth[curr_x][curr_y] = depth[parent_.first][parent_.second];
				earliest_parent[curr.first][curr.second] = make_pair(parent_.first, parent_.second);
			}
		}
	}

	return;
}

static inline pair<int,int> getEarliestParents(int i, int j)
{
	if((earliest_parent[i][j].first == i) && (earliest_parent[i][j].second == j))
		return earliest_parent[i][j]; //make_pair(i, j);
	earliest_parent[i][j] = getEarliestParents(earliest_parent[i][j].first, earliest_parent[i][j].second);

	return earliest_parent[i][j];
}

static inline void findComponents()
{
	totalGoldOnMap = 0;
	for(int i = 0; i < 16; i++)
	{
		for(int j = 0; j < 26; j++)
		{
			reachable[i][j] = false;
			component[i][j] = -1;
			depth[i][j] = POS_INF;
			earliest_parent[i][j] = make_pair(i, j);
			if(map[i][j] == GOLD)
				totalGoldOnMap++;
		}
	}
	
	dfs(ourSpawn, 0);
	TRACE("saurabh: dfs done");
	printParentDepthMatrix();
	
	for(int i = 0; i < 16; i++)
		for(int j = 0; j < 26; j++)
			earliest_parent[i][j] = getEarliestParents(i, j);
	for(int i = 0; i < 16; i++)
		for(int j = 0; j < 26; j++)
			earliest_parent[i][j] = getEarliestParents(i, j);
	for(int i = 0; i < 16; i++)
		for(int j = 0; j < 26; j++)
			earliest_parent[i][j] = getEarliestParents(i, j);
	for(int i = 0; i < 16; i++)
		for(int j = 0; j < 26; j++)
			earliest_parent[i][j] = getEarliestParents(i, j);
	//printEarliestParents();
	
	for(int i = 0; i < 16; i++)
		for(int j = 0; j < 26; j++)
			depth[i][j] = depth[earliest_parent[i][j].first][earliest_parent[i][j].second];
	printParentDepthMatrix();
	
	int count[600];
	for(int i = 0; i < 600; i++)
		count[i] = 0;
	for(int i = 0; i < 16; i++)
		for(int j = 0; j < 26; j++)
			if(depth[i][j] != POS_INF)
				count[depth[i][j]]++;
	
	int compo = -1;
	int temp_map[600];
	for(int i = 0; i < 600; i++)
	{
		if(count[i] > 1)
			compo++;
		temp_map[i] = compo;
	}
	
	for(int i = 0; i < 16; i++)
		for(int j = 0; j < 26; j++)
			if(depth[i][j] != POS_INF)
				component[i][j] = temp_map[depth[i][j]];
			else
				component[i][j] = -1;
	TRACE("component calc done\n");
	
	printComponentMatrix();

	for(int i = 0; i < 600; i++)
		gold_comp[i] = 0;

	for(int i = 0; i < 16; i++)
		for(int j = 0; j < 26; j++)
			if(map[i][j] == GOLD)
				gold_comp[component[i][j]]++;
	
	for(int i = 0; i < 600; i++)
		if(gold_comp[i] > gold_comp[max_gold_comp])
			max_gold_comp = i;
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
        enemies[i].program = string(tempBuffer);
        enemies[i].master = NOONE;
        enemies[i].isTrapped = false;
        enemies[i].spawnDelay = 0;
        enemies[i].distSq = distSq(enemies[i].loc,currLoc);
        enemies[i].distSqToOpponent = distSq(enemies[i].loc,enemyLoc);
    }
    currTurn = -1;
    currScore = 0;
    enemyScore = 0;
	
	findComponents();
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
            if(isSupported(oldLoc))
                enemies[i].lastMove = BOTTOM;
            else
                enemies[i].lastMove = NONE;
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
        switch(info.first){
        case RED:
            enemies[i].chaseState = CHASE_RED;
            break;
        case BLUE:
            enemies[i].chaseState = CHASE_BLUE;
            break;
        case NOONE:
            //TODO deal with return to patrol
            switch(enemies[i].chaseState){
            case CHASE_RED:
            case CHASE_BLUE:
                enemies[i].chaseState = RETURN_TO_PATROL;
                break;
            case RETURN_TO_PATROL:

                break;
            }

        }
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

    state s = pointsScore();
    TRACE("TRACE: Action: %d pos: %d %d depth: %d\n",static_cast<int>(s.first),s.pos.first,s.pos.second,s.depth);
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
