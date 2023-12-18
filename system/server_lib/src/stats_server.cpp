//
// Created by Alicia on 2023/9/6.
//
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <mutex>

#include "dpdk_ctrl.h"
#include "general_ctrl.h"
#include "general_server.h"
#include "stats_server.h"

uint32_t stats_stage = STATS_COLLECT; // STATS_COLLECT // STATS_ANALYZE

volatile bool clean_flag = 0;
//volatile uint32_t stats_phase[NC_MAX_LCORES];
topk_obj_info merge_to_info_list[NC_MAX_LCORES * TOPK];
topk_obj_info lcore_to_info_list[NC_MAX_LCORES][TOPK + 1];

uint32_t lock_stat_flag = 0;
volatile uint32_t upd_obj_flag = 0;
uint32_t upd_obj_num = 0;

cold_obj_info co_info = {};
cold_obj_info co_info_heap[TOPK] = {0};

hot_obj_info ho_info = {};
hot_obj_info ho_info_heap[TOPK] = {0};
uint32_t ho_num = 0;

upd_obj_info uo_info = {};
upd_obj_info uo_info_list[TOPK] = {};
upd_obj_info uo_info_pernode_list[NUM_BACKENDNODE][TOPK];
uint32_t uo_info_num_pernode_list[NUM_BACKENDNODE];
uint32_t uo_num = 0;

std::unordered_map<uint_obj, uint32_t> stats_0[NC_MAX_LCORES];
std::unordered_map<uint_obj, uint32_t> stats_1[NC_MAX_LCORES];

std::mutex mtx;

