#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <errno.h> 
#include <sys/queue.h>
#include <time.h> 
#include <assert.h>
#include <arpa/inet.h> 
#include <getopt.h>
#include <stdbool.h> 
 
 
#include <rte_memory.h> 
#include <rte_memzone.h>
#include <rte_launch.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_lcore.h>
#include <rte_debug.h>
#include <rte_cycles.h>
#include <rte_mbuf.h>
#include <rte_ether.h>     
#include <rte_ip.h> 
#include <rte_udp.h> 
#include <rte_ethdev.h>

#include <unistd.h> 

#include "./include/util.h"
#include "./include/cache_upd_ctrl.h"

//---------------------------------------modify here---------------------------------------
//-------------------------------------------------------


static void main_ctrl(uint32_t lcore_id, uint16_t rx_queue_id, struct_ctrlPara ctrlPara){

    uint8_t header_template_local[HEADER_TEMPLATE_SIZE_CTRL];
    forCtrlNode_init_header_template_local_ctrl(header_template_local);

    // initiate lconf
    struct lcore_configuration *lconf = &lcore_conf[lcore_id];
    // memory related points
    struct rte_mbuf *mbuf; 

    struct rte_mbuf *mbuf_burst[NC_MAX_BURST_SIZE];
    struct rte_mbuf *mbuf_rcv;

    uint64_t cur_tsc = rte_rdtsc();
    uint64_t update_tsc = cur_tsc + rte_get_tsc_hz() / 1000 * ctrlPara.waitTime;
    uint64_t end_tsc = cur_tsc + rte_get_tsc_hz() * END_S;
    uint32_t pkt_count = 0;

    uint32_t analyze_count = 0;
    uint32_t update_count = 0;
    uint32_t clean_count = 0;
    uint64_t analyze_time_total = 0;   
    uint64_t update_time_total = 0;   
    uint64_t clean_time_total = 0;       
    uint64_t start_time = 0;
    uint64_t end_time = 0;
    uint64_t start_time_tmp = 0;
    uint64_t end_time_tmp = 0;
    uint64_t switch_start_time_tmp;
    uint64_t switch_end_time_tmp;
    uint64_t fpga_start_time_tmp;
    uint64_t fpga_end_time_tmp;

    float tsc_per_ms = rte_get_tsc_hz() / 1000;
    topk_hot_obj_multiBN=(SqList*)malloc(sizeof(SqList));
    // // printf("828:check point 0\n");

    while(1){
        cur_tsc = rte_rdtsc();
        if(unlikely(cur_tsc > end_tsc)){
            if(ctrlNode_stats_stage == STATS_COLLECT){
                runEnd_flag = 1;
                printf("average_analyze_time = %f ms\n", 1.0 * analyze_time_total / analyze_count / (rte_get_tsc_hz() / 1000));
                printf("average_update_time = %f ms\n", 1.0 * update_time_total / update_count / (rte_get_tsc_hz() / 1000));
                printf("average_clean_time = %f ms\n", 1.0 * clean_time_total / clean_count / (rte_get_tsc_hz() / 1000));
                break;
            } 
        }

        // // printf("828:check point 1\n"); 

        switch(ctrlNode_stats_stage){
            case STATS_COLLECT: { 
                if(cur_tsc > update_tsc){
                    // printf("828:STATS_COLLECT starts\n");
                    mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]); 
                    forCtrlNode_generate_ctrl_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, ether_type_switch, OP_LOCK_STAT_SWITCH, 0, NULL, mac_zero);
                    enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                    // printf("828:send OP_LOCK_STAT_SWITCH\n");
                    // enqueue_pkt_with_thres(lcore_id, mbuf, 1, 1);

                    for(int i = 0; i < NUM_BACKENDNODE; i++){
                        char* ip_dst_bn = ip_backendNode_arr[i];
                        struct rte_ether_addr mac_dst_bn = mac_backendNode_arr[i];
                        mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]);
                        forCtrlNode_generate_ctrl_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, ether_type_fpga, OP_LOCK_STAT_FPGA, 0, NULL, mac_dst_bn); 
                        enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0); // enqueue_pkt_with_thres(lcore_id, mbuf, 32, 0);
                    }
                    // printf("828:send OP_LOCK_STAT_FPGA\n");

                    enqueue_pkt_with_thres(lcore_id, mbuf, 1, 1);
                    // printf("send hot-in flag!\n");

                    int comingFlag_switch = 0;
                    int comingCount_fpga = 0;
                    // // printf("828:check point 1.6\n");

                    // printf("828:rcv loop starts\n");
                    while(comingFlag_switch == 0 || comingCount_fpga != NUM_BACKENDNODE){
                        for (int i = 0; i < lconf->n_rx_queue; i++) {
                            uint32_t nb_rx = rte_eth_rx_burst(lconf->port, lconf->rx_queue_list[i], mbuf_burst, NC_MAX_BURST_SIZE);
                            for (int j = 0; j < nb_rx; j++) {
                                mbuf_rcv = mbuf_burst[j];
                                rte_prefetch0(rte_pktmbuf_mtod(mbuf_rcv, void *));     
                                struct rte_ether_hdr* eth = rte_pktmbuf_mtod(mbuf_rcv, struct rte_ether_hdr *);
                                // printf("829: stage = %d, ether_type = %d\n", ctrlNode_stats_stage, ntohs(eth->ether_type));
                                if(ntohs(eth->ether_type) == ether_type_switch){
                                    CtrlHeader_switch* message_header = (CtrlHeader_switch*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE_CTRL);
                                    if(message_header->opType == OP_LOCK_STAT_SWITCH && comingFlag_switch == 0){
                                        comingFlag_switch = 1;
                                        // printf("828:OP_LOCK_STAT_SWITCH ok!\n");
                                        // printf("829: comingFlag_switch = %d\n", comingFlag_switch);
                                    }
                                }
                                else if(ntohs(eth->ether_type) == ether_type_fpga){
                                    CtrlHeader_fpga* message_header = (CtrlHeader_fpga*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE_CTRL);
                                    if(message_header->opType == OP_LOCK_STAT_FPGA && comingCount_fpga != NUM_BACKENDNODE){
                                        comingCount_fpga++;
                                        // printf("828:OP_LOCK_STAT_FPGA ok!\n");
                                        // printf("829: comingFlag_fpga = %d\n", comingFlag_fpga);
                                    }
                                }
                                rte_pktmbuf_free(mbuf_rcv);
                            }
                        }
                    }
                    // printf("828:rcv loop ends\n");

                    ctrlNode_stats_stage = STATS_ANALYZE;
                    // find_topk_hot_ready_flag = 0;
                    for(int i = 0; i < (n_lcores - 1) / 2; i++){
                        find_topk_cold_ready_flag[i] = 1;
                    } 
                    p4_clean_flag = 1;
                    epoch_idx = 0;

                    // // printf("828:check point 2\n");
                    // printf("828:STATS_COLLECT ends\n");
                }
                break;}
                
            case STATS_ANALYZE: {
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                // printf("828:STATS_ANALYZE starts\n");
                // usleep(1000000);
                start_time = rte_rdtsc();
                int topk_cold_obj_num = 0;

                // printf("828:send-rcv loop starts (OP_GET_COLD_COUNTER_SWITCH)\n");
                // uint64_t start_time_1 = rte_rdtsc();
                pkt_count = 0;
                int tmp = ctrlPara.cacheSetSize / FETCHM;
                int startFlag = 0;

                int readyFlag = 0;
                int pktNumPartial = ctrlPara.cacheSize / FETCHM / ((n_lcores - 1) / 2);
                
                for(int i = 0; i < (n_lcores - 1) / 2; i++){
                    // maxheap_downAdjustCount[i] = 0;
                    pktSendCount[i] = 0;
                    pktRecvCount[i] = 0;
                    for(int j = 0; j < pktNumPartial; j++){
                        slidingWin[i][j] = -2;
                    }
                    for(int j = 0; j < TOPK+1; j++){
                        struct_coldobj cold_obj;
                        cold_obj.obj_idx = 0xffffffff;
                        cold_obj.cold_count = 0xffffffff;
                        topk_cold_obj_heap_tmp[i][j] = cold_obj;
                    } 
                }
                for(int i = 0; i < (n_lcores - 1) / 2; i++){
                    find_topk_cold_ready_flag[i] = 0;
                } 
                // uint64_t end_time_1 = rte_rdtsc();
                // uint64_t start_time_2 = rte_rdtsc();
                while(readyFlag == 0){
                    readyFlag = 1;
                    for(int i = 0; i < (n_lcores - 1) / 2; i++){
                        if(find_topk_cold_ready_flag[i] == 0){
                            readyFlag = 0;
                        }
                    }

                    // if(find_topk_cold_ready_flag[1] == 1 && find_topk_cold_ready_flag[2] == 1 && find_topk_cold_ready_flag[3] == 1){
                    //     printf("pktSendCount[%d]=%d, pktRecvCount[%d]=%d\n", 0, pktSendCount[0], 0, pktRecvCount[0]);
                    // }
                    // for (int i = 0; i < lconf->n_rx_queue; i++) {
                    //     uint32_t nb_rx = rte_eth_rx_burst(lconf->port, lconf->rx_queue_list[i], mbuf_burst, NC_MAX_BURST_SIZE);
                    //     for (int j = 0; j < nb_rx; j++) {
                    //         mbuf_rcv = mbuf_burst[j];
                    //         rte_prefetch0(rte_pktmbuf_mtod(mbuf_rcv, void *));     
                    //         struct rte_ether_hdr* eth = rte_pktmbuf_mtod(mbuf_rcv, struct rte_ether_hdr *);
                    //         printf("829: rcv: ether_src = %d, ether_dst  = %d, ether_type = %d\n", eth->s_addr, eth->d_addr, ntohs(eth->ether_type));
                    //         rte_pktmbuf_free(mbuf_rcv);
                    //     }   
                    // }
                }
                // uint64_t end_time_2 = rte_rdtsc();
                // float tsc_per_ms = rte_get_tsc_hz() / 1000;
                // printf("time_p1 = %f ms, tmie_p2 = %f ms\n", 1.0 * (end_time_1 - start_time_1) / tsc_per_ms, 1.0 * (end_time_2 - start_time_2) / tsc_per_ms);

                // end_time = rte_rdtsc(); 
                // int heapCount = 0;
                // for(int i = 0; i < (n_lcores - 1) / 2; i++){
                //     heapCount += maxheap_downAdjustCount[i];
                // }
                // printf("average_analyze_time = %f ms, average_analyze_time_per_obj = %f ms, maxheap_downAdjustCount = %d\n", 1.0 * (end_time - start_time) / (rte_get_tsc_hz() / 1000), 1.0 * (end_time - start_time) / (rte_get_tsc_hz() / 1000) / TOPK, heapCount);

                // for(int i = 0; i < TOPK; i++){
                //     struct_coldobj cold_obj;
                //     cold_obj.obj_idx = 0xffffffff;
                //     cold_obj.cold_count = 0xffffffff;
                //     topk_cold_obj_heap[i] = cold_obj;
                // }

                // while(pkt_count < tmp){
                //     if(startFlag == 0){
                //         find_topk_cold_ready_flag = 0;
                //         startFlag = 1;
                //     }
                //     for (int i = 0; i < lconf->n_rx_queue; i++) {
                //         uint32_t nb_rx = rte_eth_rx_burst(lconf->port, lconf->rx_queue_list[i], mbuf_burst, NC_MAX_BURST_SIZE);
                //         for (int j = 0; j < nb_rx; j++) {
                //             mbuf_rcv = mbuf_burst[j];
                //             rte_prefetch0(rte_pktmbuf_mtod(mbuf_rcv, void *));     
                //             struct rte_ether_hdr* eth = rte_pktmbuf_mtod(mbuf_rcv, struct rte_ether_hdr *);
                //             // printf("829: stage = %d, ether_type = %d\n", ctrlNode_stats_stage, ntohs(eth->ether_type));
                //             if(ntohs(eth->ether_type) == ether_type_switch){
                //                 CtrlHeader_switch* message_header = (CtrlHeader_switch*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE_CTRL);
                //                 if(message_header->opType == OP_GET_COLD_COUNTER_SWITCH && message_header->objIdx == pkt_count){
                //                     // comingFlag_switch = 1; 
                //                     // printf("829: comingFlag_switch = %d\n", comingFlag_switch);
 
                //                     // // violate sorting !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                //                     // for(int k = 0; k < TOPK; k++){ 
                //                     //     struct_coldobj cold_obj;  
                //                     //     cold_obj.obj_idx = ntohl(message_header->objIdx) * TOPK + k;//epoch_idx * cache_set_size + i * TOPK + k;
                //                     //     cold_obj.cold_count = ntohl(message_header->objCounter[k]);
                //                     //     if(topk_cold_obj_num < TOPK){
                //                     //         topk_cold_obj[topk_cold_obj_num] = cold_obj;
                //                     //         topk_cold_obj_num++;
                //                     //     }
                //                     //     else{
                //                     //         uint32_t max_idx = 0;
                //                     //         uint32_t max_cold_count = topk_cold_obj[max_idx].cold_count;
                //                     //         for(int m = 0; m < TOPK; m++){
                //                     //             if(topk_cold_obj[m].cold_count > max_cold_count){
                //                     //                 max_idx = m;
                //                     //                 max_cold_count = topk_cold_obj[m].cold_count;
                //                     //             }
                //                     //         }
                //                     //         if(max_cold_count > cold_obj.cold_count){
                //                     //             topk_cold_obj[max_idx] = cold_obj;
                //                     //         }   
                //                     //     }
                //                     // }

                //                     // min heap !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                //                     for(int k = 0; k < TOPK; k++){
                //                         struct_coldobj cold_obj;
                //                         cold_obj.obj_idx = ntohl(message_header->objIdx) * TOPK + k;//epoch_idx * cache_set_size + i * TOPK + k;
                //                         cold_obj.cold_count = ntohl(message_header->objCounter[k]);
                //                         // printf("cold_obj.obj_idx = %u", cold_obj.obj_idx);
                //                         if(cold_obj.cold_count < topk_cold_obj_heap[1].cold_count){
                //                             topk_cold_obj_heap[1] = cold_obj;
                //                             maxheap_downAdjust(topk_cold_obj_heap, 1, TOPK);
                //                         } 
                //                     }
                //                     pkt_count++;
                //                 }
                //             }
                //             rte_pktmbuf_free(mbuf_rcv);
                //         }   
                //     }
                // }

                // min heap !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                for(int i = 0; i < TOPK+1; i++){
                    struct_coldobj cold_obj;
                    cold_obj.obj_idx = 0xffffffff;
                    cold_obj.cold_count = 0xffffffff;
                    topk_cold_obj_heap[i] = cold_obj;
                }
                int findColdLoopNum = (n_lcores - 1) / 2;
                for(int i = 0; i < findColdLoopNum; i++){
                    for(int k = 0; k < TOPK; k++){
                        struct_coldobj cold_obj = topk_cold_obj_heap_tmp[i][k+1];
                        if(cold_obj.cold_count < topk_cold_obj_heap[1].cold_count){
                            topk_cold_obj_heap[1] = cold_obj;
                            maxheap_downAdjust(topk_cold_obj_heap, 1, TOPK);
                        } 
                    }
                }
                for(int i = 0; i < TOPK; i++){
                    topk_cold_obj[i] = topk_cold_obj_heap[i + 1];
                }
                
                // sort -- decreasing
                for(int i = 0; i < TOPK - 1; i++){
                    for(int j = 0; j < TOPK - 1 - i; j++){
                        if(topk_cold_obj[j].cold_count < topk_cold_obj[j+1].cold_count){
                            struct_coldobj temp = topk_cold_obj[j];
                            topk_cold_obj[j] = topk_cold_obj[j+1];
                            topk_cold_obj[j+1] = temp;
                        }
                    }
                }

                // end_time = rte_rdtsc(); 
                // printf("average_analyze_time = %f ms\n", 1.0 * (end_time - start_time) / (rte_get_tsc_hz() / 1000));
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                // printf("topk_cold_obj (obj_idx, obj_counter):\n");
                // for(int i = 0; i < TOPK; i++){
                //     printf("(%d, %d)\t", topk_cold_obj[i].obj_idx, topk_cold_obj[i].cold_count);
                // }
                // printf("\n");
                // printf("828:OP_GET_COLD_COUNTER_SWITCH ok!\n");

                for(int i = 0; i < NUM_BACKENDNODE; i++){
                    char* ip_dst_bn = ip_backendNode_arr[i];
                    struct rte_ether_addr mac_dst_bn = mac_backendNode_arr[i];
                    mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]);
                    forCtrlNode_generate_ctrl_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, ether_type_fpga, OP_GET_HOT_COUNTER_FPGA, 0, NULL, mac_dst_bn); 
                    enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0); // enqueue_pkt_with_thres(lcore_id, mbuf, 32, 0);
                }
                enqueue_pkt_with_thres(lcore_id, mbuf, 1, 1);
                // printf("828:send OP_GET_HOT_COUNTER_FPGA\n");

                int topk_hot_obj_num = 0;
                int comingCount_fpga = 0;
                topk_hot_obj_multiBN->length = 0;
                
                while(comingCount_fpga != NUM_BACKENDNODE * max_pkts_from_one_bn){
                    for (int i = 0; i < lconf->n_rx_queue; i++) {
                        uint32_t nb_rx = rte_eth_rx_burst(lconf->port, lconf->rx_queue_list[i], mbuf_burst, NC_MAX_BURST_SIZE);
                        for (int j = 0; j < nb_rx; j++) {
                            mbuf_rcv = mbuf_burst[j];
                            rte_prefetch0(rte_pktmbuf_mtod(mbuf_rcv, void *)); 
                            struct rte_ether_hdr* eth = rte_pktmbuf_mtod(mbuf_rcv, struct rte_ether_hdr *);
                            // // printf("828:stage = %d, ether_type = %d\n", ctrlNode_stats_stage, ntohs(eth->ether_type));
                            if(ntohs(eth->ether_type) == ether_type_fpga){
                                CtrlHeader_fpga_big* message_header = (CtrlHeader_fpga_big*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE_CTRL);
                                if(message_header->opType == OP_GET_HOT_COUNTER_FPGA && comingCount_fpga != NUM_BACKENDNODE * max_pkts_from_one_bn){
                                    comingCount_fpga++;
                                    //!!!!!!!!!!!!!!!!!!!!!!!!!!!找到最冷的再替换掉
                                    for(int k = 0; k < NUM_BACKENDNODE; k++){
                                        char* ip_dst_bn = ip_backendNode_arr[k];
                                        struct rte_ether_addr mac_dst_bn = mac_backendNode_arr[k];
                                        if(mac_compare(mac_dst_bn, eth->s_addr)){
                                            
                                            for(int m = 0; m < HOTOBJ_IN_ONEPKT; m++){
                                                SqNote tmp_hotobj;
                                                tmp_hotobj.obj = ntohl(message_header->obj[m]);
                                                tmp_hotobj.count = ntohl(message_header->counter[m]);
                                                tmp_hotobj.backendNode_idx = k;
                                                if(tmp_hotobj.count > 0){
                                                    topk_hot_obj_multiBN->r[1 + topk_hot_obj_multiBN->length] = tmp_hotobj;
                                                    topk_hot_obj_multiBN->length++;
                                                }
                                            }

                                            // printf("k = %d\n");

                                            // for(int m = 0; m < TOPK; m++){
                                            //     SqNote tmp_hotobj;
                                            //     tmp_hotobj.obj = ntohl(message_header->obj[m]);
                                            //     tmp_hotobj.count = ntohl(message_header->counter[m]);
                                            //     tmp_hotobj.backendNode_idx = k;
                                            //     if(tmp_hotobj.count > 0){
                                            //         topk_hot_obj_multiBN->r[1 + topk_hot_obj_multiBN->length] = tmp_hotobj;
                                            //         topk_hot_obj_multiBN->length++;
                                            //     }
                                            // }
                                            break;
                                        }
                                    }
                                }
                                else{
                                    // printf("828:comingCount_fpga = %d\n", comingCount_fpga);
                                    // printf("828:other pkts, ether_type = %u (expect = %u), op = %u (expect = %u)\n", ntohs(eth->ether_type), ether_type_fpga, message_header->opType, OP_GET_HOT_COUNTER_FPGA);
                                }
                            }
                            else{
                                CtrlHeader_fpga_big* message_header = (CtrlHeader_switch*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE_CTRL);
                                // printf("828:other pkts, ether_type = %u (expect = %u), op = %u (expect = %u)\n", ntohs(eth->ether_type), ether_type_fpga, message_header->opType, OP_GET_HOT_COUNTER_FPGA);
                            }
                            rte_pktmbuf_free(mbuf_rcv);
                        }
                    }
                }

                // sort -- increasing
                QuickSort(topk_hot_obj_multiBN);
                topk_hot_obj_num = topk_hot_obj_multiBN->length;
                //-----------------

                // printf("topk_hot_obj_num = %d, topk_hot_obj (obj_idx, obj_counter):\n", topk_hot_obj_num);
                // for(int i = 0; i < topk_hot_obj_num; i++){
                //     printf("(%d, %d)\t", topk_hot_obj_multiBN->r[i+1].obj, topk_hot_obj_multiBN->r[i+1].count);
                // }
                // printf("\n");
                // printf("topk_hot_obj_num = %d, topk_hot_obj (obj_idx, obj_counter):\n", topk_hot_obj_num);
                // for(int i = 0; i < TOPK; i++){
                //     printf("(%d, %d)\t", topk_hot_obj[i].obj, topk_hot_obj[i].hot_count);
                // }
                // printf("\n");
                // printf("828:OP_GET_HOT_COUNTER_FPGA ok!\n");

                // // sort
                // for(int i = 0; i < TOPK - 1; i++){
                //     for(int j = 0; j < TOPK - 1 - i; j++){
                //         if(topk_hot_obj[j].hot_count < topk_hot_obj[j+1].hot_count){
                //             struct_hotobj temp = topk_hot_obj[j];
                //             topk_hot_obj[j] = topk_hot_obj[j+1];
                //             topk_hot_obj[j+1] = temp;
                //         }
                //     }
                // }


                topk_upd_obj_num = 0;
                for(int i = 0; i < NUM_BACKENDNODE; i++){
                    topk_upd_obj_num_multiBN[i] = 0;
                }
                // for(int i = 0; i < topk_hot_obj_num; i++){
                //     if(topk_hot_obj[i].hot_count > topk_cold_obj[TOPK-1-i].cold_count){
                //         topk_upd_obj_num++;
                //     }
                // }
            
                // printf("828:checkpoint 0!\n");

                int bound = 0;
                if(topk_hot_obj_num < TOPK){
                    bound = topk_hot_obj_num;
                }
                else{
                    bound = TOPK;
                }
                for(int i = 0; i < bound; i++){
                    if(topk_hot_obj_multiBN->r[1+(topk_hot_obj_num-1-i)].count > topk_cold_obj[TOPK-1-i].cold_count){
                        topk_upd_obj_num++;
                    }
                }
                // // printf("828:checkpoint 1!\n");

                // for(int i = 0; i < topk_hot_obj_num; i++){
                //     if(topk_hot_obj_multiBN->r[i+1].count > topk_cold_obj[TOPK-1-i].cold_count){
                //         topk_upd_obj_num++;
                //     }
                // }

                if(topk_upd_obj_num > 0){
                    for(int i = 0; i < topk_upd_obj_num; i++){
                        topk_hot_obj_upd[i].obj = topk_hot_obj_multiBN->r[1+(topk_hot_obj_num-1-i)].obj;
                        topk_hot_obj_upd[i].hot_count = topk_hot_obj_multiBN->r[1+(topk_hot_obj_num-1-i)].count;
                        topk_cold_obj_upd[i] = topk_cold_obj[TOPK-1-i]; 
                        topk_hot_obj_upd[i].obj_idx = topk_cold_obj_upd[i].obj_idx;
                        //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                        int tmp_idx = topk_hot_obj_multiBN->r[1+(topk_hot_obj_num-1-i)].backendNode_idx;
                        int tmp_num = topk_upd_obj_num_multiBN[tmp_idx];
                        topk_upd_obj_multiBN[tmp_idx][tmp_num].obj = topk_hot_obj_multiBN->r[1+(topk_hot_obj_num-1-i)].obj;
                        topk_upd_obj_multiBN[tmp_idx][tmp_num].coldIdx = topk_cold_obj_upd[i].obj_idx;
                        topk_upd_obj_multiBN[tmp_idx][tmp_num].coldObj = obj_offloaded[topk_cold_obj_upd[i].obj_idx];
                        topk_upd_obj_num_multiBN[tmp_idx]++;
                    }
                    ctrlNode_stats_stage = STATS_UPDATE;

                    for(int i = 0; i < topk_upd_obj_num; i++){
                        obj_offloaded[topk_hot_obj_upd[i].obj_idx] = topk_hot_obj_upd[i].obj;
                    }
                }
                else{
                    // // printf("828:checkpoint 2!\n");
                    for(int i = 0; i < NUM_BACKENDNODE; i++){
                        char* ip_dst_bn = ip_backendNode_arr[i];
                        struct rte_ether_addr mac_dst_bn = mac_backendNode_arr[i];
                        // // printf("828:checkpoint 2.1!\n");
                        mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]);
                        // // printf("828:checkpoint 2.2!\n");
                        forCtrlNode_generate_ctrl_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, ether_type_fpga, OP_LOCK_HOT_QUEUE_FPGA, topk_upd_obj_num_multiBN[i], topk_upd_obj_multiBN[i], mac_dst_bn);
                        // // printf("828:checkpoint 2.3!\n");
                        enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0); // enqueue_pkt_with_thres(lcore_id, mbuf, 32, 0);
                    }
                    // // printf("828:checkpoint 3!\n");
                    enqueue_pkt_with_thres(lcore_id, mbuf, 1, 1);

                    // printf("828:send OP_LOCK_HOT_QUEUE_FPGA (don't update)\n");

                    // printf("828:rcv loop starts\n");
                    int comingCount_fpga = 0;
                    while(comingCount_fpga != NUM_BACKENDNODE){
                        for (int i = 0; i < lconf->n_rx_queue; i++) {
                            uint32_t nb_rx = rte_eth_rx_burst(lconf->port, lconf->rx_queue_list[i], mbuf_burst, NC_MAX_BURST_SIZE);
                            for (int j = 0; j < nb_rx; j++) {
                                mbuf_rcv = mbuf_burst[j];
                                rte_prefetch0(rte_pktmbuf_mtod(mbuf_rcv, void *)); 
                                struct rte_ether_hdr* eth = rte_pktmbuf_mtod(mbuf_rcv, struct rte_ether_hdr *);
                                if(ntohs(eth->ether_type) == ether_type_fpga){
                                    CtrlHeader_fpga_big* message_header = (CtrlHeader_fpga_big*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE_CTRL);
                                    if(message_header->opType == OP_LOCK_HOT_QUEUE_FPGA && comingCount_fpga != NUM_BACKENDNODE){
                                        comingCount_fpga++;
                                        // printf("828:OP_LOCK_HOT_QUEUE_FPGA ok (don't update)\n");
                                    }
                                }
                                rte_pktmbuf_free(mbuf_rcv);
                            }
                        }
                    }
                    // printf("828:rcv loop ends\n");
                    
                    epoch_idx++;
                    if(epoch_idx < ctrlPara.updEpochs){
                        ctrlNode_stats_stage = STATS_ANALYZE;
                    }
                    else{
                        ctrlNode_stats_stage = STATS_CLEAN;
                    }
                }

                end_time = rte_rdtsc(); 
                analyze_time_total += end_time - start_time;
                analyze_count += 1;

                // printf("topk_upd_obj (obj, obj_idx, obj_counter):\n");
                // for(int i = 0; i < topk_upd_obj_num; i++){
                //     printf("(%d, %d, %d)\t", topk_hot_obj_upd[i].obj, topk_hot_obj_upd[i].obj_idx, topk_hot_obj_upd[i].hot_count);
                // }
                // printf("\n");
                // printf("828:STATS_ANALYZE ends\n");
                break;}

