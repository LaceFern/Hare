//
// Created by Alicia on 2023/9/6.
//

#ifndef KVS_SERVER_STATS_SERVER_H
#define KVS_SERVER_STATS_SERVER_H

#include "dpdk_ctrl.h"

#define STATS_LOCK_STAT 0
#define STATS_GET_HOT 1
#define STATS_HOT_REPORT 2
#define STATS_WAIT 3
#define STATS_READY 4
#define STATS_REPLACE_SUCCESS 5

struct topk_obj_info{
    uint_obj obj;
    uint32_t count;
};

extern uint32_t stats_stage;

extern volatile bool clean_flag;
//extern volatile uint32_t stats_phase[NC_MAX_LCORES];
extern topk_obj_info merge_to_info_list[NC_MAX_LCORES * TOPK];
extern topk_obj_info lcore_to_info_list[NC_MAX_LCORES][TOPK + 1];

extern uint32_t lock_stat_flag;
extern volatile uint32_t upd_obj_flag;
extern uint32_t upd_obj_num;

extern cold_obj_info co_info;
extern cold_obj_info co_info_heap[TOPK];

extern hot_obj_info ho_info;
extern hot_obj_info ho_info_heap[TOPK];
extern uint32_t ho_num;

extern upd_obj_info uo_info;
extern upd_obj_info uo_info_list[TOPK];
extern upd_obj_info uo_info_pernode_list[NUM_BACKENDNODE][TOPK];
extern uint32_t uo_info_num_pernode_list[NUM_BACKENDNODE];
extern uint32_t uo_num;

extern std::unordered_map<uint_obj, uint32_t> stats_0[NC_MAX_LCORES];
extern std::unordered_map<uint_obj, uint32_t> stats_1[NC_MAX_LCORES];

uint32_t server_stats_loop();
void stats_update(uint_obj obj, uint32_t stats_id);
void minheap_downAdjust(topk_obj_info heap[], int low, int high);

void move_appdata_fromSDPtoBS(upd_obj_info uo_info_list[]);
void move_appdata_fromBStoSDP(upd_obj_info uo_info_list[]);


struct throughput_statistics {
    uint64_t tx;
    uint64_t rx;
    uint64_t last_tx;
    uint64_t last_rx;
} __rte_cache_aligned;
extern throughput_statistics tput_stat[NC_MAX_LCORES];
void print_per_core_throughput(void);
#endif //KVS_SERVER_STATS_SERVER_H
