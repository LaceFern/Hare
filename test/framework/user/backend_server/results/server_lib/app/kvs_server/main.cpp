#include <iostream>
#include <unordered_map>

#include "dpdk_basic.h"
#include "dpdk_udp.h"
#include "general_ctrl.h"
#include "dpdk_ctrl.h"

#include "stats_server.h"
#include "general_server.h"

#include "kvs_app.h"

void move_appdata_fromSDPtoBS(upd_obj_info uo_info_list[]){
    /* USER_REGION */
}
void move_appdata_fromBStoSDP(upd_obj_info uo_info_list[]){
    /* USER_REGION */
}


int32_t user_loop(void *arg) {
    if (rte_lcore_id() == 0) {
        server_stats_loop();
    }
    else {
        /*** APP region begins ***/
        uint32_t lcore_id = rte_lcore_id();
        uint32_t rx_queue_id = lcore_id_2_rx_queue_id_list[lcore_id];

        rte_mbuf *mbuf;

        uint8_t rcv_pkt_list[65536];
        uint32_t rcv_pkt_len_list[1024];
        uint8_t rcv_pkt[1024];
        uint32_t rcv_pkt_len;
        uint32_t rcv_pkt_bias;

        uint8_t send_payload[1024];
        uint32_t send_payload_len;

        PacketGenerator pkt_gen(
                lcore_id, nullptr,
                (uint8_t *) CTRL_MAC, (uint8_t *) SERVER_MAC,
                (uint8_t *) CTRL_IP, (uint8_t *) SERVER_IP,
                RCV_UDP_PORT_BIAS, RCV_UDP_PORT_BIAS);

        while(1){
            rcv_pkt_bias = 0;
            uint32_t rcv_count = rcv_complete_pkt_user(
                    lcore_id, rcv_pkt_list, rcv_pkt_len_list,
                    0, 0, 0,
                    1, 512,
                    1, 10);

            tput_stat[lcore_id].rx += rcv_count;

            for(uint32_t i = 0; i < rcv_count; i++) {
                /** USER_RECEIVE region begins **/
                rcv_pkt_len = rcv_pkt_len_list[i];
                rte_memcpy(rcv_pkt, rcv_pkt_list + rcv_pkt_bias, rcv_pkt_len);
                rcv_pkt_bias += rcv_pkt_len;
                /** USER_RECEIVE region ends **/

                /** USER_PROCESS region begins **/
                auto eth = (rte_ether_hdr *)rcv_pkt;
                auto ip = (rte_ipv4_hdr *)((uint8_t *)eth + sizeof(rte_ether_hdr));
                auto udp = (rte_udp_hdr *)((uint8_t *)ip + sizeof(rte_ipv4_hdr));
                auto rcv_kr_payload = (kvs_request_payload *) ((uint8_t *)udp + sizeof(rte_udp_hdr));

                uint_key key = rcv_kr_payload->key_byte_list[KEY_BYTE_NUM - 1] |
                        rcv_kr_payload->key_byte_list[KEY_BYTE_NUM - 2] << 8 |
                        rcv_kr_payload->key_byte_list[KEY_BYTE_NUM - 3] << 16 |
                        rcv_kr_payload->key_byte_list[KEY_BYTE_NUM - 4] << 24;

//                if(debug_flag){
//                    std::cout << "lcore id = " << lcore_id << "; obj = " << key << "; rcv_pkt_len - UDP_HEADER_SIZE = " << rcv_pkt_len - UDP_HEADER_SIZE << std::endl;
//                    for(int k = 0; k < rcv_pkt_len - UDP_HEADER_SIZE; k++){
//                        std::cout << (int)(rcv_pkt + UDP_HEADER_SIZE)[k] << "\t";
//                    }
//                    std::cout << std::endl;
//                    for(int k = 0; k < KEY_BYTE_NUM; k++){
//                        std::cout << (int)rcv_kr_payload->key_byte_list[k] << "\t";
//                    }
//                    std::cout << std::endl;
//                }

                if(rcv_kr_payload->op_type == OP_GET){
                    uint32_t conflict_flag = 0;
                    if(upd_obj_flag == 1){
                        for(uint32_t j = 0; j < TOPK; j++){
                            if(uo_info_list[j].hot_obj == key){
                                conflict_flag = 1;
                            }
                        }
                    }

                    mbuf = rte_pktmbuf_alloc(pktmbuf_pool_list[rx_queue_id]);
                    pkt_gen.setMbuf(mbuf);
                    pkt_gen.setSrcAddrEth_direct(eth->d_addr.addr_bytes);
                    pkt_gen.setDstAddrEth_direct(eth->s_addr.addr_bytes);

                    pkt_gen.setSrcAddrIp_direct(ip->dst_addr);
                    pkt_gen.setDstAddrIp_direct(ip->src_addr);

                    uint16_t src_port_udp = ntohs(udp->src_port);
                    uint16_t dst_port_udp = ntohs(udp->dst_port);

                    pkt_gen.setSrcPortUdp(dst_port_udp);
                    pkt_gen.setDstPortUdp(src_port_udp);


                    auto send_kr_payload = (kvs_response_payload *)send_payload;
                    send_payload_len = sizeof(kvs_response_payload);
                    if(conflict_flag == 1){
                        send_kr_payload->op_type = OP_GETFAIL;
                        pkt_gen.generate_pkt(send_payload, send_payload_len);
                        enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);

                        tput_stat[lcore_id].tx ++;
                    }
                    else{
                        send_kr_payload->op_type = OP_GETSUCC;
                        for(uint32_t j = 0; j < KEY_BYTE_NUM; j++){
                            send_kr_payload->key_byte_list[j] = rcv_kr_payload->key_byte_list[j];
                        }
                        for(uint32_t j = 0; j < KEY_BYTE_NUM; j++){
                            send_kr_payload->value_byte_list[j] = rcv_kr_payload->key_byte_list[j];
                        }
                        pkt_gen.generate_pkt(send_payload, send_payload_len);
                        enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);

                        tput_stat[lcore_id].tx ++;
                    }

                }
                /** USER_PROCESS region ends **/

                /** UPDATE_STATS region begins **/
                uint_obj obj = key;
                stats_update(obj, rx_queue_id - 1);
                /** UPDATE_STATS region ends **/

            }
        }
        /*** APP region ends ***/
    }
    return 0;
}


int main(int argc, char **argv){
    int ret = nc_init_udpflow(argc, argv);
    argc -= ret;
    argv += ret;

    /** USER_INIT region begins **/
    /*** Initialize application-related global variables ***/
    KVS kvs;
    kvs.init_kv_mapping();
    void *arg = &kvs;
    /** USER_INIT region ends **/

    uint32_t lcore_id;
    rte_eal_mp_remote_launch(user_loop, arg, CALL_MAIN);
    RTE_LCORE_FOREACH_SLAVE(lcore_id) {
        if (rte_eal_wait_lcore(lcore_id) < 0) {
            break;
        }
    }
    return 0;
}