//-------------------------------------------------------------------
            case STATS_UPDATE: {
                // printf("828:STATS_UPDATE starts\n");

                start_time = rte_rdtsc();

                start_time_tmp = rte_rdtsc();//-------------------------------
                // obj the queue on switch and wait for queue empty
                for(int i = 0; i < NUM_BACKENDNODE; i++){
                    char* ip_dst_bn = ip_backendNode_arr[i];
                    struct rte_ether_addr mac_dst_bn = mac_backendNode_arr[i];

                    uint32_t upd_num = 0;
                    if(topk_upd_obj_num_multiBN[i] == 0){
                        upd_num = 0xffffffff;
                        mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]);
                        forCtrlNode_generate_ctrl_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, ether_type_fpga, OP_LOCK_HOT_QUEUE_FPGA, upd_num, topk_upd_obj_multiBN[i], mac_dst_bn);
                        enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0); // enqueue_pkt_with_thres(lcore_id, mbuf, 32, 0);
                    }
                    else{
                        upd_num = topk_upd_obj_num_multiBN[i];
                        uint32_t pkts_to_each_bn = (upd_num - 1) / HOTOBJ_IN_ONEPKT + 1;
                        for(int j = 0; j < pkts_to_each_bn; j++){
                            if(j < pkts_to_each_bn - 1){
                                mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]);
                                forCtrlNode_generate_ctrl_pkt_mul(rx_queue_id, header_template_local, lcore_id, mbuf, ether_type_fpga, OP_LOCK_HOT_QUEUE_FPGA, upd_num, topk_upd_obj_multiBN[i] + HOTOBJ_IN_ONEPKT * j, mac_dst_bn, HOTOBJ_IN_ONEPKT);
                                enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0); // enqueue_pkt_with_thres(lcore_id, mbuf, 32, 0);
                                rte_sleep_us(SLEEP_US_BIGPKT);
                            }
                            else{
                                mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]);
                                forCtrlNode_generate_ctrl_pkt_mul(rx_queue_id, header_template_local, lcore_id, mbuf, ether_type_fpga, OP_LOCK_HOT_QUEUE_FPGA, upd_num, topk_upd_obj_multiBN[i] + HOTOBJ_IN_ONEPKT * j, mac_dst_bn, upd_num - HOTOBJ_IN_ONEPKT * j);
                                enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0); // enqueue_pkt_with_thres(lcore_id, mbuf, 32, 0);
                                rte_sleep_us(SLEEP_US_BIGPKT);
                            }
                            
                        }
                    }

                    // uint32_t upd_num = (topk_upd_obj_num_multiBN[i] == 0) ? 0xffffffff : topk_upd_obj_num_multiBN[i];
                    // forCtrlNode_generate_ctrl_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, ether_type_fpga, OP_LOCK_HOT_QUEUE_FPGA, upd_num, topk_upd_obj_multiBN[i], mac_dst_bn);
                    // forCtrlNode_generate_ctrl_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, ether_type_fpga, OP_LOCK_HOT_QUEUE_FPGA, topk_upd_obj_num_multiBN[i], topk_upd_obj_multiBN[i], mac_dst_bn);
                }
                enqueue_pkt_with_thres(lcore_id, mbuf, 1, 1);
                // printf("828:send OP_LOCK_HOT_QUEUE_FPGA\n");

                for (int i = 0; i < topk_upd_obj_num; i++) {
                    mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]); 
                    forCtrlNode_generate_ctrl_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, ether_type_switch, OP_LOCK_COLD_QUEUE_SWITCH, topk_cold_obj_upd[i].obj_idx, NULL, mac_zero);
                    enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0); // enqueue_pkt_with_thres(lcore_id, mbuf, 32, 0); 
                }
                enqueue_pkt_with_thres(lcore_id, mbuf, 1, 1);
                // printf("828:send OP_LOCK_COLD_QUEUE_SWITCH\n");
                end_time_tmp = rte_rdtsc();//-------------------------------
                // printf("211:OP_LOCK_HOT_QUEUE time = %f ms\n", 1.0 * (end_time_tmp - start_time_tmp) / tsc_per_ms);//-------------------------------

                start_time_tmp = rte_rdtsc();//-------------------------------
                // printf("828:rcv loop starts\n");
                int fpga_coming_count = 0;
                int fpga_empty_count = 0;
                int p4_empty_flag = 0;
                int p4_empty_count = 0;
                // while((fpga_coming_count + fpga_empty_count < NUM_BACKENDNODE + NUM_BACKENDNODE) || p4_empty_flag == 0){
                while(fpga_coming_count < NUM_BACKENDNODE || fpga_empty_count < NUM_BACKENDNODE || p4_empty_flag == 0){
                    for (int i = 0; i < lconf->n_rx_queue; i++) {
                        uint32_t nb_rx = rte_eth_rx_burst(lconf->port, lconf->rx_queue_list[i], mbuf_burst, NC_MAX_BURST_SIZE);
                        for (int j = 0; j < nb_rx; j++) {
                            mbuf_rcv = mbuf_burst[j];
                            rte_prefetch0(rte_pktmbuf_mtod(mbuf_rcv, void *)); 
                            struct rte_ether_hdr* eth = rte_pktmbuf_mtod(mbuf_rcv, struct rte_ether_hdr *);
                            if(ntohs(eth->ether_type) == ether_type_switch){
                                CtrlHeader_switch* message_header = (CtrlHeader_switch*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE_CTRL);
                                if(message_header->opType == OP_LOCK_COLD_QUEUE_SWITCH || message_header->opType == OP_CALLBACK){
                                    p4_empty_count++;
                                    // printf("828:obj idx = %d empty!\n", ntohl(message_header->objIdx));
                                    if(p4_empty_count == topk_upd_obj_num){
                                        p4_empty_flag = 1;
                                        // printf("828:OP_EMPTY_SWITCH ok\n");
                                    }
                                }
                                else{
                                    // printf("828:other pkts, ether_type = %u, opType = %u\n", ntohs(eth->ether_type), message_header->opType);
                                }
                            }
                            else if(ntohs(eth->ether_type) == ether_type_fpga){
                                CtrlHeader_fpga* message_header = (CtrlHeader_fpga*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE_CTRL);
                                if(message_header->opType == OP_QUEUE_EMPTY_FPGA){
                                    fpga_empty_count++;
                                    // printf("828:OP_EMPTY_FPGA: stage = %d, stage_count = %d, node = %d ok\n", message_header->replaceNum, message_header->obj[0], message_header->obj[1]);
                                }
                                else if(message_header->opType == OP_LOCK_HOT_QUEUE_FPGA){
                                    fpga_coming_count++;
                                    // printf("828:OP_LOCK_HOT_QUEUE_FPGA: stage = %d, stage_count = %d, node = %d ok\n", message_header->replaceNum, message_header->obj[0], message_header->obj[1]);
                                    // // printf("828:OP_LOCK_HOT_QUEUE_FPGA ok\n");
                                }
                                else{
                                    // printf("828:other pkts, ether_type = %u\n", ntohs(eth->ether_type));
                                }
                            }
                            else{
                                // printf("828:other pkts, ether_type = %u\n", ntohs(eth->ether_type));
                            }
                            rte_pktmbuf_free(mbuf_rcv);
                        }
                    }
                }
                // printf("828:rcv loop ends\n");
                end_time_tmp = rte_rdtsc();//-------------------------------
                // printf("211:QUEUE EMPTY time = %f ms\n", 1.0 * (end_time_tmp - start_time_tmp) / tsc_per_ms);//-------------------------------

                start_time_tmp = rte_rdtsc();//-------------------------------
                //!!!!!!!!!!!!!!!!!!!!OP_HOT_REPORT_FPGA
                for(int i = 0; i < NUM_BACKENDNODE; i++){
                    char* ip_dst_bn = ip_backendNode_arr[i];
                    struct rte_ether_addr mac_dst_bn = mac_backendNode_arr[i];
                    mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]);
                    forCtrlNode_generate_ctrl_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, ether_type_fpga, OP_HOT_REPORT_FPGA, 0, NULL, mac_dst_bn);
                    enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0); // enqueue_pkt_with_thres(lcore_id, mbuf, 32, 0);
                }                
                enqueue_pkt_with_thres(lcore_id, mbuf, 1, 1);


                uint32_t pkts_to_cp = (topk_upd_obj_num - 1) / HOTOBJ_IN_ONEPKT + 1;
                for(int i = 0; i < pkts_to_cp; i++){
                    if(i < pkts_to_cp - 1){
                        mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]); 
                        forCtrlNode_generate_ctrl_pkt_mul(rx_queue_id, header_template_local, lcore_id, mbuf, ether_type_switch, OP_HOT_REPORT_SWITCH, topk_upd_obj_num, topk_hot_obj_upd + HOTOBJ_IN_ONEPKT * i, mac_zero, HOTOBJ_IN_ONEPKT);
                        enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0); // enqueue_pkt_with_thres(lcore_id, mbuf, 32, 0);
                        rte_sleep_us(SLEEP_US_BIGPKT);
                    }
                    else{
                        mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]); 
                        forCtrlNode_generate_ctrl_pkt_mul(rx_queue_id, header_template_local, lcore_id, mbuf, ether_type_switch, OP_HOT_REPORT_SWITCH, topk_upd_obj_num, topk_hot_obj_upd + HOTOBJ_IN_ONEPKT * i, mac_zero, topk_upd_obj_num - HOTOBJ_IN_ONEPKT * i);
                        enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0); // enqueue_pkt_with_thres(lcore_id, mbuf, 32, 0);
                        rte_sleep_us(SLEEP_US_BIGPKT);
                    }
                }
                enqueue_pkt_with_thres(lcore_id, mbuf, 1, 1);
                // mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]); 
                // forCtrlNode_generate_ctrl_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, ether_type_switch, OP_HOT_REPORT_SWITCH, topk_upd_obj_num, topk_hot_obj_upd, mac_zero);
                // enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                // printf("828:send OP_HOT_REPORT_SWITCH\n");

                // printf("828:rcv loop starts\n");
                int comingFlag_switch = 0;
                int comingCount_fpga = 0;
                
                switch_start_time_tmp = rte_rdtsc();//-------------------------------
                fpga_start_time_tmp = rte_rdtsc();//-------------------------------
                // pkt_count = 2;
                while(comingFlag_switch == 0 || comingCount_fpga != NUM_BACKENDNODE){
                    for (int i = 0; i < lconf->n_rx_queue; i++) {
                        uint32_t nb_rx = rte_eth_rx_burst(lconf->port, lconf->rx_queue_list[i], mbuf_burst, NC_MAX_BURST_SIZE);
                        for (int j = 0; j < nb_rx; j++) {
                            mbuf_rcv = mbuf_burst[j];
                            rte_prefetch0(rte_pktmbuf_mtod(mbuf_rcv, void *)); 
                            struct rte_ether_hdr* eth = rte_pktmbuf_mtod(mbuf_rcv, struct rte_ether_hdr *);
                            if(ntohs(eth->ether_type) == ether_type_switch){
                                CtrlHeader_switch* message_header = (CtrlHeader_switch*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE_CTRL);
                                if(message_header->opType == OP_REPLACE_SUCCESS && comingFlag_switch == 0){
                                    comingFlag_switch = 1;
                                    switch_end_time_tmp = rte_rdtsc();//-------------------------------
                                    // printf("211:OP_HOT_REPORT time (switch) = %f ms\n", 1.0 * (switch_end_time_tmp - switch_start_time_tmp) / tsc_per_ms);//-------------------------------
                                    // printf("828:OP_REPLACE_SUCCESS_SWITCH ok\n");
                                }
                                else{
                                    // printf("828:other pkts, ether_type = %u (expect = %u), op = %u (expect = %u)\n", ntohs(eth->ether_type), ether_type_switch, message_header->opType, OP_REPLACE_SUCCESS);
                                }
                            }
                            else if(ntohs(eth->ether_type) == ether_type_fpga){
                                CtrlHeader_fpga* message_header = (CtrlHeader_fpga*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE_CTRL);
                                if(message_header->opType == OP_HOT_REPORT_END_FPGA && comingCount_fpga != NUM_BACKENDNODE){
                                    comingCount_fpga++;
                                    // printf("828:OP_HOT_REPORT_END_FPGA ok\n");
                                    if(comingCount_fpga == NUM_BACKENDNODE){
                                        fpga_end_time_tmp = rte_rdtsc();//-------------------------------
                                        // printf("211:OP_HOT_REPORT time (fpga) = %f ms\n", 1.0 * (fpga_end_time_tmp - fpga_start_time_tmp) / tsc_per_ms);//-------------------------------
                                    }
                                }
                                else{
                                    // printf("828:other pkts, ether_type = %u (expect = %u), op = %u (expect = %u)\n", ntohs(eth->ether_type), ether_type_fpga, message_header->opType, OP_REPLACE_SUCCESS);
                                }
                            }
                            else{
                                CtrlHeader_switch* message_header = (CtrlHeader_switch*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE_CTRL);
                                // printf("828:other pkts, ether_type = %u (expect = %u), op = %u (expect = %u)\n", ntohs(eth->ether_type), ether_type_switch, message_header->opType, OP_REPLACE_SUCCESS);
                            }
                            rte_pktmbuf_free(mbuf_rcv);
                        }
                    }
                }
                // printf("828:rcv loop ends\n");
                end_time_tmp = rte_rdtsc();//-------------------------------
                // printf("211:OP_HOT_REPORT time = %f ms\n", 1.0 * (end_time_tmp - start_time_tmp) / tsc_per_ms);//-------------------------------

                start_time_tmp = rte_rdtsc();//-------------------------------
                for(int i = 0; i < NUM_BACKENDNODE; i++){
                    char* ip_dst_bn = ip_backendNode_arr[i];
                    struct rte_ether_addr mac_dst_bn = mac_backendNode_arr[i];
                    mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]);
                    forCtrlNode_generate_ctrl_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, ether_type_fpga, OP_REPLACE_SUCCESS, topk_upd_obj_num_multiBN[i], topk_upd_obj_multiBN[i], mac_dst_bn);
                    enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0); // enqueue_pkt_with_thres(lcore_id, mbuf, 32, 0);
                }
                enqueue_pkt_with_thres(lcore_id, mbuf, 1, 1);
                // printf("828:send OP_REPLACE_SUCCESS_FPGA\n");

                // printf("828:rcv loop starts\n");
                comingCount_fpga = 0;
                while(comingCount_fpga != NUM_BACKENDNODE){
                    for (int i = 0; i < lconf->n_rx_queue; i++) {
                        uint32_t nb_rx = rte_eth_rx_burst(lconf->port, lconf->rx_queue_list[i], mbuf_burst, NC_MAX_BURST_SIZE);
                        for (int j = 0; j < nb_rx; j++) {
                            mbuf_rcv = mbuf_burst[j];
                            rte_prefetch0(rte_pktmbuf_mtod(mbuf_rcv, void *)); 
                            struct rte_ether_hdr* eth = rte_pktmbuf_mtod(mbuf_rcv, struct rte_ether_hdr *);
                            if(ntohs(eth->ether_type) == ether_type_fpga){
                                CtrlHeader_fpga* message_header = (CtrlHeader_fpga*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE_CTRL);
                                if(message_header->opType == OP_REPLACE_SUCCESS && comingCount_fpga != NUM_BACKENDNODE){
                                    comingCount_fpga++;
                                    // printf("828:OP_REPLACE_SUCCESS_FPGA ok\n");
                                }
                                else{
                                    // printf("828:other pkts, ether_type = %u (expect = %u), op = %u (expect = %u)\n", ntohs(eth->ether_type), ether_type_fpga, message_header->opType, OP_REPLACE_SUCCESS);
                                }
                            }
                            else{
                                CtrlHeader_switch* message_header = (CtrlHeader_switch*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE_CTRL);
                                // printf("828:other pkts, ether_type = %u (expect = %u), op = %u (expect = %u)\n", ntohs(eth->ether_type), ether_type_fpga, message_header->opType, OP_REPLACE_SUCCESS);
                            }
                            rte_pktmbuf_free(mbuf_rcv);
                        }
                    }
                }
                // printf("828:rcv loop ends\n");
                end_time_tmp = rte_rdtsc();//-------------------------------
                // printf("211:OP_REPLACE_SUCCESS time = %f ms\n", 1.0 * (end_time_tmp - start_time_tmp) / tsc_per_ms);//-------------------------------

                epoch_idx++;
                if(epoch_idx < ctrlPara.updEpochs){
                    ctrlNode_stats_stage = STATS_ANALYZE;
                }
                else{
                    ctrlNode_stats_stage = STATS_CLEAN;
                }

                end_time = rte_rdtsc();
                update_time_total += end_time - start_time;
                update_count += 1;
                // printf("828:STATS_UPDATE ends\n");
                break;}

            case STATS_CLEAN: {
                // printf("828:STATS_CLEAN starts\n");
                start_time = rte_rdtsc();

                start_time_tmp = rte_rdtsc();//-------------------------------
                for(int i = 0; i < NUM_BACKENDNODE; i++){
                    char* ip_dst_bn = ip_backendNode_arr[i];
                    struct rte_ether_addr mac_dst_bn = mac_backendNode_arr[i];
                    mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]);
                    forCtrlNode_generate_ctrl_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, ether_type_fpga, OP_CLN_HOT_COUNTER_FPGA, 0, NULL, mac_dst_bn); 
                    enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0); // enqueue_pkt_with_thres(lcore_id, mbuf, 32, 0);
                }                
                enqueue_pkt_with_thres(lcore_id, mbuf, 1, 1);

                // printf("828:send OP_CLN_HOT_COUNTER_FPGA\n");

                // printf("828:rcv loop starts\n");
                int fpga_clean_count = 0;
                while(fpga_clean_count != NUM_BACKENDNODE){
                    for (int i = 0; i < lconf->n_rx_queue; i++) {
                        uint32_t nb_rx = rte_eth_rx_burst(lconf->port, lconf->rx_queue_list[i], mbuf_burst, NC_MAX_BURST_SIZE);
                        for (int j = 0; j < nb_rx; j++) {
                            mbuf_rcv = mbuf_burst[j];
                            rte_prefetch0(rte_pktmbuf_mtod(mbuf_rcv, void *)); 
                            struct rte_ether_hdr* eth = rte_pktmbuf_mtod(mbuf_rcv, struct rte_ether_hdr *);
                            if(ntohs(eth->ether_type) == ether_type_fpga){
                                CtrlHeader_fpga* message_header = (CtrlHeader_fpga*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE_CTRL);
                                if(message_header->opType == OP_CLN_HOT_COUNTER_FPGA && fpga_clean_count != NUM_BACKENDNODE){
                                    fpga_clean_count++;
                                    // printf("828:OP_CLN_HOT_COUNTER_FPGA ok\n");
                                }
                            }
                            rte_pktmbuf_free(mbuf_rcv);
                        }
                    }
                }
                // printf("828:rcv loop ends\n");
                end_time_tmp = rte_rdtsc();//-------------------------------
                // printf("210:OP_CLN_HOT_COUNTER_FPGA time = %f ms\n", 1.0 * (end_time_tmp - start_time_tmp) / tsc_per_ms);//-------------------------------
                
                start_time_tmp = rte_rdtsc();//-------------------------------
                // printf("828:send-rcv loop starts (OP_CLN_COLD_COUNTER_SWITCH)\n");
                for(int i = 0; i < ctrlPara.cacheSize / FETCHM; i++){
                    mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]);
                    forCtrlNode_generate_ctrl_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, ether_type_switch, OP_CLN_COLD_COUNTER_SWITCH, i, NULL, mac_zero);
                    enqueue_pkt_with_thres(lcore_id, mbuf, 32, 0);
                }
                enqueue_pkt_with_thres(lcore_id, mbuf, 1, 1);
                end_time_tmp = rte_rdtsc();//-------------------------------
                // printf("210:OP_CLN_COLD_COUNTER_SWITCH time = %f ms\n", 1.0 * (end_time_tmp - start_time_tmp) / tsc_per_ms);//-------------------------------