uint32_t server_stats_loop(){
    uint32_t lcore_id = rte_lcore_id();
    uint32_t rx_queue_id = lcore_id_2_rx_queue_id_list[lcore_id];
    /***************************** variables *****************************/
    rte_mbuf *mbuf;

    uint8_t rcv_payload_list[65536];
    uint32_t rcv_payload_len_list[1024];
    uint8_t rcv_payload[1024];
    uint32_t rcv_payload_len;
    uint32_t rcv_payload_bias;
    uint8_t send_payload[1024];
    uint32_t send_payload_len;

    ctrl_para para;

    uint32_t loop_count = 0;
    uint32_t rcv_count = 0;

    lcore_configuration *lconf = &lcore_conf_list[lcore_id];

    Timer t_program;
    Timer t_wait;
    Timer t_temp;

    t_program.start();

    PacketGenerator pkt_gen(
            lcore_id, nullptr,
            (uint8_t *) CTRL_MAC, (uint8_t *) SERVER_MAC,
            (uint8_t *) CTRL_IP, (uint8_t *) SERVER_IP,
            UDP_CTRL_PORT, UDP_CTRL_PORT);


    // initiate
    uint32_t next_stage_flag = 0;
    uint32_t num_stats_lcore = n_lcore - 1;
    for (int m = 0; m < num_stats_lcore; m++) {
        for (int n = 0; n < TOPK; n++) {
            lcore_to_info_list[m][n + 1].count = 0;
            lcore_to_info_list[m][n + 1].obj = INVALID_4B;
        }
    }

    while(1){
        switch (stats_stage) {
            case STATS_COLLECT:{
                next_stage_flag = 0;

                if(clean_flag == 1){
                    for(int i = 0; i < NC_MAX_LCORES; i++){
                        stats_1[i].clear();
                    }
                }
                else{
                    for(int i = 0; i < NC_MAX_LCORES; i++){
                        stats_0[i].clear();
                    }
                }

                auto rcv_op_payload = (only_op_payload *)rcv_payload;
                auto send_op_payload = (only_op_payload *) send_payload;

                do{
                    rcv_payload_bias = 0;
                    rcv_count = rcv_pkt_user(
                            lcore_id, rcv_payload_list, rcv_payload_len_list,
                            0, 0, 0,
                            0, 0,
                            1, 10);

                    for(uint32_t i = 0; i < rcv_count; i++){
                        rcv_payload_len = rcv_payload_len_list[i];
                        rte_memcpy(rcv_payload, rcv_payload_list + rcv_payload_bias, rcv_payload_len);
                        rcv_payload_bias += rcv_payload_len;

                        if(rcv_op_payload->op_type == OP_UNLOCK_STAT_FPGA){
                            send_payload_len = sizeof(only_op_payload);

                            mbuf = rte_pktmbuf_alloc(pktmbuf_pool_list[rx_queue_id]);
                            pkt_gen.setMbuf(mbuf);
                            pkt_gen.setDstAddrEth((uint8_t *) CTRL_MAC);
                            pkt_gen.setDstAddrIp((uint8_t *) CTRL_IP);

                            send_op_payload->op_type = OP_UNLOCK_STAT_FPGA;

                            pkt_gen.generate_pkt(send_payload, send_payload_len);
                            enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                            if (debug_flag) std::cout << "response OP_UNLOCK_STAT_FPGA" << std::endl;
                        }
                        else if(rcv_op_payload->op_type == OP_LOCK_STAT_FPGA){

                            lock_stat_flag = 1;

                            send_payload_len = sizeof(only_op_payload);

                            mbuf = rte_pktmbuf_alloc(pktmbuf_pool_list[rx_queue_id]);
                            pkt_gen.setMbuf(mbuf);
                            pkt_gen.setDstAddrEth((uint8_t *) CTRL_MAC);
                            pkt_gen.setDstAddrIp((uint8_t *) CTRL_IP);

                            send_op_payload->op_type = OP_LOCK_STAT_FPGA;

                            pkt_gen.generate_pkt(send_payload, send_payload_len);
                            enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                            if (debug_flag) std::cout << "response OP_LOCK_STAT_FPGA" << std::endl;
                        }
                        else if(rcv_op_payload->op_type == OP_GET_HOT_COUNTER_FPGA){
                            if (debug_flag) std::cout << "STATS_COLLECT: start (rcv) OP_GET_HOT_COUNTER_FPGA" << std::endl;
                            stats_stage = STATS_ANALYZE;
                            next_stage_flag = 1;
                        }
                    }
                }while(next_stage_flag == 0);

                break;
            }
            case STATS_ANALYZE:{

//                if(debug_flag){
//                    std::cout << "input 1 to start STATS_ANALYZE:" << std::endl;
//                    uint32_t start_word = 0;
//                    while(start_word != 1){
//                        std::cin >> start_word;
//                    }
//                    std::cout << "STATS_ANALYZE starts!" << std::endl;
//                }

                std::lock_guard<std::mutex> lock(mtx);

                for (int m = 0; m < num_stats_lcore; m++) {
//                    if(debug_flag){
//                        std::cout << "lcore_to_info_list[" << m << "][0]: " <<
//                                  "obj = " << lcore_to_info_list[m][0].obj <<
//                                  ", count = " << lcore_to_info_list[m][0].count << std::endl;
//                    }
                    for (int n = 0; n < TOPK; n++) {

                        if(debug_flag){
                            std::cout << "lcore_to_info_list[" << m << "][" << n + 1 << "]: " <<
                                      "obj = " << lcore_to_info_list[m][n + 1].obj <<
                                      ", count = " << lcore_to_info_list[m][n + 1].count << std::endl;
                        }

                        merge_to_info_list[m * TOPK + n] = lcore_to_info_list[m][n + 1];
                        lcore_to_info_list[m][n + 1].count = 0;
                        lcore_to_info_list[m][n + 1].obj = INVALID_4B;


                    }

                }

                std::sort(
                        merge_to_info_list,
                        merge_to_info_list + num_stats_lcore * TOPK,
                        [](const topk_obj_info &a, const topk_obj_info &b){
                            return a.count > b.count;
                        });

                if(debug_flag){
                     for(int n=0; n<TOPK; n++){
                         std::cout << "merge_to_info_list: " <<
                                   "obj = " << merge_to_info_list[n].obj <<
                                   ", count = " << merge_to_info_list[n].count << std::endl;
                     }
                }

                next_stage_flag = 0;
                do{
                    auto ho_payload = (hot_obj_payload *)send_payload;
                    send_payload_len = sizeof(hot_obj_payload);
                    for(int j = 0; j < TOPK / HOTOBJ_PER_PKT; j++){
                        mbuf = rte_pktmbuf_alloc(pktmbuf_pool_list[rx_queue_id]);
                        pkt_gen.setMbuf(mbuf);
                        pkt_gen.setDstAddrEth((uint8_t *) CTRL_MAC);
                        pkt_gen.setDstAddrIp((uint8_t *) CTRL_IP);

                        ho_payload->op_type = OP_GET_HOT_COUNTER_FPGA;
                        ho_payload->obj_num = HOTOBJ_PER_PKT;

                        // need to complete hot info and cold info
                        for(int k = 0; k < HOTOBJ_PER_PKT; k++){
                            uint32_t obj_index = j * HOTOBJ_PER_PKT + k;
                            ho_payload->obj[obj_index] = htonl(merge_to_info_list[obj_index].obj);
                            ho_payload->counter[obj_index] = htonl(merge_to_info_list[obj_index].count);
                        }
                        ho_payload->pkt_index = j;
                        ho_payload->pkt_num = TOPK / HOTOBJ_PER_PKT;
                        pkt_gen.generate_pkt(send_payload, send_payload_len);
                        enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                    }
                    if (debug_flag) std::cout << "response OP_GET_HOT_COUNTER_FPGA" << std::endl;

                    auto rcv_op_payload = (only_op_payload *) rcv_payload;
                    auto send_op_payload = (only_op_payload *) send_payload;
                    do{
                        rcv_payload_bias = 0;
                        rcv_count = rcv_pkt_user(
                                lcore_id, rcv_payload_list, rcv_payload_len_list,
                                0, 0, 0,
                                0, 0,
                                1, 100);
                        for(uint32_t i = 0; i < rcv_count; i++) {
                            rcv_payload_len = rcv_payload_len_list[i];
                            rte_memcpy(rcv_payload, rcv_payload_list + rcv_payload_bias, rcv_payload_len);
                            rcv_payload_bias += rcv_payload_len;

                            if (rcv_op_payload->op_type == OP_GET_HOT_COUNTER_FPGA) {
                                if (debug_flag) std::cout << "start (rcv) OP_GET_HOT_COUNTER_FPGA" << std::endl;
                            }
                            else if(rcv_op_payload->op_type == OP_GET_END_2_UPDATE_FPGA){
                                mbuf = rte_pktmbuf_alloc(pktmbuf_pool_list[rx_queue_id]);
                                pkt_gen.setMbuf(mbuf);
                                pkt_gen.setDstAddrEth((uint8_t *) CTRL_MAC);
                                pkt_gen.setDstAddrIp((uint8_t *) CTRL_IP);
                                send_op_payload->op_type = OP_GET_END_2_UPDATE_FPGA;
                                enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                                if (debug_flag) std::cout << "response OP_GET_END_2_UPDATE_FPGA" << std::endl;
                                next_stage_flag = 1;
                                stats_stage = STATS_UPDATE;
                            }
                            else if(rcv_op_payload->op_type == OP_GET_END_2_CLEAN_FPGA){
                                mbuf = rte_pktmbuf_alloc(pktmbuf_pool_list[rx_queue_id]);
                                pkt_gen.setMbuf(mbuf);
                                pkt_gen.setDstAddrEth((uint8_t *) CTRL_MAC);
                                pkt_gen.setDstAddrIp((uint8_t *) CTRL_IP);
                                send_op_payload->op_type = OP_GET_END_2_CLEAN_FPGA;
                                enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                                if (debug_flag) std::cout << "response OP_GET_END_2_CLEAN_FPGA" << std::endl;
                                next_stage_flag = 1;
                                stats_stage = STATS_CLEAN;
                            }
                        }
                    }while(next_stage_flag == 0);
                }while(next_stage_flag == 0);
                break;
            }

            case STATS_UPDATE:{

                next_stage_flag = 0;
                std::unordered_set<uint32_t> pkt_index_set;
                uint32_t pkt_num = 0;

                auto rcv_op_payload = (only_op_payload *) rcv_payload;
                auto send_op_payload = (only_op_payload *) send_payload;
                auto rcv_chos_payload = (change_hot_obj_state_payload *)rcv_payload;

                for(uint32_t i = 0; i < TOPK; i++){
                    uo_info.hot_obj = INVALID_4B;
                    uo_info.cold_obj = INVALID_4B;
                    uo_info.cold_obj_index = INVALID_4B;
                    uo_info_list[i] = uo_info;
                }

                do{
                    rcv_payload_bias = 0;
                    rcv_count = rcv_pkt_user(
                            lcore_id, rcv_payload_list, rcv_payload_len_list,
                            0, 0, 0,
                            0, 0,
                            1, 10);

                    rcv_payload_bias = 0;
                    for (uint32_t j = 0; j < rcv_count; j++) {
                        rcv_payload_len = rcv_payload_len_list[j];
                        rte_memcpy(rcv_payload, rcv_payload_list + rcv_payload_bias, rcv_payload_len);
                        rcv_payload_bias += rcv_payload_len;

                        if (rcv_payload[UDP_HEADER_SIZE] == OP_GET_END_2_UPDATE_FPGA) {
                            mbuf = rte_pktmbuf_alloc(pktmbuf_pool_list[rx_queue_id]);
                            pkt_gen.setMbuf(mbuf);
                            pkt_gen.setDstAddrEth((uint8_t *) CTRL_MAC);
                            pkt_gen.setDstAddrIp((uint8_t *) CTRL_IP);
                            send_op_payload->op_type = OP_GET_END_2_UPDATE_FPGA;
                            enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                            if (debug_flag) std::cout << "response OP_GET_END_2_UPDATE_FPGA" << std::endl;
                        }
                        else if (rcv_payload[UDP_HEADER_SIZE] == OP_MOD_STATE_UPD_FPGA) {
                            for (uint32_t k = 0; k < ntohl(rcv_chos_payload->obj_num); k++) {
                                uint32_t index = ntohl(rcv_chos_payload->pkt_index * HOTOBJ_PER_PKT + k);
                                uo_info.hot_obj = ntohl(rcv_chos_payload->hot_obj[k]);
                                uo_info.cold_obj = ntohl(rcv_chos_payload->cold_obj[k]);
                                uo_info.cold_obj_index = ntohl(rcv_chos_payload->cold_obj_index[k]);
                                uo_info_list[index] = uo_info;

                                pkt_num = ntohl(rcv_chos_payload->pkt_num);
                                pkt_index_set.insert(index);
                            }

                            if(pkt_index_set.size() == pkt_num){
                                mbuf = rte_pktmbuf_alloc(pktmbuf_pool_list[rx_queue_id]);
                                pkt_gen.setMbuf(mbuf);
                                pkt_gen.setDstAddrEth((uint8_t *) CTRL_MAC);
                                pkt_gen.setDstAddrIp((uint8_t *) CTRL_IP);
                                send_op_payload->op_type = OP_MOD_STATE_UPD_FPGA;
                                enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                                if (debug_flag) std::cout << "response OP_MOD_STATE_UPD_FPGA" << std::endl;
                            }
                        }
                    }
                }while(pkt_index_set.size() == pkt_num);

                upd_obj_flag = 1;
                move_appdata_fromSDPtoBS(uo_info_list);
                move_appdata_fromBStoSDP(uo_info_list);

                do {
                    rcv_payload_bias = 0;
                    rcv_count = rcv_pkt_user(
                            lcore_id, rcv_payload_list, rcv_payload_len_list,
                            0, 0, 0,
                            0, 0,
                            1, 10);
                    rcv_payload_bias = 0;
                    for (uint32_t j = 0; j < rcv_count; j++) {
                        rcv_payload_len = rcv_payload_len_list[j];
                        rte_memcpy(rcv_payload, rcv_payload_list + rcv_payload_bias, rcv_payload_len);
                        rcv_payload_bias += rcv_payload_len;

                        if (rcv_op_payload->op_type == OP_MOD_STATE_UPD_FPGA) {
                            mbuf = rte_pktmbuf_alloc(pktmbuf_pool_list[rx_queue_id]);
                            pkt_gen.setMbuf(mbuf);
                            pkt_gen.setDstAddrEth((uint8_t *) CTRL_MAC);
                            pkt_gen.setDstAddrIp((uint8_t *) CTRL_IP);
                            send_op_payload->op_type = OP_MOD_STATE_UPD_FPGA;
                            enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                            if (debug_flag) std::cout << "response OP_MOD_STATE_UPD_FPGA" << std::endl;
                        }
                        else if (rcv_op_payload->op_type == OP_MOD_STATE_NOUPD_FPGA) {
                            mbuf = rte_pktmbuf_alloc(pktmbuf_pool_list[rx_queue_id]);
                            pkt_gen.setMbuf(mbuf);
                            pkt_gen.setDstAddrEth((uint8_t *) CTRL_MAC);
                            pkt_gen.setDstAddrIp((uint8_t *) CTRL_IP);
                            send_op_payload->op_type = OP_MOD_STATE_NOUPD_FPGA;
                            enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                            if (debug_flag) std::cout << "response OP_MOD_STATE_NOUPD_FPGA" << std::endl;
                            next_stage_flag = 1;
                            stats_stage = STATS_CLEAN;
                        }
                    }
                }while(next_stage_flag == 0);
                break;
            }
            case STATS_CLEAN:{
                next_stage_flag = 0;
                uint32_t inverse_clean_flag = ~clean_flag;

                auto rcv_op_payload = (only_op_payload *) rcv_payload;
                auto send_op_payload = (only_op_payload *) send_payload;

                do {
                    rcv_payload_bias = 0;
                    rcv_count = rcv_pkt_user(
                            lcore_id, rcv_payload_list, rcv_payload_len_list,
                            0, 0, 0,
                            0, 0,
                            1, 10);
                    rcv_payload_bias = 0;
                    for (uint32_t j = 0; j < rcv_count; j++) {
                        rcv_payload_len = rcv_payload_len_list[j];
                        rte_memcpy(rcv_payload, rcv_payload_list + rcv_payload_bias, rcv_payload_len);
                        rcv_payload_bias += rcv_payload_len;

                        if (rcv_op_payload->op_type == OP_GET_END_2_UPDATE_FPGA) {
                            mbuf = rte_pktmbuf_alloc(pktmbuf_pool_list[rx_queue_id]);
                            pkt_gen.setMbuf(mbuf);
                            pkt_gen.setDstAddrEth((uint8_t *) CTRL_MAC);
                            pkt_gen.setDstAddrIp((uint8_t *) CTRL_IP);
                            send_op_payload->op_type = OP_GET_END_2_UPDATE_FPGA;
                            enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                            if (debug_flag) std::cout << "response OP_GET_END_2_UPDATE_FPGA" << std::endl;
                        }
                        else if(rcv_op_payload->op_type == OP_MOD_STATE_NOUPD_FPGA){
                            mbuf = rte_pktmbuf_alloc(pktmbuf_pool_list[rx_queue_id]);
                            pkt_gen.setMbuf(mbuf);
                            pkt_gen.setDstAddrEth((uint8_t *) CTRL_MAC);
                            pkt_gen.setDstAddrIp((uint8_t *) CTRL_IP);
                            send_op_payload->op_type = OP_MOD_STATE_NOUPD_FPGA;
                            enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                            if (debug_flag) std::cout << "response OP_MOD_STATE_NOUPD_FPGA" << std::endl;
                        }
                        else if (rcv_op_payload->op_type == OP_CLN_HOT_COUNTER_FPGA) {
                            upd_obj_flag = 0;
                            clean_flag = inverse_clean_flag;
                            mbuf = rte_pktmbuf_alloc(pktmbuf_pool_list[rx_queue_id]);
                            pkt_gen.setMbuf(mbuf);
                            pkt_gen.setDstAddrEth((uint8_t *) CTRL_MAC);
                            pkt_gen.setDstAddrIp((uint8_t *) CTRL_IP);
                            send_op_payload->op_type = OP_CLN_HOT_COUNTER_FPGA;
                            enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);

                            if (debug_flag) std::cout << "response OP_CLN_HOT_COUNTER_FPGA" << std::endl;
                        }
                        else if (rcv_op_payload->op_type == OP_UNLOCK_STAT_FPGA) {
                            mbuf = rte_pktmbuf_alloc(pktmbuf_pool_list[rx_queue_id]);
                            pkt_gen.setMbuf(mbuf);
                            pkt_gen.setDstAddrEth((uint8_t *) CTRL_MAC);
                            pkt_gen.setDstAddrIp((uint8_t *) CTRL_IP);
                            send_op_payload->op_type = OP_UNLOCK_STAT_FPGA;
                            enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                            if (debug_flag) std::cout << "response OP_UNLOCK_STAT_FPGA" << std::endl;
                            next_stage_flag = 1;
                            stats_stage = STATS_CLEAN;
                            lock_stat_flag = 0;
                        }
                    }
                }while(next_stage_flag == 0);
            }
        }

//        /** server stats **/
//        // print stats at master lcore
//        t_program.finish();
//        if (t_program.get_interval_second() > 1) {
//            print_per_core_throughput();
//            t_program.start();
//        }
    }

    return 0;
}


