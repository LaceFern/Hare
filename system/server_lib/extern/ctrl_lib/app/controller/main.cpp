#include <iostream>
#include <algorithm>
#include "dpdk_basic.h"
#include "dpdk_udp.h"
#include "general_ctrl.h"
#include "p4_ctrl.h"
#include "dpdk_ctrl.h"
#include <unordered_map>

static void main_ctrl(uint32_t lcore_id, uint16_t rx_queue_id) {

//    SwitchControlPlane scp("P4KVS");
    SwitchControlPlane scp("P4KVS");
    scp.transfer_init();
    /***************************** variables *****************************/
    rte_mbuf *mbuf;

    uint8_t rcv_payload_list[524288]; //65536
    uint32_t rcv_payload_len_list[4096]; //1024
    uint8_t rcv_payload[4096];
    uint32_t rcv_payload_len;
    uint32_t rcv_payload_bias;
    uint8_t send_payload[4096];
    uint32_t send_payload_len;

    ctrl_para para;

    cold_obj_info co_info = {};
    cold_obj_info co_info_heap_extend[TOPK + 1] = {0};
    cold_obj_info co_info_heap[TOPK] = {0};

    hot_obj_info ho_info = {};
    hot_obj_info ho_info_heap[TOPK] = {0};
    uint32_t ho_num = 0;

    upd_obj_info uo_info = {};
    upd_obj_info uo_info_list[TOPK] = {};
    upd_obj_info uo_info_pernode_list[NUM_BACKENDNODE][TOPK];
    uint32_t uo_info_num_pernode_list[NUM_BACKENDNODE];
    int32_t uo_num = 0;

    uint32_t loop_count = 0;
    uint32_t rcv_count = 0;

    lcore_configuration *lconf = &lcore_conf_list[lcore_id];

    Timer t_program;
    Timer t_wait;
    Timer t_temp;

    uint32_t restart_times = 0;

    std::unordered_map<uint32_t, uint_obj> obj_index_2_obj_mapping;
    for (uint64_t i = init_begin; i < init_end; ++i) {
        uint_obj obj = i;
        uint32_t obj_index = i;
        obj_index_2_obj_mapping[obj_index] = obj;
    }

    PacketGenerator pkt_gen(
            lcore_id, nullptr,
            (uint8_t *) SWITCH_MAC, (uint8_t *) CTRL_MAC,
            (uint8_t *) SWITCH_IP, (uint8_t *) CTRL_IP,
            UDP_CTRL_PORT, UDP_CTRL_PORT);


//    PacketGenerator pkt_gen(
//            lcore_id, nullptr,
//            (uint8_t *) AMAX1_MAC, (uint8_t *) CTRL_MAC,
//            (uint8_t *) AMAX1_IP, (uint8_t *) CTRL_IP,
//            UDP_CTRL_PORT, UDP_CTRL_PORT);


    std::cout << "input 1 to start control:" << std::endl;
    uint32_t start_word;
    std::cin >> start_word;
    std::cout << "control starts!" << std::endl;

    /***************************** work *****************************/
    t_program.start();
    t_wait.start();
    if(debug_flag) std::cout << "STATS_COLLECT starts" << std::endl;
    while(true){
        t_program.finish();
        if(unlikely(t_program.get_interval_second() > END_S)){
            std::cout << "average_loop_time = " << t_program.get_interval_millisecond()/loop_count << "ms" << std::endl;
            break;
        }

        switch(ctrl_stage){
            case STATS_COLLECT: {
                t_wait.finish();
                if(t_wait.get_interval_millisecond() > para.wait_millisecond){
                    if(debug_flag) std::cout << "STATS_COLLECT ends" << std::endl;

                    /**** OP_LOCK_STAT_SWITCH ****/
                    t_temp.start();

                    auto op_payload = (only_op_payload *)send_payload;
                    send_payload_len = sizeof(only_op_payload);

                    restart_times = 0;
                    do{
                        mbuf = rte_pktmbuf_alloc(pktmbuf_pool_list[rx_queue_id]);
                        pkt_gen.setMbuf(mbuf);

                        op_payload->op_type = OP_LOCK_STAT_SWITCH;

                        pkt_gen.generate_pkt(send_payload, send_payload_len);

//                        if(debug_flag){
//                            uint8_t * tmp = rte_pktmbuf_mtod(mbuf, uint8_t *);
//                            for(uint32_t i = 0; i < mbuf->pkt_len; i++){
//                                std::cout << std::hex << (uint32_t)(tmp[i]) << "\t";
//                            }
//                            std::cout << std::endl;
//
//                            rte_ether_hdr* eth = rte_pktmbuf_mtod(mbuf, rte_ether_hdr *);
//                            auto *ip = (rte_ipv4_hdr *)((uint8_t*) eth + sizeof(rte_ether_hdr));
//                            auto *udp = (rte_udp_hdr *)((uint8_t*) ip + sizeof(rte_ipv4_hdr));
//                            auto *pld = (uint8_t*)eth + UDP_HEADER_SIZE;
//                            std::cout << "src udp port = " << ntohs(udp->src_port) << std::endl;
//                            std::cout << "dst udp port = " << ntohs(udp->dst_port) << std::endl;
//                        }

                        enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                        if(debug_flag) std::cout << "send OP_LOCK_STAT_SWITCH" << std::endl;

                        if(debug_flag) std::cout << "rcv_pkt_user function starts" << std::endl;
                        rcv_count = rcv_pkt_user(
                                lcore_id, rcv_payload_list, rcv_payload_len_list,
                                1, 0, OP_LOCK_STAT_SWITCH,
                                1, 1,
                                1, 100);
                        if(debug_flag) std::cout << "rcv_pkt_user function ends" << std::endl;

//                        if(debug_flag){
//                        rcv_count = rcv_complete_pkt_user(
//                                lcore_id, rcv_payload_list, rcv_payload_len_list,
//                                1, 0 + UDP_HEADER_SIZE, OP_LOCK_STAT_SWITCH,
//                                1, 1,
//                                1, 100);
//                            rcv_payload_bias = 0;
//                            for (uint32_t j = 0; j < rcv_count; j++) {
//                                rcv_payload_len = rcv_payload_len_list[j];
//                                rte_memcpy(rcv_payload, rcv_payload_list + rcv_payload_bias, rcv_payload_len);
//                                rcv_payload_bias += rcv_payload_len;
//
//                                for(uint32_t i = 0; i < rcv_payload_len; i++){
//                                    std::cout << std::hex << (uint32_t)(rcv_payload[i]) << "\t";
//                                }
//                            }
//                        }

                        restart_times++;
                        if(restart_times == RESTART_TIMES){
                            uint32_t quit_flag = 0;
                            std::cout << "error: waiting OP_LOCK_STAT_SWITCH > " << RESTART_TIMES << " times" << std::endl;
                            std::cout << "input 0 to wait, input others to quit:\n";
                            std::cin >> quit_flag;
                            if(quit_flag == 0){
                                restart_times = 0;
                            }
                            else{
                                exit(1);
                            }
                        }
                    }while(rcv_count < 1);

                    t_temp.finish();
                    if(debug_flag) std::cout << "receive OP_LOCK_STAT_SWITCH" << std::endl;
                    if(debug_flag) std::cout << "time interval = "
                                            << t_temp.get_interval_microsecond()
                                            << "us" << std::endl;

                    /**** OP_LOCK_STAT_FPGA ****/
                    t_temp.start();
                    for(int i = 0; i < NUM_BACKENDNODE; i++){
                        restart_times = 0;
                        do{
                            mbuf = rte_pktmbuf_alloc(pktmbuf_pool_list[rx_queue_id]);
                            pkt_gen.setMbuf(mbuf);
                            pkt_gen.setDstAddrEth((uint8_t *) backend_node_mac_list[i]);
                            pkt_gen.setDstAddrIp((uint8_t *) backend_node_ip_list[i]);

//                            if(debug_flag){
//                                std::cout << "before send payload" << std::endl;
//                                for(uint32_t j = 0; j < send_payload_len; j++){
//                                    std::cout << std::hex << (uint32_t)(send_payload[j]) << "\t";
//                                }
//                            }

                            op_payload->op_type = OP_LOCK_STAT_FPGA;


//                            if(debug_flag){
//                                std::cout << "after send payload" << std::endl;
//                                for(uint32_t j = 0; j < send_payload_len; j++){
//                                    std::cout << std::hex << (uint32_t)(send_payload[j]) << "\t";
//                                }
//                            }

                            pkt_gen.generate_pkt(send_payload, send_payload_len);
                            enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);

                            if(debug_flag) std::cout << "send OP_LOCK_STAT_FPGA [" << i << "]" << std::endl;
//                            if(debug_flag){
//                                uint8_t * tmp = rte_pktmbuf_mtod(mbuf, uint8_t *);
//                                for(uint32_t i = 0; i < mbuf->pkt_len; i++){
//                                    std::cout << std::hex << (uint32_t)(tmp[i]) << "\t";
//                                }
//                                std::cout << std::endl;
//
//                                rte_ether_hdr* eth = rte_pktmbuf_mtod(mbuf, rte_ether_hdr *);
//                                auto *ip = (rte_ipv4_hdr *)((uint8_t*) eth + sizeof(rte_ether_hdr));
//                                auto *udp = (rte_udp_hdr *)((uint8_t*) ip + sizeof(rte_ipv4_hdr));
//                                auto *pld = (uint8_t*)eth + UDP_HEADER_SIZE;
//                                std::cout << "src udp port = " << ntohs(udp->src_port) << std::endl;
//                                std::cout << "dst udp port = " << ntohs(udp->dst_port) << std::endl;
//                            }

                            rcv_count = rcv_pkt_user(
                                    lcore_id, rcv_payload_list, rcv_payload_len_list,
                                    1, 0, OP_LOCK_STAT_FPGA,
                                    1, 1,
                                    1, 100);

//                            if(debug_flag){
//                                rcv_count = rcv_complete_pkt_user(
//                                        lcore_id, rcv_payload_list, rcv_payload_len_list,
//                                        1, 0 + UDP_HEADER_SIZE, OP_LOCK_STAT_FPGA,
//                                        1, 1,
//                                        1, 100);
//                                rcv_payload_bias = 0;
//                                for (uint32_t j = 0; j < rcv_count; j++) {
//                                    rcv_payload_len = rcv_payload_len_list[j];
//                                    rte_memcpy(rcv_payload, rcv_payload_list + rcv_payload_bias, rcv_payload_len);
//                                    rcv_payload_bias += rcv_payload_len;
//
//                                    for(uint32_t i = 0; i < rcv_payload_len; i++){
//                                        std::cout << std::hex << (uint32_t)(rcv_payload[i]) << "\t";
//                                    }
//                                }
//                            }

                            restart_times++;
                            if(restart_times == RESTART_TIMES){
                                uint32_t quit_flag = 0;
                                std::cout << "error: waiting OP_LOCK_STAT_FPGA > " << RESTART_TIMES << "times" << std::endl;
                                std::cout << "input 0 to wait, input others to quit:\n";
                                std::cin >> quit_flag;
                                if(quit_flag == 0){
                                    restart_times = 0;
                                }
                                else{
                                    exit(1);
                                }
                            }

                        }while(rcv_count < 1);

                        if(debug_flag) std::cout << "receive OP_LOCK_STAT_FPGA [" << i << "]" << std::endl;
                    }
                    if(debug_flag) std::cout << "time interval = "
                                             << t_temp.get_interval_microsecond()
                                             << "us" << std::endl;


//                    if(debug_flag == 1){
//                        std::cout << "input 1 to start STATS_ANALYZE:" << std::endl;
//                        uint32_t start_word;
//                        std::cin >> start_word;
//                        std::cout << "STATS_ANALYZE starts!" << std::endl;
//                    }


                    ctrl_stage = STATS_ANALYZE;
                }
                break;
            }
            case STATS_ANALYZE: {
                if(debug_flag) std::cout << "STATS_ANALYZE starts" << std::endl;

                t_temp.start();
                uint32_t loop = 32768;//8
                uint32_t batch = para.cache_size / COLDCOUNT_PER_PKT / loop;

                for(uint32_t i = 0; i < TOPK + 1; i++){
                    co_info.obj_index = INVALID_4B;
                    co_info.cold_count = INVALID_4B;
                    co_info_heap_extend[i] = co_info;
                }

                pkt_gen.setDstAddrEth((uint8_t *)SWITCH_MAC);
                pkt_gen.setDstAddrIp((uint8_t *)SWITCH_IP);
                auto co_send_payload = (fetch_co_payload *)send_payload;
                auto co_rcv_payload = (fetch_co_payload *)rcv_payload;
                send_payload_len = sizeof(fetch_co_payload);

                for(uint32_t batch_count = 0; batch_count < loop; batch_count++) {
                    do {
//                        if (debug_flag) std::cout << "batch [" << batch_count << "]" << std::endl;
                        for (uint32_t i = 0; i < batch; i++) {
                            mbuf = rte_pktmbuf_alloc(pktmbuf_pool_list[rx_queue_id]);
                            pkt_gen.setMbuf(mbuf);

                            co_send_payload->op_type = OP_GET_COLD_COUNTER_SWITCH;
                            co_send_payload->obj_index = htonl(batch_count * batch + i);

                            pkt_gen.generate_pkt(send_payload, send_payload_len);
                            enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
//                            if (debug_flag) std::cout << "send OP_GET_COLD_COUNTER_SWITCH [" << batch_count * batch + i << "]" << std::endl;

//                            if(debug_flag){
//                                std::cout << "send:   ";
//                                for(uint32_t j = 0; j < send_payload_len; j++){
//                                    std::cout << std::hex << (uint32_t)(send_payload[j]) << "\t";
//                                }
//                            }
                        }

                        rcv_count = rcv_pkt_user(
                                lcore_id, rcv_payload_list, rcv_payload_len_list,
                                1, 0, OP_GET_COLD_COUNTER_SWITCH,
                                1, batch,
                                1, 1000);

//                        rcv_count = rcv_pkt_user(
//                                lcore_id, rcv_payload_list, rcv_payload_len_list,
//                                0, 0, OP_GET_COLD_COUNTER_SWITCH,
//                                1, batch,
//                                1, 1000);
//                        if (debug_flag) std::cout << "batch = " << batch << ", rcv_count = " << rcv_count << std::endl;

//                        if(debug_flag){
//                            rcv_payload_bias = 0;
//                            for (uint32_t j = 0; j < rcv_count; j++) {
//                                rcv_payload_len = rcv_payload_len_list[j];
//                                rte_memcpy(rcv_payload, rcv_payload_list + rcv_payload_bias, rcv_payload_len);
//                                rcv_payload_bias += rcv_payload_len;
//
//                                std::cout << "recv:   ";
//                                for(uint32_t i = 0; i < rcv_payload_len; i++){
//                                    std::cout << std::hex << (uint32_t)(rcv_payload[i]) << "\t";
//                                }
//                                std::cout << std::endl;
//                            }
//                        }

                        rcv_payload_bias = 0;
                        for (uint32_t j = 0; j < rcv_count; j++) {
                            rcv_payload_len = rcv_payload_len_list[j];
                            rte_memcpy(rcv_payload, rcv_payload_list + rcv_payload_bias, rcv_payload_len);
                            rcv_payload_bias += rcv_payload_len;

                            for (uint32_t k = 0; k < COLDCOUNT_PER_PKT; k++) {
                                co_info.obj_index = ntohl(co_rcv_payload->obj_index) * COLDCOUNT_PER_PKT + k;
                                co_info.cold_count = ntohl(co_rcv_payload->obj_count[k]);
                                if (co_info.cold_count < co_info_heap_extend[1].cold_count) {
                                    co_info_heap_extend[1] = co_info;
                                    maxheap_downAdjust(co_info_heap_extend, 1, TOPK);
                                }
//                                if (debug_flag) std::cout << "receive OP_GET_COLD_COUNTER_SWITCH [" << co_send_payload->obj_index << "]" << std::endl;
                            }

                        }
                    } while (rcv_count < batch);
                }
                for(uint32_t i = 0; i < TOPK; i++){
                    co_info_heap[i] = co_info_heap_extend[1 + i];
                }

//                if(debug_flag){
//                    std::cout << "co_info_heap_extend (cold_obj_index, cold_count):" << std::endl;
//                    for(int i = 0; i < TOPK; i++){
//                        std::cout << "(" << co_info_heap_extend[i].obj_index << ", " << co_info_heap_extend[i].cold_count << ")" << "\t";
//                    }
//                }

                std::sort(
                        co_info_heap,
                        co_info_heap + TOPK,
                        [](const cold_obj_info &a, const cold_obj_info &b){
                            return a.cold_count > b.cold_count;
                        });
                t_temp.finish();
                if(debug_flag) std::cout << "time interval = "
                                         << t_temp.get_interval_microsecond()
                                         << "us" << std::endl;
                if(debug_flag) std::cout << "throuhgput = "
                                         << para.cache_size / COLDCOUNT_PER_PKT / t_temp.get_interval_microsecond()
                                         << "Mqps" << std::endl;

                if(debug_flag){
                    std::cout << "Top cold objects (cold_obj_index, cold_count):" << std::endl;
                    for(int i = 0; i < TOPK; i++){
                        std::cout << "(" << co_info_heap[i].obj_index << ", " << co_info_heap[i].cold_count << ")" << "\t";
                    }
                    std::cout << std::endl;
                }

                if(debug_flag){
                    std::cout << "input 1 to start next step:" << std::endl;
                    uint32_t start_word = 0;
                    while(start_word != 1){
                        std::cin >> start_word;
                    }
                    std::cout << "next step starts!" << std::endl;
                }

                t_temp.start();
                auto op_payload = (only_op_payload *)send_payload;
//                auto hot_obj_payload = reinterpret_cast<struct hot_obj_payload *>(rcv_payload);
                auto ho_payload = (hot_obj_payload *)rcv_payload;
                send_payload_len = sizeof(only_op_payload);
                uint32_t heap_bias = 0;
                for(int i = 0; i < NUM_BACKENDNODE; i++){
                    restart_times = 0;
                    do{
                        mbuf = rte_pktmbuf_alloc(pktmbuf_pool_list[rx_queue_id]);
                        pkt_gen.setMbuf(mbuf);
                        pkt_gen.setDstAddrEth((uint8_t *) backend_node_mac_list[i]);
                        pkt_gen.setDstAddrIp((uint8_t *) backend_node_ip_list[i]);

                        op_payload->op_type = OP_GET_HOT_COUNTER_FPGA;

                        pkt_gen.generate_pkt(send_payload, send_payload_len);
                        enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                        if(debug_flag) std::cout << "send OP_GET_HOT_COUNTER_FPGA [" << i << "]" << std::endl;


                        rcv_count = rcv_pkt_user(
                                lcore_id, rcv_payload_list, rcv_payload_len_list,
                                1, 0, OP_GET_HOT_COUNTER_FPGA,
                                1, TOPK / HOTOBJ_PER_PKT,
                                1, 1000);

                        rcv_payload_bias = 0;
                        for (uint32_t j = 0; j < rcv_count; j++) {
                            rcv_payload_len = rcv_payload_len_list[j];
                            rte_memcpy(rcv_payload, rcv_payload_list + rcv_payload_bias, rcv_payload_len);
                            rcv_payload_bias += rcv_payload_len;

                            for (uint32_t k = 0; k < ntohl(ho_payload->obj_num); k++) {
                                ho_info.obj = ntohl(ho_payload->obj[k]);
                                ho_info.hot_count = ntohl(ho_payload->counter[k]);
                                ho_info.backend_node_index = ntohl(ho_payload->backend_node_index);

                                ho_info_heap[heap_bias] = ho_info;
                                heap_bias++;
                            }
                        }

                        restart_times++;
                        if(restart_times == RESTART_TIMES){
                            uint32_t quit_flag = 0;
                            std::cout << "error: waiting OP_GET_HOT_COUNTER_FPGA > " << RESTART_TIMES << "times" << std::endl;
                            std::cout << "input 0 to wait, input others to quit:\n";
                            std::cin >> quit_flag;
                            if(quit_flag == 0){
                                restart_times = 0;
                            }
                            else{
                                exit(1);
                            }
                        }

                        if(debug_flag) std::cout << "receive OP_GET_HOT_COUNTER_FPGA [" << i << "]" << std::endl;
                    }while(rcv_count < TOPK / HOTOBJ_PER_PKT);

//                    do{
//                        mbuf = rte_pktmbuf_alloc(pktmbuf_pool_list[rx_queue_id]);
//                        op_payload->op_type = OP_GET_END_FPGA;
//                        pkt_gen.generate_pkt(send_payload, send_payload_len);
//                        enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
//                        if(debug_flag) std::cout << "send OP_GET_END_FPGA [" << i << "]" << std::endl;
//
//                        rcv_count = rcv_pkt_user(
//                                lcore_id, rcv_payload_list, rcv_payload_len_list,
//                                1, 0, OP_GET_END_FPGA,
//                                1, 1,
//                                1, 1000);
//                    }while(rcv_count < 1);
                }
                t_temp.finish();
                if(debug_flag) std::cout << "time interval = "
                                         << t_temp.get_interval_microsecond()
                                         << "us" << std::endl;

                t_temp.start();
                uint32_t report_ho_num = heap_bias;
                ho_num = (report_ho_num < TOPK) ? report_ho_num : TOPK;
                uo_num = 0;
                std::sort(
                        ho_info_heap,
                        ho_info_heap + ho_num,
                        [](const hot_obj_info &a, const hot_obj_info &b){
                            return a.hot_count > b.hot_count;
                        });
                for(int i = 0; i < ho_num; i++){
                    if(ho_info_heap[i].hot_count > co_info_heap[TOPK-1-i].cold_count){
                        uo_num++;
                    }
                }
                t_temp.finish();
                if(debug_flag){
                    std::cout << "Top cold objects (cold_obj_index, cold_count):" << std::endl;
                    for(int i = 0; i < TOPK; i++){
                        std::cout << "(" << co_info_heap[i].obj_index << ", " << co_info_heap[i].cold_count << ")" << "\t";
                    }
                    std::cout << std::endl;
                    std::cout << "Top hot objects (hot_obj, hot_count):" << std::endl;
                    for(int i = 0; i < ho_num; i++){
                        std::cout << "(" << ho_info_heap[i].obj << ", " << ho_info_heap[i].hot_count << ")" << "\t";
                    }
                    std::cout << std::endl;
                    std::cout << "replacement policy ((cold) cold_obj_index, (hot) hot_obj):" << std::endl;
                    std::cout << "time interval = "
                              << t_temp.get_interval_microsecond()
                              << "us" << std::endl;
                }

                if(uo_num > 0){
                    for(int i = 0; i < NUM_BACKENDNODE; i++){
                        uo_info_num_pernode_list[i] = 0;
                    }
                    for(int i = 0; i < uo_num; i++){
                        uint32_t bn_index = ho_info_heap[i].backend_node_index;
                        uo_info_pernode_list[bn_index][uo_info_num_pernode_list[bn_index]].hot_obj = ho_info_heap[i].obj;
                        uo_info_num_pernode_list[bn_index]++;
                    }
                    ctrl_stage = STATS_UPDATE;

                    for(int i = 0; i < NUM_BACKENDNODE; i++){
                        do{
                            mbuf = rte_pktmbuf_alloc(pktmbuf_pool_list[rx_queue_id]);
                            op_payload->op_type = OP_GET_END_2_UPDATE_FPGA;
                            pkt_gen.generate_pkt(send_payload, send_payload_len);
                            enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                            if(debug_flag) std::cout << "send OP_GET_END_2_UPDATE_FPGA [" << i << "]" << std::endl;

                            rcv_count = rcv_pkt_user(
                                    lcore_id, rcv_payload_list, rcv_payload_len_list,
                                    1, 0, OP_GET_END_2_UPDATE_FPGA,
                                    1, 1,
                                    1, 1000);
                        }while(rcv_count < 1);
                    }
                }
                else{
                    ctrl_stage = STATS_CLEAN;

                    for(int i = 0; i < NUM_BACKENDNODE; i++){
                        do{
                            mbuf = rte_pktmbuf_alloc(pktmbuf_pool_list[rx_queue_id]);
                            op_payload->op_type = OP_GET_END_2_CLEAN_FPGA;
                            pkt_gen.generate_pkt(send_payload, send_payload_len);
                            enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                            if(debug_flag) std::cout << "send OP_GET_END_2_CLEAN_FPGA [" << i << "]" << std::endl;

                            rcv_count = rcv_pkt_user(
                                    lcore_id, rcv_payload_list, rcv_payload_len_list,
                                    1, 0, OP_GET_END_2_CLEAN_FPGA,
                                    1, 1,
                                    1, 1000);
                        }while(rcv_count < 1);
                    }
                }
                break;
            }
            case STATS_UPDATE: {
                if(debug_flag) std::cout << "STATS_UPDATE starts" << std::endl;

                auto ccos_payload = (change_cold_obj_state_payload *)send_payload;
                do {
                    for (uint32_t i = 0; i < uo_num; i++) {
                        mbuf = rte_pktmbuf_alloc(pktmbuf_pool_list[rx_queue_id]);
                        pkt_gen.setMbuf(mbuf);
                        pkt_gen.setDstAddrEth((uint8_t *) SWITCH_MAC);
                        pkt_gen.setDstAddrIp((uint8_t *) SWITCH_IP);

                        ccos_payload->op_type = OP_MOD_STATE_UPD_SWITCH;
                        ccos_payload->obj_index = htonl(uo_info_list[i].cold_obj_index);

                        pkt_gen.generate_pkt(rcv_payload, send_payload_len);
                        enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                        if (debug_flag) std::cout << "send OP_MOD_STATE_UPD_SWITCH [" << uo_info_list[i].cold_obj_index << "]" << std::endl;
                    }

                    rcv_count = rcv_pkt_user(
                            lcore_id, rcv_payload_list, rcv_payload_len_list,
                            1, 0, OP_MOD_STATE_UPD_SWITCH,
                            1, uo_num,
                            1, 1000);
                    if (debug_flag) std::cout << "receive OP_MOD_STATE_UPD_SWITCH" << std::endl;
                } while (rcv_count < uo_num);


                auto chos_payload = (change_hot_obj_state_payload *)send_payload;
                send_payload_len = sizeof(change_hot_obj_state_payload);
                uint32_t heap_bias = 0;
                for(int i = 0; i < NUM_BACKENDNODE; i++){
                    do{
                        for(int j = 0; j < (uo_num - 1) / HOTOBJ_PER_PKT + 1; j++){
                            mbuf = rte_pktmbuf_alloc(pktmbuf_pool_list[rx_queue_id]);
                            pkt_gen.setMbuf(mbuf);
                            pkt_gen.setDstAddrEth((uint8_t *) backend_node_mac_list[i]);
                            pkt_gen.setDstAddrIp((uint8_t *) backend_node_ip_list[i]);

                            chos_payload->op_type = OP_MOD_STATE_UPD_FPGA;
                            chos_payload->obj_num = htonl(((j * HOTOBJ_PER_PKT) < uo_num) ?
                                                          HOTOBJ_PER_PKT : (uo_num - (j * HOTOBJ_PER_PKT)));
                            chos_payload->pkt_num = (uo_num - 1) / HOTOBJ_PER_PKT + 1;
                            chos_payload->pkt_index = j;

                            // need to complete hot info and cold info
                            for(int k = 0; k < HOTOBJ_PER_PKT; k++){
                                uint32_t index = j * HOTOBJ_PER_PKT + k;
                                if(index < uo_num){
                                    chos_payload->hot_obj[index] = htonl(uo_info_pernode_list[i][index].hot_obj);
                                    chos_payload->cold_obj_index[index] = htonl(uo_info_pernode_list[i][index].cold_obj_index);

                                    auto reverse_iter = obj_index_2_obj_mapping.find(uo_info_pernode_list[i][index].cold_obj_index);
                                    if (reverse_iter != obj_index_2_obj_mapping.cend()) {
                                        chos_payload->cold_obj[index] = reverse_iter->second;
                                    }
                                    obj_index_2_obj_mapping[uo_info_pernode_list[i][index].cold_obj_index] = uo_info_pernode_list[i][index].hot_obj;
                                }
                                else{
                                    chos_payload->hot_obj[index] = INVALID_4B;
                                    chos_payload->cold_obj_index[index] = INVALID_4B;
                                    chos_payload->cold_obj[index] = INVALID_4B;
                                }
                            }
                            pkt_gen.generate_pkt(send_payload, send_payload_len);
                            enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                            if(debug_flag) std::cout << "send OP_MOD_STATE_UPD_FPGA [" << i << "]" << std::endl;
                        }
//                        for(int j = 0; j < (uo_info_num_pernode_list[i] - 1) / HOTOBJ_PER_PKT + 1; j++){
//                            mbuf = rte_pktmbuf_alloc(pktmbuf_pool_list[rx_queue_id]);
//                            pkt_gen.setMbuf(mbuf);
//                            pkt_gen.setDstAddrEth((uint8_t *) backend_node_mac_list[i]);
//                            pkt_gen.setDstAddrIp((uint8_t *) backend_node_ip_list[i]);
//
//                            chos_payload->op_type = OP_MOD_STATE_UPD_FPGA;
//                            chos_payload->obj_num = htonl(((j * HOTOBJ_PER_PKT) < uo_info_num_pernode_list[i]) ?
//                                                          HOTOBJ_PER_PKT : (uo_info_num_pernode_list[i] - (j * HOTOBJ_PER_PKT)));
//                            chos_payload->pkt_num = (uo_info_num_pernode_list[i] - 1) / HOTOBJ_PER_PKT + 1;
//                            chos_payload->pkt_index = j;
//
//                            // need to complete hot info and cold info
//                            for(int k = 0; k < HOTOBJ_PER_PKT; k++){
//                                uint32_t index = j * HOTOBJ_PER_PKT + k;
//                                if(index < uo_info_num_pernode_list[i]){
//                                    chos_payload->hot_obj[index] = htonl(uo_info_pernode_list[i][index].hot_obj);
//                                    chos_payload->cold_obj_index[index] = htonl(uo_info_pernode_list[i][index].cold_obj_index);
//
//                                    auto reverse_iter = obj_index_2_obj_mapping.find(uo_info_pernode_list[i][index].cold_obj_index);
//                                    if (reverse_iter != obj_index_2_obj_mapping.cend()) {
//                                        chos_payload->cold_obj[index] = reverse_iter->second;
//                                    }
//                                    obj_index_2_obj_mapping[uo_info_pernode_list[i][index].cold_obj_index] = uo_info_pernode_list[i][index].hot_obj;
//                                }
//                                else{
//                                    chos_payload->hot_obj[index] = INVALID_4B;
//                                    chos_payload->cold_obj_index[index] = INVALID_4B;
//                                    chos_payload->cold_obj[index] = INVALID_4B;
//                                }
//                            }
//                            pkt_gen.generate_pkt(send_payload, send_payload_len);
//                            enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
//                            if(debug_flag) std::cout << "send OP_MOD_STATE_UPD_FPGA [" << i << "]" << std::endl;
//                        }
                        rcv_count = rcv_pkt_user(
                                lcore_id, rcv_payload_list, rcv_payload_len_list,
                                1, 0, OP_MOD_STATE_UPD_FPGA,
                                1, 1,
                                1, 1000);
                    }while(rcv_count < 1);
                    if(debug_flag) std::cout << "receive OP_MOD_STATE_UPD_FPGA [" << i << "]" << std::endl;
                }
                t_temp.finish();
                if(debug_flag) std::cout << "time interval = "
                                         << t_temp.get_interval_microsecond()
                                         << "us" << std::endl;

                // switch control plane specific operation
                if(debug_flag) std::cout << "Modify hit-check MAT starts" << std::endl;
                t_temp.start();
                scp.modify_hitcheck_MAT(uo_info_list, uo_num);
                t_temp.finish();
                if(debug_flag) std::cout << "time interval = "
                                         << t_temp.get_interval_microsecond()
                                         << "us" << std::endl;


                t_temp.start();
                auto op_payload = (only_op_payload *)send_payload;
                for(int i = 0; i < NUM_BACKENDNODE; i++){
                    do{
                        mbuf = rte_pktmbuf_alloc(pktmbuf_pool_list[rx_queue_id]);
                        pkt_gen.setMbuf(mbuf);
                        pkt_gen.setDstAddrEth((uint8_t *) backend_node_mac_list[i]);
                        pkt_gen.setDstAddrIp((uint8_t *) backend_node_ip_list[i]);

                        op_payload->op_type = OP_MOD_STATE_NOUPD_FPGA;

                        pkt_gen.generate_pkt(send_payload, send_payload_len);
                        enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                        if(debug_flag) std::cout << "send OP_MOD_STATE_NOUPD_FPGA [" << i << "]" << std::endl;

                        rcv_count = rcv_pkt_user(
                                lcore_id, rcv_payload_list, rcv_payload_len_list,
                                1, 0, OP_MOD_STATE_NOUPD_FPGA,
                                1, 1,
                                1, 100);
                        if(debug_flag) std::cout << "receive OP_MOD_STATE_NOUPD_FPGA [" << i << "]" << std::endl;
                    }while(rcv_count < 1);
                }
                if(debug_flag) std::cout << "time interval = "
                                         << t_temp.get_interval_microsecond()
                                         << "us" << std::endl;

                t_temp.start();
//                auto op_payload = (only_op_payload *)send_payload;
                send_payload_len = sizeof(only_op_payload);
                do{
                    mbuf = rte_pktmbuf_alloc(pktmbuf_pool_list[rx_queue_id]);
                    pkt_gen.setMbuf(mbuf);

                    op_payload->op_type = OP_MOD_STATE_NOUPD_SWITCH;

                    pkt_gen.generate_pkt(send_payload, send_payload_len);
                    enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                    if(debug_flag) std::cout << "send OP_MOD_STATE_NOUPD_SWITCH" << std::endl;

                    rcv_count = rcv_pkt_user(
                            lcore_id, rcv_payload_list, rcv_payload_len_list,
                            1, 0, OP_MOD_STATE_NOUPD_SWITCH,
                            1, 1,
                            1, 100);
                    if(debug_flag) std::cout << "receive OP_MOD_STATE_NOUPD_SWITCH" << std::endl;
                }while(rcv_count < 1);
                t_temp.finish();
                if(debug_flag) std::cout << "time interval = "
                                         << t_temp.get_interval_microsecond()
                                         << "us" << std::endl;

                break;
            }
            case STATS_CLEAN: {
                t_temp.start();
                uint32_t loop = 2;
                uint32_t batch = para.cache_size / COLDCOUNT_PER_PKT / loop;

                pkt_gen.setDstAddrEth((uint8_t *)SWITCH_MAC);
                pkt_gen.setDstAddrIp((uint8_t *)SWITCH_IP);
                auto op_payload = (only_op_payload *)send_payload;
                send_payload_len = sizeof(fetch_co_payload);

                for(uint32_t batch_count = 0; batch_count < loop; batch_count++) {
                    do {
                        for (uint32_t i = 0; i < batch; i++) {
                            mbuf = rte_pktmbuf_alloc(pktmbuf_pool_list[rx_queue_id]);
                            pkt_gen.setMbuf(mbuf);

                            op_payload->op_type = OP_CLN_COLD_COUNTER_SWITCH;

                            pkt_gen.generate_pkt(rcv_payload, send_payload_len);
                            enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                            if (debug_flag) std::cout << "send OP_CLN_COLD_COUNTER_SWITCH [" << batch_count << "]" << std::endl;
                        }

                        rcv_count = rcv_pkt_user(
                                lcore_id, rcv_payload_list, rcv_payload_len_list,
                                1, 0, OP_CLN_COLD_COUNTER_SWITCH,
                                1, batch,
                                1, 1000);
                    } while (rcv_count < batch);
                }
                t_temp.finish();
                if(debug_flag) std::cout << "time interval = "
                                         << t_temp.get_interval_microsecond()
                                         << "us" << std::endl;
                if(debug_flag) std::cout << "throuhgput = "
                                         << para.cache_size / COLDCOUNT_PER_PKT / t_temp.get_interval_microsecond()
                                         << "us" << std::endl;

                t_temp.start();
                for(int i = 0; i < NUM_BACKENDNODE; i++){
                    do{
                        mbuf = rte_pktmbuf_alloc(pktmbuf_pool_list[rx_queue_id]);
                        pkt_gen.setMbuf(mbuf);
                        pkt_gen.setDstAddrEth((uint8_t *) backend_node_mac_list[i]);
                        pkt_gen.setDstAddrIp((uint8_t *) backend_node_ip_list[i]);

                        op_payload->op_type = OP_CLN_HOT_COUNTER_FPGA;

                        pkt_gen.generate_pkt(send_payload, send_payload_len);
                        enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                        if(debug_flag) std::cout << "send OP_CLN_HOT_COUNTER_FPGA [" << i << "]" << std::endl;

                        rcv_count = rcv_pkt_user(
                                lcore_id, rcv_payload_list, rcv_payload_len_list,
                                1, 0, OP_CLN_HOT_COUNTER_FPGA,
                                1, 1,
                                1, 100);
                        if(debug_flag) std::cout << "receive OP_CLN_HOT_COUNTER_FPGA [" << i << "]" << std::endl;
                    }while(rcv_count < 1);
                }
                if(debug_flag) std::cout << "time interval = "
                                         << t_temp.get_interval_microsecond()
                                         << "us" << std::endl;

                t_temp.start();
                do{
                    mbuf = rte_pktmbuf_alloc(pktmbuf_pool_list[rx_queue_id]);
                    pkt_gen.setMbuf(mbuf);

                    op_payload->op_type = OP_UNLOCK_STAT_SWITCH;

                    pkt_gen.generate_pkt(send_payload, send_payload_len);
                    enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                    if(debug_flag) std::cout << "send OP_UNLOCK_STAT_SWITCH" << std::endl;

                    rcv_count = rcv_pkt_user(
                            lcore_id, rcv_payload_list, rcv_payload_len_list,
                            1, 0, OP_UNLOCK_STAT_SWITCH,
                            1, 1,
                            1, 100);
                    if(debug_flag) std::cout << "receive OP_UNLOCK_STAT_SWITCH" << std::endl;
                }while(rcv_count < 1);
                t_temp.finish();
                if(debug_flag) std::cout << "time interval = "
                                         << t_temp.get_interval_microsecond()
                                         << "us" << std::endl;

                t_temp.start();
                for(int i = 0; i < NUM_BACKENDNODE; i++){
                    do{
                        mbuf = rte_pktmbuf_alloc(pktmbuf_pool_list[rx_queue_id]);
                        pkt_gen.setMbuf(mbuf);
                        pkt_gen.setDstAddrEth((uint8_t *) backend_node_mac_list[i]);
                        pkt_gen.setDstAddrIp((uint8_t *) backend_node_ip_list[i]);

                        op_payload->op_type = OP_UNLOCK_STAT_FPGA;

                        pkt_gen.generate_pkt(send_payload, send_payload_len);
                        enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                        if(debug_flag) std::cout << "send OP_UNLOCK_STAT_FPGA [" << i << "]" << std::endl;

                        rcv_count = rcv_pkt_user(
                                lcore_id, rcv_payload_list, rcv_payload_len_list,
                                1, 0, OP_UNLOCK_STAT_FPGA,
                                1, 1,
                                1, 100);
                        if(debug_flag) std::cout << "receive OP_UNLOCK_STAT_FPGA [" << i << "]" << std::endl;
                    }while(rcv_count < 1);
                }
                if(debug_flag) std::cout << "time interval = "
                                         << t_temp.get_interval_microsecond()
                                         << "us" << std::endl;


                ctrl_stage = STATS_COLLECT;
                t_wait.start();
                break;
            }
        }
    }
}

static int32_t user_loop(void *arg){
    uint32_t lcore_id = rte_lcore_id();
    uint32_t rx_queue_id = lcore_id_2_rx_queue_id_list[lcore_id];
    if (rx_queue_id == 0) {
        main_ctrl(lcore_id, rx_queue_id);
    }
    return 0;
}


int main(int argc, char **argv){
    int ret = nc_init(argc, argv);
    argc -= ret;
    argv += ret;

    uint32_t lcore_id;
    rte_eal_mp_remote_launch(user_loop, nullptr, CALL_MAIN);
    RTE_LCORE_FOREACH_SLAVE(lcore_id) {
        if (rte_eal_wait_lcore(lcore_id) < 0) {
            break;
        }
    }
    return 0;
}