//                 pkt_count = 0;
//                 uint64_t tmp_end = rte_rdtsc() + rte_get_tsc_hz() * 10;
//                 // ret = sem_post(&semTag);
//                 int tmp = ctrlPara.cacheSize / TOPK;
//                 int startFlag = 0;
//                 int firstTimeFlag = 0;

//                 // start_time = rte_rdtsc();

// //                 for(int i = 0; i < 4096; i++){
// //                     check_arr[i] = 0;
// //                 }

//                 while(likely(pkt_count < tmp)){
// //                     if(rte_rdtsc() > tmp_end){
// //                         printf("pkt_count = %d\n", pkt_count);
// //                         for(int i = 0; i < 4096; i++){
// //                             if(check_arr[i] == 0){
// //                                 printf("don't rcv idx = %d\n", i);
// //                             }
// //                         }
// //                         exit(1);
// //                     }

//                     if(unlikely(startFlag == 0)){
//                         p4_clean_flag = 0;
//                         startFlag = 1;
//                     }

//                     for (int i = 0; i < lconf->n_rx_queue; i++) {
//                         uint32_t nb_rx = rte_eth_rx_burst(lconf->port, lconf->rx_queue_list[i], mbuf_burst, NC_MAX_BURST_SIZE);
//                         for (int j = 0; j < nb_rx; j++) {
//                             mbuf_rcv = mbuf_burst[j];
//                             rte_prefetch0(rte_pktmbuf_mtod(mbuf_rcv, void *));     
//                             struct rte_ether_hdr* eth = rte_pktmbuf_mtod(mbuf_rcv, struct rte_ether_hdr *);
//                             // printf("829: stage = %d, ether_type = %d\n", ctrlNode_stats_stage, ntohs(eth->ether_type));
//                             if(ntohs(eth->ether_type) == ether_type_switch){
//                                 CtrlHeader_switch* message_header = (CtrlHeader_switch*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE_CTRL);
//                                 if(likely(message_header->opType == OP_CLN_COLD_COUNTER_SWITCH)){
//                                     pkt_count++;
//                                     // printf("ntohl(message_header->objIdx) = %d", ntohl(message_header->objIdx));
// //                                     check_arr[ntohl(message_header->objIdx)] = 1;
//                                 }
//                                 // else{
//                                 //     // printf("828:other pkts, ether_type = %u (expect = %u), op = %u (expect = %u)\n", ntohs(eth->ether_type), ether_type_switch, message_header->opType, OP_CLN_COLD_COUNTER_SWITCH);
//                                 // } 
//                             }
//                             // else{
//                             //     CtrlHeader_switch* message_header = (CtrlHeader_switch*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE_CTRL);
//                             //     // printf("828:other pkts, ether_type = %u (expect = %u), op = %u (expect = %u)\n", ntohs(eth->ether_type), ether_type_switch, message_header->opType, OP_CLN_COLD_COUNTER_SWITCH);
//                             // }
//                             rte_pktmbuf_free(mbuf_rcv);
//                             firstTimeFlag = 0;
//                         }   

