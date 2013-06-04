#ifndef BISTROMATHICS_POINTS_H
#define BISTROMATHICS_POINTS_H 1

#include "game_state.h"

typedef pair<int,int> loc;

struct state
{
	Action first;
	int firstAction;
	loc pos;
	int depth;
};

//finds the action which gets you closest to a gold
state pointsScore();

#endif
