#ifndef BISTROMATHICS_MAP_COMPONENT_H
#define BISTROMATHICS_MAP_COMPONENT_H 1

#include <utility>
#include <list>
#include <vector>
#include "points.h"

using namespace std;

void printReachableMatrix(int starti, int startj);
void printComponentMatrix();
void dfs(pair<int,int> start, pair<int,int> curr);
void computeAllWayReachability();
void assignComponents();
void findGoldInComponents();
void findTotalGoldInMap();
bool shouldSuicide(pair<int,int> loc);
Action getSuicidalMove(pair<int,int> loc);

#endif
