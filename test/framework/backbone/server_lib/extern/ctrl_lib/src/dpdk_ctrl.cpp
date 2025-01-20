//
// Created by Alicia on 2023/9/4.
//

//#include "general_ctrl.h"
#include <iostream>
#include "dpdk_ctrl.h"

PacketGenerator::PacketGenerator(
        uint32_t lcore_id,
        rte_mbuf *mbuf,
        uint8_t *dst_addr_eth,
        uint8_t *src_addr_eth,
        uint8_t *dst_addr_ip,
        uint8_t *src_addr_ip,
        uint16_t dst_port_udp,
        uint16_t src_port_udp
) :
        lcore_id(lcore_id),
        mbuf(mbuf),
        dst_addr_eth(dst_addr_eth),
        src_addr_eth(src_addr_eth),
        dst_addr_ip (dst_addr_ip ),
        src_addr_ip (src_addr_ip ),
        dst_port_udp(dst_port_udp),
        src_port_udp(src_port_udp)
{}



uint8_t PacketGenerator::hex2num(uint8_t c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

void PacketGenerator::hexstr2mac(uint8_t *dst, uint8_t *src) {
    uint32_t i = 0;
    while(i < 6) {
        if(' ' == *src||':'== *src||'"'== *src||'\''== *src) {
            src++;
            continue;
        }
        *(dst+i) = ((hex2num(*src)<<4)|hex2num(*(src+1)));
        i++;
        src += 2;
    }
}

void PacketGenerator::generate_pkt(uint8_t *payload, uint32_t payload_len) {
//    lcore_configuration *lconf = &lcore_conf_list[lcore_id];
    assert(mbuf != nullptr);

    mbuf->next = NULL;
    mbuf->nb_segs = 1;
    mbuf->ol_flags = 0;

    uint32_t pkt_len = UDP_HEADER_SIZE + payload_len;
    mbuf->data_len = pkt_len;
    mbuf->pkt_len = pkt_len;

    // init packet header
    rte_ether_hdr* eth = rte_pktmbuf_mtod(mbuf, rte_ether_hdr *);
    auto *ip = (rte_ipv4_hdr *)((uint8_t*) eth + sizeof(rte_ether_hdr));
    auto *udp = (rte_udp_hdr *)((uint8_t*) ip + sizeof(rte_ipv4_hdr));
    auto *pld = (uint8_t*)eth + UDP_HEADER_SIZE;

//    //???????
//    for(uint32_t i = 0; i < payload_len; i++){
//        pld[i] = payload[payload_len - 1 - i];
//    }
    rte_memcpy(pld, payload, payload_len);

    // eth header
//    std::cout << "src_addr_ip" << src_addr_eth << std::endl;
//    std::cout << "dst_addr_ip" << dst_addr_eth << std::endl;

    eth->ether_type = rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4);//rte_cpu_to_be_16(0x2331);//rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4);
    if(src_addr_eth_direct_flag == 1){
        rte_memcpy(eth->s_addr.addr_bytes, src_addr_eth_direct, RTE_ETHER_ADDR_LEN);
    }
    else{
        hexstr2mac(eth->s_addr.addr_bytes, src_addr_eth);
    }
    if(dst_addr_eth_direct_flag == 1){
        rte_memcpy(eth->d_addr.addr_bytes, dst_addr_eth_direct, RTE_ETHER_ADDR_LEN);
    }
    else{
        hexstr2mac(eth->d_addr.addr_bytes, dst_addr_eth);
    }

    // ip header
    if(src_addr_ip_direct_flag == 1){
        int st1 = inet_pton(AF_INET, (char *)src_addr_ip, &(ip->src_addr));
        if(st1 != 1) {
            fprintf(stderr, "inet_pton() failed.Error message: %s", strerror(st1));
            exit(EXIT_FAILURE);
        }
    }
    else{
        ip->src_addr = src_addr_ip_direct;
    }
    if(dst_addr_ip_direct_flag == 1){
        int st2 = inet_pton(AF_INET, (char *)dst_addr_ip, &(ip->dst_addr));
        if(st2 != 1) {
            fprintf(stderr, "inet_pton() failed.Error message: %s", strerror(st2));
            exit(EXIT_FAILURE);
        }
    }
    else{
        ip->dst_addr = dst_addr_ip_direct;
    }

    ip->total_length = rte_cpu_to_be_16(pkt_len - sizeof(rte_ether_hdr));
    ip->version_ihl = 0x45;
    ip->type_of_service = 0;
    ip->packet_id = 0;
    ip->fragment_offset = 0;
    ip->time_to_live = 64;
    ip->next_proto_id = IPPROTO_UDP;
    uint32_t ip_cksum;
    auto *ptr16 = (uint16_t *)ip;
    ip_cksum = 0;
    ip_cksum += ptr16[0]; ip_cksum += ptr16[1];
    ip_cksum += ptr16[2]; ip_cksum += ptr16[3];
    ip_cksum += ptr16[4];
    ip_cksum += ptr16[6]; ip_cksum += ptr16[7];
    ip_cksum += ptr16[8]; ip_cksum += ptr16[9];
    ip_cksum = ((ip_cksum & 0xffff0000) >> 16) + (ip_cksum & 0x0000ffff);
    if (ip_cksum > 65535) {
        ip_cksum -= 65535;
    }
    ip_cksum = (~ip_cksum) & 0x0000ffff;
    if (ip_cksum == 0) {
        ip_cksum = 0xffff;
    }
    ip->hdr_checksum = (uint16_t)ip_cksum;

    // udp header
    udp->src_port = htons(src_port_udp);
    udp->dst_port = htons(dst_port_udp);
    udp->dgram_len = rte_cpu_to_be_16(pkt_len- sizeof(rte_ether_hdr)- sizeof(rte_ipv4_hdr));
    udp->dgram_cksum = 0;
}