// //                         if(nb_rx == 0 && pkt_count < tmp && pkt_count > 0){
// //                             if(firstTimeFlag == 0){
// //                                 usleep(50);
// //                                 firstTimeFlag = 1;
// //                             }
// //                             else{
// //                                 // printf("828:pkt_count = %d, tmp = %d\n", pkt_count, tmp);
// //                                 tmp = tmp - pkt_count;
// //                                 pkt_count = 0;
// //                                 startFlag = 0;
// //                                 firstTimeFlag = 0;
// //                             }

// //                         }
//                     }
//                 }

                // end_time = rte_rdtsc(); 
                // clean_time_total = end_time - start_time;
                // printf("average_clean_time = %f ms\n", 1.0 * clean_time_total / (rte_get_tsc_hz() / 1000));
  
                // start_time = rte_rdtsc();
                // for(int i = 0; i < tmp; i++){ 
                //     // printf("start\n");
                //     // printf("i = %d, cache_size = %d, i * 8 = %d\n", i, cache_size, i * 8);
                //     mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]); 
                //     forCtrlNode_generate_ctrl_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, ether_type_switch, OP_CLN_COLD_COUNTER_SWITCH, i, NULL, mac_zero);
                //     enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                //     int comingFlag_switch = 0;
                //     while(comingFlag_switch == 0){
                //         for (int ii = 0; ii < lconf->n_rx_queue; ii++) {
                //             uint32_t nb_rx = rte_eth_rx_burst(lconf->port, lconf->rx_queue_list[ii], mbuf_burst, NC_MAX_BURST_SIZE);
                //             for (int j = 0; j < nb_rx; j++) {
                //                 mbuf_rcv = mbuf_burst[j];
                //                 rte_prefetch0(rte_pktmbuf_mtod(mbuf_rcv, void *));     
                //                 struct rte_ether_hdr* eth = rte_pktmbuf_mtod(mbuf_rcv, struct rte_ether_hdr *);
                //                 // printf("im here!!!!!\n");
                //                 if(ntohs(eth->ether_type) == ether_type_switch){
                //                     CtrlHeader_switch* message_header = (CtrlHeader_switch*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE_CTRL);
                //                     if(message_header->opType == OP_CLN_COLD_COUNTER_SWITCH){
                //                         comingFlag_switch = 1;
                //                         // printf("829: comingFlag_switch = %d\n", comingFlag_switch);
                //                         pkt_count++;
                //                         // printf("829: stage = %d, pkt_count = %d\n", ctrlNode_stats_stage, pkt_count);
                //                         // printf("829: clean right_stage = %d, ether_type = %x, message_header->opType = %d, message_header->objIdx = %d\n", ctrlNode_stats_stage, ntohs(eth->ether_type), message_header->opType, ntohl(message_header->objIdx));
                //                     }
                //                     else{
                //                         // printf("828:clean wrong_stage = %d, ether_type = %x, message_header->opType = %d, message_header->objIdx = %d\n", ctrlNode_stats_stage, ntohs(eth->ether_type), message_header->opType, ntohl(message_header->objIdx));
                //                     }
                //                 }
                //                 else{
                //                     // printf("828:clean wrong_stage = %d, ether_type = %x\n", ctrlNode_stats_stage, ntohs(eth->ether_type));
                //                 }
                //                 rte_pktmbuf_free(mbuf_rcv);
                //             }
                //         }
                //     }
                //     // printf("end\n");
                // }
                // end_time = rte_rdtsc();
                // clean_time_total = end_time - start_time;
                // printf("average_clean_time = %f ms\n", 1.0 * clean_time_total / (rte_get_tsc_hz() / 1000));

                // printf("828:send-rcv loop ends\n");
                // printf("828:OP_CLN_COLD_COUNTER_SWITCH ok\n");

                start_time_tmp = rte_rdtsc();//-------------------------------
                // printf("828:send-rcv loop starts (OP_CLN_COLD_COUNTER_SWITCH)\n");
                // unobj stats
                mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]); 
                forCtrlNode_generate_ctrl_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, ether_type_switch, OP_UNLOCK_STAT_SWITCH, 0, NULL, mac_zero);
                enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                // printf("828:send OP_UNLOCK_STAT_SWITCH\n");

                for(int i = 0; i < NUM_BACKENDNODE; i++){
                    char* ip_dst_bn = ip_backendNode_arr[i];
                    struct rte_ether_addr mac_dst_bn = mac_backendNode_arr[i];
                    mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]);
                    forCtrlNode_generate_ctrl_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, ether_type_fpga, OP_UNLOCK_STAT_FPGA, 0, NULL, mac_dst_bn);                    
                    enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0); // enqueue_pkt_with_thres(lcore_id, mbuf, 32, 0);
                }
                enqueue_pkt_with_thres(lcore_id, mbuf, 1, 1);
                // printf("828:send OP_UNLOCK_STAT_FPGA\n");

                // printf("828:rcv loop starts\n");
                int comingFlag_switch = 0;
                int comingCount_fpga = 0;
                while(comingFlag_switch == 0 || comingCount_fpga != NUM_BACKENDNODE){
                    for (int i = 0; i < lconf->n_rx_queue; i++) {
                        uint32_t nb_rx = rte_eth_rx_burst(lconf->port, lconf->rx_queue_list[i], mbuf_burst, NC_MAX_BURST_SIZE);
                        for (int j = 0; j < nb_rx; j++) {
                            mbuf_rcv = mbuf_burst[j];
                            rte_prefetch0(rte_pktmbuf_mtod(mbuf_rcv, void *)); 
                            struct rte_ether_hdr* eth = rte_pktmbuf_mtod(mbuf_rcv, struct rte_ether_hdr *);
                            // printf("829: stage = %d, ntohs(eth->ether_type) = %x\n", ctrlNode_stats_stage, ntohs(eth->ether_type));
                            if(ntohs(eth->ether_type) == ether_type_switch){
                                CtrlHeader_switch* message_header = (CtrlHeader_switch*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE_CTRL);
                                // printf("829: stage = %d, ntohs(eth->ether_type) = %x, message_header->opType = %d\n", ctrlNode_stats_stage, ntohs(eth->ether_type), message_header->opType);
                                
                                if(message_header->opType == OP_UNLOCK_STAT_SWITCH && comingFlag_switch == 0){
                                    comingFlag_switch = 1; 
                                    // printf("828:OP_UNLOCK_STAT_SWITCH ok\n");
                                }
                                else{
                                    // printf("828:other pkts\n");
                                }
                            }
                            else if(ntohs(eth->ether_type) == ether_type_fpga){
                                CtrlHeader_fpga* message_header = (CtrlHeader_fpga*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE_CTRL);
                                // printf("829: stage = %d, ntohs(eth->ether_type) = %x, message_header->opType = %d\n", ctrlNode_stats_stage, ntohs(eth->ether_type), message_header->opType);
                                
                                if(message_header->opType == OP_UNLOCK_STAT_FPGA && comingCount_fpga != NUM_BACKENDNODE){
                                    comingCount_fpga++;
                                    // printf("828:OP_UNLOCK_STAT_FPGA ok\n");
                                }
                                else{
                                    // printf("828:other pkts\n");
                                }
                            }
                            else{
                                // printf("828:other pkts\n");
                            }
                            rte_pktmbuf_free(mbuf_rcv);
                        }
                    }
                }
                // printf("828:rcv loop ends\n");
                end_time_tmp = rte_rdtsc();//-------------------------------
                // printf("210:OP_UNLOCK_STAT time = %f ms\n", 1.0 * (end_time_tmp - start_time_tmp) / tsc_per_ms);//-------------------------------


                ctrlNode_stats_stage = STATS_COLLECT;
                cur_tsc = rte_rdtsc();
                update_tsc = cur_tsc + rte_get_tsc_hz() / 1000 * ctrlPara.waitTime;

                end_time = rte_rdtsc(); 
                clean_time_total += end_time - start_time;
                clean_count += 1;
                // printf("828:STATS_CLEAN ends\n");
                break;}
        }
    }
    free(topk_hot_obj_multiBN);
}


