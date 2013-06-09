#include <time.h>
#include <pthread.h>
#include "util.h"

void *watchdogRoutine(void *){
    struct timespec sleepSpec;
    sleepSpec.tv_sec = 0;
    sleepSpec.tv_nsec = 50*1000;
    struct timespec start,stop;
    clock_getres(CLOCK_MONOTONIC,&start);
    INFO("Clock resolution: %ld\n",start.tv_nsec);


    while(true){
        clock_gettime(CLOCK_MONOTONIC,&start);
        nanosleep(&sleepSpec,NULL);
        clock_gettime(CLOCK_MONOTONIC,&stop);
        long diff;
        if(start.tv_sec == stop.tv_sec){
            diff = stop.tv_nsec - start.tv_nsec;
        }
        else{
            diff = (1000*1000*(stop.tv_sec-start.tv_sec) + stop.tv_nsec) - start.tv_nsec;
        }
        //INFO("NANO: %ld\n",diff);
    }
    return NULL;
}

void startWatchdog(){
    pthread_t id;
    INFO("HI! from watchdog\n");

    pthread_create(&id,NULL,watchdogRoutine,NULL);
}
