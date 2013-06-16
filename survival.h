#ifndef BISTROMATHICS_SURVIVAL_H
#define BISTROMATHICS_SURVIVAL_H 1

#include <utility>
#include "game_state.h"

using namespace std;

void predict(double *scores);

ChaseInfo computeChase(const World&,int enemyId);

void initSurvival();
#endif
