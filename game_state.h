#ifndef BISTROMATHICS_GAME_STATE_H
#define BISTROMATHICS_GAME_STATE_H 1

#include <utility>
#include <list>
#include <vector>
#include <cstdlib>

#include "simple_stack.h"

using namespace std;

typedef pair<signed char,signed char> loc_t;

#define EMPTY '.'
#define LADDER 'H'
#define BRICK '='
#define GOLD '*'
#define REMOVED_BRICK '-'
#define FILLED_BRICK '+'

//bunch of tiny utility functions

static inline bool isImpassable(char c){
    return c==BRICK || c==FILLED_BRICK || c=='\0';
}

static inline bool isSolid(char c){
    return c==BRICK || c==LADDER || c==FILLED_BRICK || c=='\0';
}

//don't add to these, it might break other code
enum ChaseState{
    //UNKNOWN is if we lose track and can't figure out what the enemy is doing
    //if it's dead, use PATROL
    CHASE_RED,CHASE_BLUE,RETURN_TO_PATROL,PATROL,UNKNOWN
};

static const char* chaseStateNames[] = {
    "CHASE_RED",
    "CHASE_BLUE",
    "RETURN_TO_PATROL",
    "PATROL",
    "UNKNOWN"
};

//find a better place for these...
enum Player{
    NOONE = -1,RED=0,BLUE=1
};

enum Action{
    NONE=0,LEFT,RIGHT,DIG_LEFT,DIG_RIGHT,TOP,BOTTOM
};

struct ChaseInfo{
    Player target;
    Action startDir;
    int pathLength;
};

struct SharedEnemyInfo{
    loc_t spawn;
    vector<Action> program;
};

//note 5 bits -> max value 31 unsigned

//bits 0-5 row //6 bits
//bits 6-11 col //6 bits
//bits 12-16 spawn_delay
//bit 17 trapped?
//bit 18-19 master
//bits 20-22 chaseState
//bits 23-26 lastMove

#define ROW_MASK 0x3F
#define ROW_SHIFT 0
#define COL_MASK 0xFC0
#define COL_SHIFT 6
#define SPAWN_DELAY_MASK 0x1F000
#define SPAWN_DELAY_SHIFT 12
#define TRAPPED_MASK 0x20000
#define TRAPPED_SHIFT 17
#define MASTER_MASK 0xC0000
#define MASTER_SHIFT 18
#define CHASE_STATE_MASK 0x700000
#define CHASE_STATE_SHIFT 20
#define LAST_MOVE_MASK 0x7800000
#define LAST_MOVE_SHIFT 23

class World;


static inline bool checkBounds(int r, int c){
    return r>=0 && r<16 && c>=0 && c<25;
}
static inline bool checkBounds(const loc_t& loc){
    return checkBounds(loc.first,loc.second);
}


struct EnemyInfo{
    const SharedEnemyInfo* info;
    const World *env;
    unsigned int flags;
    int patrolIndex;
    SimpleStack<Action> chaseStack;


#define GET_VAL(name) ((name ## _MASK & flags) >> name ## _SHIFT)
#define SET_VAL(name,val) (flags = (flags & ~ name ## _MASK)|((((unsigned int)val)<<name ## _SHIFT) & name##_MASK))
    loc_t getLoc() const{
        int r = GET_VAL(ROW);
        r = (r<<(32-6))>>(32-6);
        int c = GET_VAL(COL);
        c = (c<<(32-6))>>(32-6);
        //TODO sign extend
        return make_pair(r,c);
    }
    void setLoc(const loc_t& val){
        SET_VAL(ROW,val.first);
        SET_VAL(COL,val.second);
    }
    int getSpawnDelay() const{
        return GET_VAL(SPAWN_DELAY);
    }
    void setSpawnDelay(int val){
        SET_VAL(SPAWN_DELAY,val);
    }
    Player getMaster() const{
        int val = GET_VAL(MASTER);
        switch(val){
        case 0:
            return RED;
        case 1:
            return BLUE;
        default:
            return NOONE;
        }
    }
    void setMaster(Player p){
        SET_VAL(MASTER,p);
    }
    bool isTrapped() const{
        return GET_VAL(TRAPPED);
    }
    void setIsTrapped(bool val){
        SET_VAL(TRAPPED,val);
    }
    Action getLastMove() const{
        return static_cast<Action>(GET_VAL(LAST_MOVE));
    }
    void setLastMove(Action a){
        SET_VAL(LAST_MOVE,a);
    }
    ChaseState getChaseState() const{
        return static_cast<ChaseState>(GET_VAL(CHASE_STATE));
    };
    void setChaseState(ChaseState s){
        SET_VAL(CHASE_STATE,s);
    }
    inline bool isAlive() const{
        return checkBounds(getLoc());
    }
#undef GET_VAL
#undef SET_VAL
    bool isFalling();
    bool didMove();
};