void PacketGenerator::setLcoreId(uint32_t lcoreId) {
    lcore_id = lcoreId;
}

void PacketGenerator::setMbuf(rte_mbuf *mbuf) {
    PacketGenerator::mbuf = mbuf;
}

void PacketGenerator::setSrcAddrEth(uint8_t *srcAddrEth) {
    src_addr_eth = srcAddrEth;
    src_addr_eth_direct_flag = 0;
}

void PacketGenerator::setDstAddrEth(uint8_t *dstAddrEth) {
    dst_addr_eth = dstAddrEth;
    dst_addr_eth_direct_flag = 0;
}

void PacketGenerator::setSrcAddrIp(uint8_t *srcAddrIp) {
    src_addr_ip = srcAddrIp;
    src_addr_ip_direct_flag = 0;
}

void PacketGenerator::setDstAddrIp(uint8_t *dstAddrIp) {
    dst_addr_ip = dstAddrIp;
    dst_addr_ip_direct_flag = 0;
}

void PacketGenerator::setSrcAddrEth_direct(uint8_t *srcAddrEth) {
    rte_memcpy(src_addr_eth_direct, srcAddrEth, RTE_ETHER_ADDR_LEN);
    src_addr_eth_direct_flag = 1;
}

void PacketGenerator::setDstAddrEth_direct(uint8_t *dstAddrEth) {
    rte_memcpy(dst_addr_eth_direct, dstAddrEth, RTE_ETHER_ADDR_LEN);
    dst_addr_eth_direct_flag = 1;
}

void PacketGenerator::setSrcAddrIp_direct(rte_be32_t srcAddrIp) {
    src_addr_ip_direct = srcAddrIp;
    src_addr_ip_direct_flag = 1;
}

void PacketGenerator::setDstAddrIp_direct(rte_be32_t dstAddrIp) {
    dst_addr_ip_direct = dstAddrIp;
    dst_addr_ip_direct_flag = 1;
}

void PacketGenerator::setSrcPortUdp(uint16_t srcPortUdp) {
    src_port_udp = srcPortUdp;
}

void PacketGenerator::setDstPortUdp(uint16_t dstPortUdp) {
    dst_port_udp = dstPortUdp;
}