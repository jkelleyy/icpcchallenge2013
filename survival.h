#ifndef BISTROMATHICS_SURVIVAL_H
#define BISTROMATHICS_SURVIVAL_H 1

#include <utility>
#include "game_state.h"

using namespace std;

void scoreSurvival(int *scores);

pair<int,ChaseInfo> computeChaseState(int enemyId);

#endif