//things that don't change
class StaticWorldData{
public:
    int nrounds;
    int nenemies;
    loc_t enemySpawn;
    loc_t ourSpawn;
    char baseMap[16][26];
};

extern StaticWorldData fixedData;


//copy on write map
template<int nrows,int ncols>
class COWMap{
private:
    template<typename T,int arrSize>
    struct RefCountedArray{
        int refCount;
        T arr[arrSize];
        RefCountedArray * copy(){
            RefCountedArray *ret = new RefCountedArray;
            ret->refCount=1;
            for(int i=0;i<arrSize;i++)
                ret->arr[i] = this->arr[i];
            return ret;
        }
        //note, this function does RAW access, it doesn't understand COW
        T& lookup(int index) {return arr[index];}
        const T& lookup(int index) const {return arr[index];}
    };
    RefCountedArray<RefCountedArray<char,ncols>*,nrows> * data;
public:
    COWMap() : data(new RefCountedArray<RefCountedArray<char,ncols>*,nrows>()) {
        data->refCount++;
        for(int i=0;i<nrows;i++){
            data->lookup(i) = new RefCountedArray<char,ncols>();
            data->lookup(i)->refCount++;
        }
    }
    COWMap(const COWMap& other) : data(other.data){
        data->refCount++;
    }
    ~COWMap(){
        data->refCount--;
        if(data->refCount==0){
            for(int i=0;i<nrows;i++){
                data->lookup(i)->refCount--;
                if(data->lookup(i)->refCount==0)
                    delete data->lookup(i);
            }
            delete data;
        }
    }
    COWMap& operator=(const COWMap& other){
        this->data->refCount--;
        if(this->data->refCount==0){
            for(int i=0;i<nrows;i++){
                this->data->lookup(i)->refCount--;
                if(this->data->lookup(i)->refCount==0)
                    delete this->data->lookup(i);
            }
            delete data;
        }
        this->data = other.data;
        this->data->refCount++;
        return *this;
    }
    class FakeRef{
    private:
        COWMap<nrows,ncols> *map;
        int r,c;
    public:
        FakeRef(COWMap<nrows,ncols> *base,int _r, int _c) : map(base),r(_r),c(_c) {};
        operator char() const{return map->data->lookup(r)->lookup(c);}
        FakeRef operator=(const char& val) {
            if(val==*this)
                return *this;
            if(map->data->refCount!=1){
                map->data->refCount--;
                map->data = map->data->copy();
                for(int i=0;i<nrows;i++){
                    map->data->lookup(i)->refCount++;
                }
            }
            if(map->data->lookup(r)->refCount!=1){
                map->data->lookup(r)->refCount--;
                map->data->lookup(r) = map->data->lookup(r)->copy();
            }
            //now it's unique
            map->data->lookup(r)->lookup(c) = val;
            return *this;
        }
    };
    char lookup(int r, int c) const {return data->lookup(r)->lookup(c);}
    char lookup(const loc_t& loc) const {return lookup(loc.first,loc.second);}
    //act like the FakeRef is a char&
    FakeRef lookup(int r, int c) {return FakeRef(this,r,c);}
    FakeRef lookup(const loc_t& loc) {return lookup(loc.first,loc.second);}
};

//moved here since no one actually cares about these values
extern int enemyScore;
extern int currScore;
extern Action ourLastMove;


static inline char checkBaseMapSafe(int r,int c){
    if(checkBounds(r,c))
        return fixedData.baseMap[r][c];
    return '\0';
}

static inline char checkBaseMapSafe(loc_t loc){
    return checkBaseMapSafe(loc.first,loc.second);
}


static inline bool isSupportedEnemy(const loc_t& loc){
    return isSolid(checkBaseMapSafe(loc.first+1,loc.second)) || checkBaseMapSafe(loc) == LADDER;
}

//these things change every turn
class World{
public:
    vector<EnemyInfo> enemies;
    COWMap<16,25> map;
    loc_t currLoc;
    loc_t enemyLoc;
    char brickDelay;
    char enemySpawnDelay;
    char enemyBrickDelay;

