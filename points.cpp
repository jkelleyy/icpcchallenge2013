#include "game_state.h"
#include <queue>

using namespace std;

typedef pair<int,int> loc;

bool visited[16][26];

state pointsScore(){
	queue<state> q;
	state currState;
	currState.pos = currLoc;
	currState.first = NONE;
	currState.depth = 0;

	for(int i =0; i<16; i++)
		for(int j = 0; j <25; j++)
			visited[i][j] = false;


	q.push(currState);
	while(!q.empty()){
		state newState = q.front();q.pop();
		loc cur = newState.pos;

		if(map[cur.first][cur.second]==GOLD)
			return newState;
		else{
			for(int i=NONE; i<7;i++){
				Action a = static_cast<Action>(i);
				if(canDoAction(a,cur)){
					loc alteredLoc = simulateAction(a,cur);
					if(!visited[alteredLoc.first][alteredLoc.second]){
						visited[alteredLoc.first][alteredLoc.second] = true;
						state alteredState;
						alteredState.pos = alteredLoc;
						alteredState.depth = newState.depth+1;
						if(alteredState.depth==1)
							alteredState.first = i;
						else
							alteredState.first = newState.first;
						q.push(alteredState);
					}
				}
			}
		}
	}

	return currState;
}
