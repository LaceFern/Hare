
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "../kfifo/kfifo.h"
#include "cache_upd_ctrl.h"

#define MAX_APP_THREAD 48
#define APP_TO_STATS 4
DECLARE_KFIFO_ARR(statsFifoArr, int, 2, MAX_APP_THREAD);

const appThreadNum = 23;

void initStatsFifoArr(){
    INIT_KFIFO_ARR(statsFifoArr, MAX_APP_THREAD);
}

int enqueue(int appThreadIndex, int objIndex) {
    if (kfifo_is_full(&(statsFifoArr[appThreadIndex]))) {
        // printf("kfifo[%d]: full!\n", appThreadIndex);
        return -1;
    }
    else{
        if (kfifo_in(&(statsFifoArr[appThreadIndex]), &objIndex, 1) == 1) {
            // printf("kfifo[%d]: objIndex(%d) in!\n", appThreadIndex, objIndex);
            return 0;
        } else {
            printf("kfifo[%d]: not full but enqueue fails! Something is Wrong!", appThreadIndex);
            return -2;
        }
    }
}

int dequeue(int appThreadIndex) {
    int objIndex = 0;
    if (kfifo_is_empty(&(statsFifoArr[appThreadIndex]))) {
        // printf("kfifo[%d]: empty!\n", appThreadIndex);
        return -1;
    }
    else{
        if (kfifo_out(&(statsFifoArr[appThreadIndex]), &objIndex, 1) == 1){
            // printf("kfifo[%d]: objIndex(%d) out!\n", appThreadIndex, objIndex);
            return objIndex;
        }
        else{
            printf("kfifo[%d]: dequeue fails! Something is Wrong!", appThreadIndex);
            return -2;
        }    
    } 
}

int consume(int appThreadNUM, int statsThreadIndex) {
    int rangeLow = APP_TO_STATS * statsThreadIndex;
    int rangeHigh = APP_TO_STATS * (statsThreadIndex + 1);
    while(1){
        for(int i = rangeLow; i < rangeHigh && i < appThreadNum; i++){
            int objIndex = dequeue(i);
            if(objIndex > -1){
                service_statistic_update(objIndex, i);
            }
        }
    }

    // // only for 23 app threads and 10 stats threads
    // int rangeLow;
    // int rangeHigh;
    // if(statsThreadIndex < 7){
    //     rangeLow = 2 * statsThreadIndex;
    //     rangeHigh = 2 * (statsThreadIndex + 1);
    // }
    // else{
    //     rangeLow = 14 + 3 * (statsThreadIndex - 7);
    //     rangeHigh = 14 + 3 * (statsThreadIndex - 7 + 1);
    // }
    // while(1){
    //     for(int i = rangeLow; i < rangeHigh && i < appThreadNum; i++){
    //         int objIndex = dequeue(i);
    //         if(objIndex > -1){
    //             service_statistic_update(objIndex, i);
    //         }
    //     }
    // }
}