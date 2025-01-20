//
// Created by Alicia on 2023/8/30.
//

#include <cstdint>
#include "general_ctrl.h"

int init_begin = 0;
int init_end = 0;//0;//10000;

uint32_t debug_flag = 1;

volatile uint32_t ctrl_stage = STATS_COLLECT;//STATS_ANALYZE;//STATS_COLLECT;//STATS_COLLECT;//STATS_COLLECT;//STATS_CLEAN;

uint8_t *backend_node_ip_list[NUM_BACKENDNODE] = {
    (uint8_t *)"10.0.0.7",
    (uint8_t *)"10.0.0.8"
};

uint8_t *backend_node_mac_list[NUM_BACKENDNODE] = {
    (uint8_t *)"98:03:9b:ca:40:18",
    (uint8_t *)"0c:42:a1:2b:0d:70"
};

void maxheap_downAdjust(cold_obj_info heap[], int low, int high){
    int i = low, j = i * 2;
    while(j <= high){
        if(j+1 <= high && heap[j+1].cold_count > heap[j].cold_count){
            j = j + 1;
        }
        if(heap[j].cold_count > heap[i].cold_count){
            cold_obj_info temp = heap[i];
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

uint32_t Partition(hot_obj_info *L,uint32_t low,uint32_t high){
    L[0]=L[low];
    uint32_t pivotcount=L[low].hot_count;
    while (low<high) {
        while (low<high && L[high].hot_count>=pivotcount) {
            high--;
        }
        L[low]=L[high];
        while (low<high && L[low].hot_count<=pivotcount) {
            low++;
        }
        L[high]=L[low];
    }
    L[low]=L[0];
    return low;
}
void QSort(hot_obj_info *L,uint32_t low,uint32_t high){
    if (low<high) {
        int pivotloc=Partition(L, low, high);
        QSort(L, low, pivotloc-1);
        QSort(L, pivotloc+1, high);
    }
}

// decrease
void QuickSort(hot_obj_info *L, uint32_t L_len){
    QSort(L, 1,L_len);
}