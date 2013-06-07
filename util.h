#ifndef BISTROMATHICS_UTIL_H
#define BISTROMATHICS_UTIL_H 1

#include <cstdio>
using namespace std;

#ifdef DEBUG_OUTPT_ON
//various utilities
//TODO add flags to turn these on and off
#define TRACE(...) fprintf(stderr,__VA_ARGS__)
#define ERROR(...) fprintf(stderr,__VA_ARGS__)
#define WARN(...) fprintf(stderr,__VA_ARGS__)
#define DEBUG(...) fprintf(stderr,__VA_ARGS__)
#define INFO(...) fprintf(stderr,__VA_ARGS__)

#else
#define TRACE(...) do{}while(0)
#define ERROR(...) do{}while(0)
#define WARN(...) do{}while(0)
#define DEBUG(...) do{}while(0)
#define INFO(...) do{}while(0)

#endif


//stuff that should probably go elsewhere
#define NEG_INF -100000000

#endif