static void send_topk_cold_loop(uint32_t lcore_id, uint16_t rx_queue_id, struct_ctrlPara ctrlPara, int findColdLoopNum, int findColdLoopID){
    struct lcore_configuration *lconf = &lcore_conf[lcore_id];
    struct rte_mbuf *mbuf_burst[NC_MAX_BURST_SIZE];
    struct rte_mbuf *mbuf_rcv;
    uint8_t header_template_local[HEADER_TEMPLATE_SIZE_CTRL];
    forCtrlNode_init_header_template_local_ctrl(header_template_local);
    struct rte_mbuf *mbuf;
    int pktNumPartial = ctrlPara.cacheSize / FETCHM / findColdLoopNum;
    int loopCount = 0;
    int whileCount = 0;
    int signal = 0;
    uint64_t startTime = 0;
    uint64_t endTime = 0;

    printf("send_topk_cold_loop[%d] is running\n", findColdLoopID);

    while(1){
        if(unlikely(runEnd_flag == 1)){
            if(ctrlNode_stats_stage == STATS_COLLECT){
                break; 
            }
        }
        while(ctrlNode_stats_stage == STATS_ANALYZE && find_topk_cold_ready_flag[findColdLoopID] == 0){
            // if(signal == 0){
            //     signal = 1;
            //     startTime = rte_rdtsc();
            // }

            loopCount++;
            // printf("loopCount[%d]=%d\n", loopCount, findColdLoopID);
            if(pktSendCount[findColdLoopID] < pktNumPartial){
                if(pktSendCount[findColdLoopID] - pktRecvCount[findColdLoopID] < WINSIZE){
                    int index = pktSendCount[findColdLoopID] + pktNumPartial * findColdLoopID;
                    slidingWin[findColdLoopID][pktSendCount[findColdLoopID]] = index;
                    mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]); 
                    forCtrlNode_generate_ctrl_pkt_fetchCold(rx_queue_id, header_template_local, lcore_id, mbuf, ether_type_switch, OP_GET_COLD_COUNTER_SWITCH, index, findColdLoopID);
                    enqueue_pkt_with_thres(lcore_id, mbuf, 32, 0);
                    pktSendCount[findColdLoopID]++;
                }
                else{ 
                    int index = pktRecvCount[findColdLoopID] + pktNumPartial * findColdLoopID;
                    mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]); 
                    forCtrlNode_generate_ctrl_pkt_fetchCold(rx_queue_id, header_template_local, lcore_id, mbuf, ether_type_switch, OP_GET_COLD_COUNTER_SWITCH, index, findColdLoopID);
                    enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                    usleep(10);
                }
            }
            else{
                int index = pktRecvCount[findColdLoopID] + pktNumPartial * findColdLoopID;
                mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]); 
                forCtrlNode_generate_ctrl_pkt_fetchCold(rx_queue_id, header_template_local, lcore_id, mbuf, ether_type_switch, OP_GET_COLD_COUNTER_SWITCH, index, findColdLoopID);
                enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                usleep(10);
            }
            if(loopCount == 31){
                loopCount = 0;
                enqueue_pkt_with_thres(lcore_id, mbuf, 1, 1);
                // printf("pktSendCount[%d]=%d, pktRecvCount[%d]=%d\n", findColdLoopID, pktSendCount[findColdLoopID], findColdLoopID, pktRecvCount[findColdLoopID]);
            }
        }

        // if(signal == 1){
        //     signal = 0;
        //     float tsc_per_ms = rte_get_tsc_hz() / 1000;
        //     endTime = rte_rdtsc();
        //     printf("pktNumPartial = %d, time[%d] = %f ms\n", pktNumPartial, findColdLoopID, 1.0 * (endTime - startTime) / tsc_per_ms);
        // }
    }
}

