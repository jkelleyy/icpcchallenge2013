#include "game_state.h"
#include "points.h"
#include "util.h"

#include <queue>

using namespace std;

typedef pair<int,int> loc;
//The number of gold pieces you can look for, max
const int maxGold = 10;

typedef long long ll;
ll visited[16][26];
//InitialGoldNumbers
ll goldNumber[16][26];


vector<state> pointsScore(int desiredGold, int turnNo){
	desiredGold = desiredGold>maxGold?maxGold:desiredGold;


	//map out all gold.
	if(turnNo ==0){
		int goldSoFar = 1;
		for(int i =0; i<16; i++)
			for(int j = 0; j <26; j++)
				if(map[i][j]==GOLD){
					goldNumber[i][j] = 1<<goldSoFar;
					goldSoFar++;
				}
	}

	queue<state> q;
	state currState;
	currState.pos = currLoc;
	currState.first = NONE;
	currState.depth = 0;
	currState.goldNumber = 1;
	currState.numGold = 0;
	//The best state to get X pieces of gold (X is index)
	vector<state> best;
	best.push_back(currState);

	if(!isAlive())
		return best;
	for(int i =0; i<16; i++)
		for(int j = 0; j <25; j++)
				visited[i][j] = 0;

	q.push(currState);

	while(!q.empty()){
		state newState = q.front();q.pop();
		loc cur = newState.pos;

		if(map[cur.first][cur.second]==GOLD && (!(newState.goldNumber&goldNumber[cur.first][cur.second])))
		{
			TRACE("NUMGOLD %d\n",newState.numGold);
			TRACE("GOLDNUM %lld\n",newState.goldNumber);
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
				if(canDoActionPlayer(a,cur)){
					loc alteredLoc = simulateAction(a,cur);

					if((visited[alteredLoc.first][alteredLoc.second] &  newState.goldNumber) != newState.goldNumber){
						visited[alteredLoc.first][alteredLoc.second] |= newState.goldNumber;
						state alteredState;
						alteredState.pos = alteredLoc;
						alteredState.depth = newState.depth+1;
						alteredState.goldNumber = newState.goldNumber;
						alteredState.numGold = newState.numGold;
						if(alteredState.depth==1)
							alteredState.first = static_cast<Action>(i);
						else
							alteredState.first = newState.first;
						q.push(alteredState);
					}
				}
			}
	}

	return best;
}
