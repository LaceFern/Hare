//
// Created by Alicia on 2023/8/28.
//

#ifndef CONTROLLER_V1_CTRL_H
#define CONTROLLER_V1_CTRL_H

#include <iostream>

/******************** need config*********************/
#define MAX_CACHE_SIZE 32768
#define WAIT_MS 50
#define END_S 400
#define TOPK 100
#define NUM_OBJ_BYTE 16
#define RESTART_TIMES 3

#define SWITCH_MAC "00:90:fb:70:63:c5" // switch data plane
#define SWITCH_IP "192.168.189.55"
#define CTRL_MAC "00:90:fb:70:63:c6" // switch control plane enp4s0f0
#define CTRL_IP "192.168.189.66"

#define AMAX1_MAC "b8:59:9f:e9:6b:1c"
#define AMAX1_IP "10.0.0.1"

//#define SWITCH_MAC "00:90:fb:70:63:c6" // switch control plane enp4s0f0
//#define SWITCH_IP "192.168.189.66"
//#define CTRL_MAC "98:03:9b:ca:48:38" // amax2
//#define CTRL_IP "192.168.189.8"

#define NUM_BACKENDNODE 1
extern uint8_t *backend_node_ip_list[NUM_BACKENDNODE];

extern uint8_t *backend_node_mac_list[NUM_BACKENDNODE];



/******************** inner *********************/

#define STATS_COLLECT 0
#define STATS_ANALYZE 1
#define STATS_UPDATE 2
#define STATS_CLEAN 3

#define OP_LOCK_STAT_SWITCH 0x0C
#define OP_LOCK_STAT_FPGA 12
#define OP_GET_COLD_COUNTER_SWITCH 0x08
#define OP_GET_HOT_COUNTER_FPGA 18
#define OP_GET_END_FPGA 20
#define OP_GET_END_2_UPDATE_FPGA 21
#define OP_GET_END_2_CLEAN_FPGA 22

#define OP_QUEUE_EMPTY_SWITCH 0x11
#define OP_QUEUE_EMPTY_FPGA 0x11
#define OP_HOT_REPORT_SWITCH 0x0E
//#define OP_LOCK_COLD_QUEUE_SWITCH 0x0A
//#define OP_LOCK_HOT_QUEUE_FPGA 19
#define OP_MOD_STATE_UPD_SWITCH 0x0A
#define OP_MOD_STATE_UPD_FPGA 19
#define OP_MOD_STATE_NOUPD_SWITCH 0x0A
#define OP_MOD_STATE_NOUPD_FPGA 19
#define OP_CLN_COLD_COUNTER_SWITCH 0x09
#define OP_CLN_HOT_COUNTER_FPGA 9
#define OP_UNLOCK_STAT_SWITCH 0x0D
#define OP_UNLOCK_STAT_FPGA 13
#define OP_UNLOCK_HOT_QUEUE_FPGA 15

#define HOTOBJ_PER_PKT 20
#define COLDCOUNT_PER_PKT 1
#define INVALID_4B 0xffffffff

struct ctrl_para{
    uint32_t wait_millisecond = WAIT_MS;
    uint32_t cache_size = MAX_CACHE_SIZE;
};




typedef uint32_t uint_obj;

class Obj{
    uint8_t byte_list[NUM_OBJ_BYTE];
};

struct only_op_payload{
    uint8_t op_type;
}__attribute__((__packed__));

struct fetch_co_payload{
    uint8_t op_type;
    uint32_t obj_index;
    uint32_t obj_count[COLDCOUNT_PER_PKT];
}__attribute__((__packed__));

struct hot_obj_payload{
    uint8_t op_type;
    uint32_t obj_num;
    uint32_t backend_node_index;
    uint_obj obj[HOTOBJ_PER_PKT];
    uint32_t    counter[HOTOBJ_PER_PKT];
    uint32_t pkt_index;
    uint32_t pkt_num;
}__attribute__((__packed__));

struct change_hot_obj_state_payload{
    uint8_t op_type;
    uint32_t obj_num;
    uint_obj hot_obj[HOTOBJ_PER_PKT];
    uint_obj cold_obj[HOTOBJ_PER_PKT];
    uint_obj cold_obj_index[HOTOBJ_PER_PKT];
    uint32_t pkt_index;
    uint32_t pkt_num;
}__attribute__((__packed__));

struct change_cold_obj_state_payload{
    uint8_t op_type;
    uint32_t obj_index;
}__attribute__((__packed__));



struct cold_obj_info{
    uint32_t obj_index = 0;
    uint32_t cold_count = 0;
    uint32_t empty_flag = 0;
};

struct hot_obj_info{
    uint_obj obj;
    uint32_t hot_count;
    uint32_t backend_node_index;
};

struct upd_obj_info{
    uint_obj hot_obj;
    uint_obj cold_obj;
    uint32_t cold_obj_index;
};

// decrease
void maxheap_downAdjust(cold_obj_info heap[], int low, int high);

uint32_t Partition(hot_obj_info *L,uint32_t low,uint32_t high);
void QSort(hot_obj_info *L,uint32_t low,uint32_t high);
void QuickSort(hot_obj_info *L, uint32_t L_len);

extern int init_begin;
extern int init_end;//0;//10000;

extern uint32_t debug_flag;

extern volatile uint32_t ctrl_stage;//STATS_ANALYZE;//STATS_COLLECT;//STATS_COLLECT;//STATS_COLLECT;//STATS_CLEAN;

#endif //CONTROLLER_V1_CTRL_H