static void recv_topk_cold_loop(uint32_t lcore_id, uint16_t rx_queue_id, struct_ctrlPara ctrlPara, int findColdLoopNum, int findColdLoopID){
    struct lcore_configuration *lconf = &lcore_conf[lcore_id];
    struct rte_mbuf *mbuf_burst[NC_MAX_BURST_SIZE];
    struct rte_mbuf *mbuf_rcv;
    uint8_t header_template_local[HEADER_TEMPLATE_SIZE_CTRL];
    forCtrlNode_init_header_template_local_ctrl(header_template_local);
    struct rte_mbuf *mbuf;
    int pktNumPartial = ctrlPara.cacheSize / FETCHM / findColdLoopNum;
    int recvEnd = 0;

    printf("recv_topk_cold_loop[%d] is running\n", findColdLoopID);

    while(1){
        if(unlikely(runEnd_flag == 1)){
            if(ctrlNode_stats_stage == STATS_COLLECT){
                break;
            } 
        }
        while(ctrlNode_stats_stage == STATS_ANALYZE && find_topk_cold_ready_flag[findColdLoopID] == 0){
            for (int i = 0; i < lconf->n_rx_queue; i++) { 
                uint32_t nb_rx = rte_eth_rx_burst(lconf->port, lconf->rx_queue_list[i], mbuf_burst, NC_MAX_BURST_SIZE);
                for (int j = 0; j < nb_rx; j++) { 
                    mbuf_rcv = mbuf_burst[j]; 
                    rte_prefetch0(rte_pktmbuf_mtod(mbuf_rcv, void *));     
                    struct rte_ether_hdr* eth = rte_pktmbuf_mtod(mbuf_rcv, struct rte_ether_hdr *);
                    if(ntohs(eth->ether_type) == ether_type_switch && find_topk_cold_ready_flag[findColdLoopID] == 0){
                        CtrlHeader_switch* message_header = (CtrlHeader_switch*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE_CTRL);
                        if(message_header->opType == OP_GET_COLD_COUNTER_SWITCH){
                            int index = ntohl(message_header->objIdx);
                            int bias = index - pktNumPartial * findColdLoopID;
                            if(bias == pktRecvCount[findColdLoopID]){
                                pktRecvCount[findColdLoopID]++;
                            }
                            if(slidingWin[findColdLoopID][bias] == index){
                                // min heap
                                slidingWin[findColdLoopID][bias] = -1;
                                for(int k = 0; k < FETCHM; k++){
                                    struct_coldobj cold_obj;
                                    cold_obj.obj_idx = ntohl(message_header->objIdx) * FETCHM + k;//epoch_idx * cache_set_size + i * TOPK + k;
                                    cold_obj.cold_count = ntohl(message_header->counter[k]);//(pktNumPartial - bias) * FETCHM - k;//ntohl(message_header->objCounter[k]);
                                    // printf("cold_obj.obj_idx = %d, cold_obj.cold_count = %d, cold_obj.cold_count_tmp = %d\n", cold_obj.obj_idx, cold_obj.cold_count, ntohl(message_header->counter[k+4]));
                                    if(cold_obj.cold_count < topk_cold_obj_heap_tmp[findColdLoopID][1].cold_count){
                                        topk_cold_obj_heap_tmp[findColdLoopID][1] = cold_obj;
                                        maxheap_downAdjust(topk_cold_obj_heap_tmp[findColdLoopID], 1, TOPK);
                                        // maxheap_downAdjustCount[findColdLoopID]++;
                                    } 
                                }
                                int bbias = 1;
                                // int count = 0;
                                while(slidingWin[findColdLoopID][bias + bbias] == -1){
                                    pktRecvCount[findColdLoopID]++;
                                    bbias++;                                  
                                }
                            }
                            if(pktRecvCount[findColdLoopID] == pktNumPartial){
                                find_topk_cold_ready_flag[findColdLoopID] = 1;
                            }
                        }
                    }  
                    rte_pktmbuf_free(mbuf_rcv);
                }   
            }
        }  
    }
}

