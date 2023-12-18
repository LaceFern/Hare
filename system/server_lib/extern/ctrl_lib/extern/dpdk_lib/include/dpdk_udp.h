//
// Created by Alicia on 2023/8/28.
//

#ifndef CONTROLLER_V1_DPDK_UDP_H
#define CONTROLLER_V1_DPDK_UDP_H

#include "dpdk_clib.h"

/******************** need config*********************/
#define IP_LOCAL                "10.0.0.3"
#define MAC_LOCAL               {0x98, 0x03, 0x9b, 0xc7, 0xc8, 0x18} //amax 3
#define RCV_UDP_PORT_BIAS 5001//1111
#define UDP_CTRL_PORT 1024

/******************** inner *********************/
#define UDP_HEADER_SIZE 42
#define MAX_PATTERN_NUM		8
#define MAX_ACTION_NUM		8

rte_flow *generate_udp_flow(uint16_t port_phys, uint16_t port_udp, uint16_t rx_q, rte_flow_error *error);
int nc_init_udpflow(int argc, char **argv);
uint32_t rcv_pkt_user(
        uint32_t lcore_id, uint8_t *payload_list, uint32_t *payload_len_list,
        uint32_t payload_verify_flag, uint32_t payload_verify_position, uint8_t payload_verify_value,
        uint32_t rcv_count_flag, uint32_t rcv_count_thres,
        uint32_t rcv_us_flag, uint64_t rcv_us_thres);

uint32_t rcv_complete_pkt_user(
        uint32_t lcore_id, uint8_t *pkt_list, uint32_t *pkt_len_list,
        uint32_t pkt_verify_flag, uint32_t pkt_verify_position, uint8_t pkt_verify_value,
        uint32_t rcv_count_flag, uint32_t rcv_count_thres,
        uint32_t rcv_us_flag, uint64_t rcv_us_thres);

#endif //CONTROLLER_V1_DPDK_UDP_H
