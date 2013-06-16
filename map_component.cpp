#include "util.h"
#include "game_state.h"
#include "points.h"
#include "survival.h"
#include "points.h"
#include "map_component.h"

using namespace std;

void printReachableMatrix(int starti, int startj)
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

void printComponentMatrix()
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

void dfs(pair<int,int> start, pair<int,int> curr)
{
	if(reachable[start.first][start.second][curr.first][curr.second])
		return;
	reachable[start.first][start.second][curr.first][curr.second] = true;

	for(int i = 0; i < 7; i++)
		if(canDoAction2(static_cast<Action>(i), curr))
			dfs(start, simulateAction(static_cast<Action>(i), curr));

	return;
}

void computeAllWayReachability()
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
			pair<int,int> start = make_pair(i, j);
			dfs(start, start);
		}
	}
}

void assignComponents()
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

void findGoldInComponents()
{
	for(int i = 0; i < 600; i++)
		gold_comp[i] = 0;

	for(int i = 0; i < 16; i++)
		for(int j = 0; j < 25; j++)
			if(component[i][j] != -1)
				if(game.checkMapRaw(i,j) == GOLD)
					gold_comp[component[i][j]]++;
}

void findTotalGoldInMap()
{
	totalGoldOnMap = 0;
	for(int i = 0; i < 16; i++)
		for(int j = 0; j < 25; j++)
			if(game.checkMapRaw(i,j) == GOLD)
				totalGoldOnMap++;
}

//This function is not used anymore, and will be removed.
bool isSuicidal(Action action, pair<int,int> loc)
{
	pair<int,int> next = simulateAction(action, loc);
	while(next.first<15 && !isSolid(game.checkMapSafe(next.first+1,next.second)))
		next.first++;
	if(game.checkMapSafe(next) == REMOVED_BRICK)
		return true;
	return false;
}

int goldInCurrComponent(pair<int,int> loc)
{
	int comp = component[loc.first][loc.second];

	int gold = 0;
	for(int i = 0; i < 16; i++)
		for(int j = 0; j < 25; j++)
			if(component[i][j] == comp)
				if(game.checkMapRaw(i,j) == GOLD)
					gold++;
	return gold;
}

bool shouldSuicide(pair<int,int> loc)
{
	if((goldInCurrComponent(loc) == 0) && (gold_comp[component[loc.first][loc.second]] < (totalGoldOnMap / 2)))
		return true;
	return false;
}

Action getDirectionClosestToEnemy(pair<int,int> loc)
{
	//TODO: Fix this
	if(game.canDoActionPlayer(LEFT, loc))
		return LEFT;
	else if(game.canDoActionPlayer(RIGHT, loc))
		return RIGHT;
	return NONE;
}

Action getSuicidalMove(pair<int,int> loc)
{
	if(isSuicidal(LEFT, loc))
		return LEFT;
	if(isSuicidal(RIGHT, loc))
		return RIGHT;
	if(game.canDoActionPlayer(DIG_LEFT, loc))
		return DIG_LEFT;
	else if(game.canDoActionPlayer(DIG_RIGHT, loc))
		return DIG_RIGHT;
	else
		return getDirectionClosestToEnemy(loc);
}