//---------------------------------------------------

static int32_t np_client_loop(__attribute__((unused)) void *arg) {
    uint32_t lcore_id = rte_lcore_id();
    int rx_queue_id = lcore_id_2_rx_queue_id[lcore_id];
    if (rx_queue_id == 0) {
        main_ctrl(lcore_id, rx_queue_id, gb_ctrlPara);
    }
    else if(rx_queue_id % 2 == 0){
        send_topk_cold_loop(lcore_id, rx_queue_id, gb_ctrlPara, (n_lcores - 1) / 2, (rx_queue_id - 1) / 2);
    }
    else{ 
        recv_topk_cold_loop(lcore_id, rx_queue_id, gb_ctrlPara, (n_lcores - 1) / 2, (rx_queue_id - 1) / 2);
    }
    return 0;  
}

 
static void nc_parse_args_help(void) {
    printf("simple_socket [EAL options] --\n"
           "  -w wait time for stats\n"
           "  -c cache size\n"
           "  -g 0:cache set, 1:global\n"
           "  -e update epochs for once collection\n");
}

static int nc_parse_args(int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, "w:c:g:e:")) != -1) {
        switch (opt) {
        case 'w':
            gb_ctrlPara.waitTime = atoi(optarg);
            break; 
        case 'c':
            gb_ctrlPara.cacheSize = atoi(optarg);
            break; 
        case 'g':
            gb_ctrlPara.globalFlag = atoi(optarg);
            break;
        case 'e':
            gb_ctrlPara.updEpochs = atoi(optarg);
            break; 
        default:
            nc_parse_args_help();
            return -1;
        }
    }
    return 1;
}


