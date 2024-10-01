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

struct throughput_statistics {
    uint64_t tx;
    uint64_t rx;
    uint64_t last_tx;
    uint64_t last_rx;
} __rte_cache_aligned;


class User {
private:
    static uint32_t stats_stage;

    static volatile bool clean_flag;
    //volatile uint32_t stats_phase[NC_MAX_LCORES];
    static topk_obj_info merge_to_info_list[NC_MAX_LCORES * TOPK];
    static topk_obj_info lcore_to_info_list[NC_MAX_LCORES][TOPK + 1];

    static uint32_t lock_stat_flag;
    static volatile uint32_t upd_obj_flag;
    static uint32_t upd_obj_num;

    static cold_obj_info co_info;
    static cold_obj_info co_info_heap[TOPK];

    static hot_obj_info ho_info;
    static hot_obj_info ho_info_heap[TOPK];
    static uint32_t ho_num;

    static upd_obj_info uo_info;
    static upd_obj_info uo_info_list[TOPK];
    static upd_obj_info uo_info_pernode_list[NUM_BACKENDNODE][TOPK];
    static uint32_t uo_info_num_pernode_list[NUM_BACKENDNODE];
    static uint32_t uo_num;

    static std::unordered_map<uint_obj, uint32_t> stats_0[NC_MAX_LCORES];
    static std::unordered_map<uint_obj, uint32_t> stats_1[NC_MAX_LCORES];

    static std::mutex mtx;

public:
    static uint32_t server_stats_loop();
    static void stats_update(uint_obj obj, uint32_t stats_id);
    static void minheap_downAdjust(topk_obj_info heap[], int low, int high);

    static void move_appdata_fromSDPtoBS(upd_obj_info uo_info_list[]);
    static void move_appdata_fromBStoSDP(upd_obj_info uo_info_list[]);
    // static void move_appdata_fromSDPtoBS(upd_obj_info uo_info);
    // static void move_appdata_fromBStoSDP(upd_obj_info uo_info);
};

#endif //KVS_SERVER_STATS_SERVER_H