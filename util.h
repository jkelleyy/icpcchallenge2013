#ifndef BISTROMATHICS_UTIL_H
#define BISTROMATHICS_UTIL_H 1

#include <cstdio>
using namespace std;

//various utilities
//TODO add flags to turn these on and off
#define TRACE(...) fprintf(stderr,__VA_ARGS__)
#define ERROR(...) fprintf(stderr,__VA_ARGS__)
#define WARN(...) fprintf(stderr,__VA_ARGS__)
#define DEBUG(...) fprintf(stderr,__VA_ARGS__)
#define INFO(...) fprintf(stderr,__VA_ARGS__)

//stuff that should probably go elsewhere
#define NEG_INF -100000000
#define POS_INF 100000000

#endif