int main(int argc, char **argv) {
    int ret;
    ret = rte_eal_init(argc, argv);
    if (ret < 0) {
        rte_exit(EXIT_FAILURE, "invalid EAL arguments\n");
    }
    argc -= ret;
    argv += ret;
    ret = nc_parse_args(argc, argv);
    if (ret < 0) {
        rte_exit(EXIT_FAILURE, "invalid user arguments\n");
    }
    // init 
    nc_init(); 
    for(int i = 0; i < (n_lcores - 1) / 2; i++){
        find_topk_cold_ready_flag[i] = 1;
    }

    int numa_id_of_port = rte_eth_dev_socket_id(0);
    printf("numa_id_of_port = %d\n", numa_id_of_port);

    check_arr = (uint32_t *)malloc(sizeof(uint32_t) * 4096);
        if(gb_ctrlPara.globalFlag == 0){
            gb_ctrlPara.cacheSetSize = gb_ctrlPara.cacheSize / gb_ctrlPara.updEpochs;
        }
        else{
            gb_ctrlPara.cacheSetSize = gb_ctrlPara.cacheSize;
        }
    // launch main loop in every lcore
    uint32_t lcore_id;
    rte_eal_mp_remote_launch(np_client_loop, NULL, CALL_MASTER);
    RTE_LCORE_FOREACH_SLAVE(lcore_id) {
        if (rte_eal_wait_lcore(lcore_id) < 0) {
            ret = -1;
            break;
        }
    }
    return 0;
}