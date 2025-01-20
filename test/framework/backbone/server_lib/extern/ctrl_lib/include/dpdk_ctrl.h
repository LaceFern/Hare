//
// Created by Alicia on 2023/9/4.
//

#ifndef CONTROLLER_DPDK_CTRL_H
#define CONTROLLER_DPDK_CTRL_H

#include "dpdk_clib.h"
#include "dpdk_basic.h"
#include "dpdk_udp.h"

class PacketGenerator{
public:
    PacketGenerator() = default;
    PacketGenerator(
            uint32_t lcore_id,
            rte_mbuf *mbuf,
            uint8_t *dst_addr_eth,
            uint8_t *src_addr_eth,
            uint8_t *dst_addr_ip,
            uint8_t *src_addr_ip,
            uint16_t dst_port_udp,
            uint16_t src_port_udp
    );
    void generate_pkt(uint8_t *payload, uint32_t payload_len);
    void hexstr2mac(uint8_t *dst, uint8_t *src);
    uint8_t hex2num(uint8_t c);

public:
    void setLcoreId(uint32_t lcoreId);

    void setMbuf(rte_mbuf *mbuf);

    void setSrcAddrEth(uint8_t *srcAddrEth);

    void setDstAddrEth(uint8_t *dstAddrEth);

    void setSrcAddrIp(uint8_t *srcAddrIp);

    void setDstAddrIp(uint8_t *dstAddrIp);

    void setSrcAddrEth_direct(uint8_t *srcAddrEth);

    void setDstAddrEth_direct(uint8_t *dstAddrEth);

    void setSrcAddrIp_direct(rte_be32_t srcAddrIp);

    void setDstAddrIp_direct(rte_be32_t dstAddrIp);

    void setSrcPortUdp(uint16_t srcPortUdp);

    void setDstPortUdp(uint16_t dstPortUdp);

private:
    uint32_t lcore_id = 0;
    rte_mbuf* mbuf = nullptr;
    uint8_t *src_addr_eth = nullptr;
    uint8_t *dst_addr_eth = nullptr;
    uint8_t *src_addr_ip = nullptr;
    uint8_t *dst_addr_ip = nullptr;
    uint16_t src_port_udp = RCV_UDP_PORT_BIAS;
    uint16_t dst_port_udp = RCV_UDP_PORT_BIAS;

    uint8_t src_addr_eth_direct[RTE_ETHER_ADDR_LEN] = {0};
    uint8_t dst_addr_eth_direct[RTE_ETHER_ADDR_LEN] = {0};
    rte_be32_t src_addr_ip_direct = 0;
    rte_be32_t dst_addr_ip_direct = 0;

    uint8_t src_addr_eth_direct_flag = 0;
    uint8_t dst_addr_eth_direct_flag = 0;
    uint8_t src_addr_ip_direct_flag = 0;
    uint8_t dst_addr_ip_direct_flag = 0;
};

#endif //CONTROLLER_DPDK_CTRL_H