    inline char checkMapRaw(const loc_t& loc) const{
        return checkMapRaw(loc.first,loc.second);
    }
    inline char checkMapRaw(int r,int c) const{
        return map.lookup(r,c);
    }
    inline char checkMapSafe(int r,int c) const{
        if(r>=0 && r<16 && c>=0 && c<25)
            return checkMapRaw(r,c);
        return '\0';
    }
    inline char checkMapSafe(const loc_t& loc) const{
        return checkMapSafe(loc.first,loc.second);
    }

    bool canDoActionRaw(Action act, const loc_t& loc) const;
    inline bool canDoActionPlayer(Action act) const { return canDoActionPlayer(act,currLoc); }
    inline bool canDoActionPlayer(Action act, const loc_t& loc) const{ return canDoActionPlayer(act,loc,brickDelay); }
    bool canDoActionPlayer(Action act, const loc_t& loc, int curBrickDelay) const;
    bool canDoActionOpponent(Action act, const loc_t& loc) const;
    inline bool canDoActionOpponent(Action act) const{ return canDoActionOpponent(act,enemyLoc); }
    inline bool isAlive() const{
        return currLoc.first!=-1;
    };
    bool isSupported() const { return isSupported(currLoc); }
    inline bool isSupported(const loc_t& loc) const {
        return isSolid(checkMapSafe(loc.first+1,loc.second)) || checkMapRaw(loc)==LADDER;
    }


};

static inline bool canDoActionEnemy(Action act,const loc_t& loc){
    switch(act){
    case NONE:
        return true;
    case LEFT:
        return isSupportedEnemy(loc)
            && !isImpassable(checkBaseMapSafe(loc.first,loc.second-1));
    case RIGHT:
        return isSupportedEnemy(loc)
            && !isImpassable(checkBaseMapSafe(loc.first,loc.second+1));
    case DIG_LEFT:
    case DIG_RIGHT:
        return false;
    case TOP:
        return loc.first>0
            && isSupportedEnemy(loc)//can't move while falling
            && checkBaseMapSafe(loc)==LADDER
            && !isImpassable(checkBaseMapSafe(loc.first-1,loc.second));
    case BOTTOM:
        return isSupportedEnemy(loc)//can't move while falling
            && !isImpassable(checkBaseMapSafe(loc.first+1,loc.second));
    }
    return false;//should be unreachable
}



//doing this was much easier than trying to impelement a smarter points system

class PointsWorld : public World{
public:
    //moved here since the prediction related stuff really doesn't care
    int missedTurns;
    int currTurn;
    char timeout[16][26];//Indicates how long for bricks and gold to respaun.  0= there now.
};

extern PointsWorld game;

static char (&originalMap)[16][26] = fixedData.baseMap;
extern int component[16][26];
extern int gold_comp[600];
extern int max_gold_comp;
extern int totalGoldOnMap;
extern bool reachable[16][26][16][26];
extern int depth[16][26];
extern loc_t earliest_parent[16][26];

static inline int distSq(const loc_t& a, const loc_t& b){
    return (b.first-a.first)*(b.first-a.first)+(b.second-a.second)*(b.second-a.second);
}

inline bool EnemyInfo::isFalling(){
    return isAlive() && !game.isSupported(getLoc()) && !isTrapped();
}

static inline Action reverseAction(Action a){
    switch(a){
    case NONE:
        return NONE;
    case LEFT:
        return RIGHT;
    case RIGHT:
        return LEFT;
    case DIG_LEFT:
        return DIG_RIGHT;
    case DIG_RIGHT:
        return DIG_LEFT;
    case TOP:
        return BOTTOM;
    case BOTTOM:
        return TOP;
    }
}


static const char *actionNames[7] = {
    "NONE",
    "LEFT",
    "RIGHT",
    "DIG_LEFT",
    "DIG_RIGHT",
    "TOP",
    "BOTTOM"
};

bool canDoAction2(Action act,const loc_t& loc = game.currLoc);
static inline loc_t simulateAction(const World& world, Action act,const loc_t& loc){
	if(loc.first==-1)
		return make_pair(-1,-1);
	//Falling
	if(!world.isSupported(loc))
		return make_pair(loc.first+1,loc.second);
	switch(act){
	case NONE:
	case DIG_LEFT:
	case DIG_RIGHT:
		return loc;
	case LEFT:
		return make_pair(loc.first,loc.second-1);
	case RIGHT:
		return make_pair(loc.first,loc.second+1);
	case TOP:
		return make_pair(loc.first-1,loc.second);
	case BOTTOM:
		return make_pair(loc.first+1,loc.second);
	}

	return loc;
}

static inline loc_t simulateAction(Action act,const loc_t& loc){return simulateAction(game,act,loc);}

#endif