void minheap_downAdjust(topk_obj_info heap[], int low, int high){
    int i = low, j = i * 2;
    while(j <= high){
        if(j+1 <= high && heap[j+1].count < heap[j].count){
            j = j + 1;
        }
        if(heap[j].count < heap[i].count){
            topk_obj_info temp = heap[i];
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

void stats_update(uint_obj obj, uint32_t stats_id){
    if(!lock_stat_flag){
        topk_obj_info to_info = {};
        to_info.obj = obj;

        if(clean_flag == 1){
            to_info.count = ++stats_0[stats_id][obj];
//            if(debug_flag){
//                std::cout << "clean_flag == 1, obj = " << obj << ", stats_0[stats_id] = " << stats_0[stats_id][obj] << std::endl;
//                std::cout << "clean_flag == 1, obj = " << obj << ", stats_0[stats_id].at(obj) = " << stats_0[stats_id].at(obj) << std::endl;
//                std::cout << "clean_flag == 1, obj = " << obj << ", to_info.count = " << to_info.count << std::endl;
//            }
        }
        else{
            to_info.count = ++stats_1[stats_id][obj];
//            if(debug_flag){
//                std::cout << "clean_flag == 0, obj = " << obj << ", stats_1[stats_id] = " << stats_1[stats_id][obj] << std::endl;
//                std::cout << "clean_flag == 0, obj = " << obj << ", stats_1[stats_id].at(obj) = " << stats_1[stats_id].at(obj) << std::endl;
//                std::cout << "clean_flag == 0, obj = " << obj << ", to_info.count = " << to_info.count << std::endl;
//            }
        }

        uint32_t jump_flag = 0;
        for(int k=1; k<=TOPK; k++){
            if(lcore_to_info_list[stats_id][k].obj == to_info.obj){
//                lcore_to_info_list[stats_id][k].count = to_info.count;
                jump_flag = 1;
                break;
            }
        }

//        if(debug_flag){
//            std::cout << "obj = " << obj << ", jump_flag = " << jump_flag << std::endl;
//            std::cout << "heap:\t";
//            for(int k=1; k<=TOPK; k++){
//                std::cout << lcore_to_info_list[stats_id][k].obj << ' ';
//            }
//            std::cout << std::endl;
//        }

        if(!jump_flag && to_info.count > lcore_to_info_list[stats_id][1].count){
//            if(debug_flag){
//                std::cout << "obj = " << obj << ", into the heap!" << std::endl;
//                std::cout << "heap:\t";
//                for(int k=1; k<=TOPK; k++){
//                    std::cout << lcore_to_info_list[stats_id][k].obj << ' ';
//                }
//                std::cout << std::endl;
//            }
            std::lock_guard<std::mutex> lock(mtx);
            lcore_to_info_list[stats_id][1] = to_info;
            minheap_downAdjust(lcore_to_info_list[stats_id], 1, TOPK);
        }

//        if(debug_flag){
//            int count = 0;
//            for(int k=1; k<=TOPK; k++){
//                if(lcore_to_info_list[stats_id][k].obj == to_info.obj){
//                    count++;
//                }
//            }
//            if(count >= 2){
//                std::cout << "obj = " << obj << ", jump_flag = " << jump_flag << std::endl;
//                std::cout << "heap:\t";
//                for(int k=1; k<=TOPK; k++){
//                    std::cout << lcore_to_info_list[stats_id][k].obj << ' ';
//                }
//                std::cout << std::endl;
//            }
//        }

    }
}




throughput_statistics tput_stat[NC_MAX_LCORES];

void print_per_core_throughput(void) {
    // time is in second
    printf("%lld\nthroughput\n", (long long)time(NULL));
    uint32_t i, j;

    fflush(stdout);

    float total_tx = 0;
    float total_rx = 0;

    for (j = 0; j < n_lcore; j++) {
        i = rx_queue_id_2_lcore_id_list[j];
         printf(" core %" PRIu32"  "
             "tx: %" PRIu64"  "
             "rx: %" PRIu64"  \n",
             i,
             tput_stat[i].tx       - tput_stat[i].last_tx      ,
             tput_stat[i].rx       - tput_stat[i].last_rx
             );

        total_tx += tput_stat[i].tx       - tput_stat[i].last_tx;
        total_rx += tput_stat[i].rx       - tput_stat[i].last_rx;

        tput_stat[i].last_tx        = tput_stat[i].tx      ;
        tput_stat[i].last_rx        = tput_stat[i].rx      ;
    }
    std::cout << "total_tx = " << total_tx / 1000000 << " Mqps" << std::endl;
    std::cout << "total_rx = " << total_rx / 1000000 << " Mqps" << std::endl;
}