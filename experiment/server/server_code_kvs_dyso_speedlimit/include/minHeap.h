#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct  heapNode_ {
    uint32_t    lockId;
    uint32_t    lockCounter;
} heapNode;

//对heap数组在[low, high]范围内向下调整
void downAdjust(heapNode heap[], int low, int high){
    int i = low, j = i * 2;
    while(j <= high){
        if(j+1 <= high && heap[j+1].lockCounter < heap[j].lockCounter){
            j = j + 1;
        }
        if(heap[j].lockCounter < heap[i].lockCounter){
            heapNode temp = heap[i];
            heap[i] = heap[j];
            heap[j] = temp;
            i = j;
            j = i * 2;
        }
        else{
            break;
        }
    }
}
