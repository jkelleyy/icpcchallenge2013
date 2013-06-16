#include "game_state.h"
#include "points.h"
#include "util.h"

#include <queue>

using namespace std;

//The number of gold pieces you can look for, max
const int maxGold = 10;

typedef long long ll;
ll visited[16][26];
//InitialGoldNumbers
ll goldNumber[16][26];
COWMap<16,25> mapCopy;
PointsWorld w;
ll digLeft = ((ll)1)<<63;
ll digRight = ((ll)1)<<62;

void updateDelay(PointsWorld& w)
{
	for(int i =0; i<16; i++)
		for(int j = 0; j <26; j++)
		{
			if(w.timeout[i][j]==1)
				w.map.lookup(i,j)=originalMap[i][j];
			if(w.timeout[i][j]>0)
				w.timeout[i][j]--;
		}
}

void updateMapCopy(PointsWorld& w)
{
    mapCopy = w.map;
    //for(int i =0; i<16; i++)
    //for(int j = 0; j <25; j++)
    //mapCopy[i][j] = w.checkMapRaw(i,j);
}

void restoreMapCopy(PointsWorld& w)
{
    w.map = mapCopy;
    //for(int i =0; i<16; i++)
    //for(int j = 0; j <25; j++)
    //w.map.lookup(i,j) = mapCopy[i][j];
}

void changeMap(PointsWorld& w, vector<dugCell> dugCells)
{
	for(int i =0; i <dugCells.size();i++)
	{
		if(w.currTurn-dugCells[i].timeDug<=25)
		{
			//TRACE("DUG AT: %d %d %d\n",dugCells[i].loc.first,dugCells[i].loc.second,dugCells[i].timeDug);
			w.map.lookup((dugCells[i]).loc.first,(dugCells[i]).loc.second) = REMOVED_BRICK;
		}
	}
}


vector<state> pointsScore(int desiredGold){
	int turnNo = game.currTurn;
	desiredGold = desiredGold>maxGold?maxGold:desiredGold;


	for(int i =0; i<16; i++)
		for(int j = 0; j <25; j++)
		{
			w.map.lookup(i,j) = game.checkMapRaw(i,j);
			w.timeout[i][j] = game.timeout[i][j];
		}
	w.currTurn = game.currTurn;


	//map out all gold.
	if(turnNo ==0){
		int goldSoFar = 1;
		for(int i =0; i<16; i++)
			for(int j = 0; j <25; j++)
				if(game.checkMapRaw(i,j)==GOLD){
					goldNumber[i][j] = 1<<goldSoFar;
					goldSoFar++;
					goldSoFar%=61;
				}
	}

	queue<state> q;
	state currState;
	currState.pos = game.currLoc;
	currState.first = NONE;
	currState.depth = 0;
	currState.goldNumber = 1;
	currState.numGold = 0;
	currState.digDelay = game.brickDelay;
	//The best state to get X pieces of gold (X is index)
	vector<state> best;
	best.push_back(currState);

	if(!game.isAlive())
		return best;
	for(int i =0; i<16; i++)
		for(int j = 0; j <25; j++)
			visited[i][j] = 0;

	updateMapCopy(w);
	q.push(currState);

	int lastDepthSeen = 0;

	while(!q.empty()){
		state newState = q.front();q.pop();
		loc_t cur = newState.pos;
		if(newState.depth > lastDepthSeen)
		{
			lastDepthSeen++;
			updateDelay(w);
			updateMapCopy(w);
			w.currTurn++;
		}

		changeMap(w, newState.dugCells);

		if(w.checkMapRaw(cur)==GOLD && (!(newState.goldNumber&goldNumber[cur.first][cur.second])))
		{
			//TRACE("NUMGOLD %d\n",newState.numGold);
			//TRACE("GOLDNUM %lld\n",newState.goldNumber);
			newState.numGold++;
			newState.goldNumber |= goldNumber[cur.first][cur.second];
			if(newState.numGold==desiredGold)
			{
				best.push_back(newState);
				return best;
			}
			else if (best.size()<=newState.numGold)
				best.push_back(newState);
		}


		for(int i=NONE; i<7;i++){
			Action a = static_cast<Action>(i);
			if(w.canDoActionPlayer(a,cur,newState.digDelay)){
				loc_t alteredLoc = simulateAction(a,cur);
				//if(a==DIG_LEFT || a==DIG_RIGHT)
				//	TRACE("Consid Might digg!\n");
				ll newGoldNum  = newState.goldNumber;
				if(a==DIG_LEFT)
					newGoldNum  |= digLeft;
				if(a==DIG_RIGHT)
					newGoldNum |=digRight;
				if((visited[alteredLoc.first][alteredLoc.second] &  newGoldNum) != newGoldNum){

					state alteredState;
					alteredState.pos = alteredLoc;
					alteredState.depth = newState.depth+1;
					alteredState.goldNumber = newGoldNum;
					alteredState.numGold = newState.numGold;
					alteredState.dugCells = newState.dugCells;

					visited[alteredLoc.first][alteredLoc.second] |= newGoldNum;

					if(a!=DIG_LEFT && a!=DIG_RIGHT){
						alteredState.digDelay = newState.digDelay==0?0:(newState.digDelay-1);
					}
					else
					{
						//TRACE("Considering digging!!!\n");
						alteredState.digDelay = 25;
						dugCell d;
						d.timeDug = w.currTurn;
						if(a==DIG_LEFT)
							d.loc = make_pair(alteredLoc.first+1,alteredLoc.second-1);
						else
							d.loc = make_pair(alteredLoc.first+1,alteredLoc.second+1);
						alteredState.dugCells.push_back(d);
					}

					if(alteredState.depth==1)
					{
						//if(a==DIG_LEFT || a==DIG_RIGHT)
						//	TRACE("FIRST ACTION DIG\n");
						alteredState.first = static_cast<Action>(i);
					}
					else
						alteredState.first = newState.first;
					q.push(alteredState);
				}
			}
		}

		restoreMapCopy(w);
	}

	return best;
}
