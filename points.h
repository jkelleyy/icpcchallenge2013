#ifndef BISTROMATHICS_POINTS_H
#define BISTROMATHICS_POINTS_H 1

#include "game_state.h"
#include <vector>

struct dugCell
{
	loc_t loc;
	int timeDug;
};

struct state
{
	Action first;
	loc_t pos;
	int depth;
	int numGold;
	long long goldNumber;
	int digDelay;
	vector<dugCell> dugCells;
	bool us;
};

//finds the action which gets you closest to a gold
vector<state> pointsScore(int desiredGold);

#endif
