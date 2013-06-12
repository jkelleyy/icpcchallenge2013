#ifndef BISTROMATHICS_POINTS_H
#define BISTROMATHICS_POINTS_H 1

#include "game_state.h"
#include <vector>

struct state
{
	Action first;
	pair<int,int> pos;
	int depth;
	int numGold;
	long long goldNumber;
};

//finds the action which gets you closest to a gold
vector<state> pointsScore(int desiredGold, int turnNo);

#endif
