//
// Created by Alicia on 2023/8/28.
//

#ifndef TEST_DPDK_PROGRAM_H
#define TEST_DPDK_PROGRAM_H

#include "dpdk_clib.h"
#include "dpdk_udp.h"

/******************** dpdk debug *********************/
#ifdef DEBUG
#define DEBUG_PRINT(...) do{ fprintf( stderr, __VA_ARGS__ ); } while( 0 )
#else
#define DEBUG_PRINT(...) do{ } while ( 0 )
#endif

/******************** dpdk configs (macro) *********************/
#define NC_NB_MBUF              8192 //2048
#define NC_MBUF_SIZE            (2048+sizeof(struct rte_mbuf)+RTE_PKTMBUF_HEADROOM)
#define NC_MBUF_CACHE_SIZE      32
#define NC_MAX_BURST_SIZE       32
#define NC_MAX_LCORES           64
#define NC_RX_QUEUE_PER_LCORE   1
#define NC_TX_QUEUE_PER_LCORE   1
#define NC_NB_RXD               4096 // RX descriptors
#define NC_NB_TXD               512 // TX descriptors


/******************** dpdk structures *********************/


struct mbuf_table {
    uint32_t len;
    rte_mbuf *m_table[NC_MAX_BURST_SIZE];
};

struct lcore_configuration {
    uint32_t vid; // virtual core id
    uint32_t port; // one port
    uint32_t tx_queue_id; // one TX queue
    uint32_t n_rx_queue;  // number of RX queues
    uint32_t rx_queue_list[NC_RX_QUEUE_PER_LCORE]; // list of RX queues
    mbuf_table tx_mbufs; // mbufs to hold TX queue
} __rte_cache_aligned;



/******************** dpdk packet functions *********************/
// send packets, drain TX queue
void send_pkt_burst(uint32_t lcore_id);

void enqueue_pkt_with_thres(uint32_t lcore_id, rte_mbuf *mbuf, uint32_t thres, uint32_t drain_flag);

void send_pkt_user(uint32_t lcore_id, uint8_t *pkt, uint32_t pkt_size, uint32_t seq_flag, uint32_t thres, uint32_t drain_flag);

//static uint32_t rcv_pkt_user(
//        uint32_t lcore_id, uint8_t *pkt_list, uint32_t *pkt_len_list,
//        uint32_t pkt_verify_flag, uint32_t pkt_verify_position, uint8_t pkt_verify_value,
//        uint32_t rcv_count_flag, uint32_t rcv_count_thres,
//        uint32_t rcv_us_flag, uint64_t rcv_us_thres);
/******************** dpdk check status functions *********************/
void check_link_status();
int nc_init(int argc, char **argv);

/******************** time functions *********************/
class Timer{
public:
    void start();
    void finish();
    double get_interval_second();
    double get_interval_millisecond();
    double get_interval_microsecond();
    static uint64_t get_current_tsc();
    static uint64_t get_persecond_tsc();
    static uint64_t get_permillisecond_tsc();
    static uint64_t get_permicrosecond_tsc();
private:
    uint64_t time_start = 0;
    uint64_t time_end = 0;
    uint64_t time_interval = 0;
};

extern uint32_t n_enabled_port;
extern uint32_t enabled_port_list[RTE_MAX_ETHPORTS];
extern uint32_t n_lcore;
extern uint32_t lcore_id_2_rx_queue_id_list[NC_MAX_LCORES];
extern uint32_t rx_queue_id_2_lcore_id_list[NC_MAX_LCORES];
extern rte_ether_addr port_eth_addr_list[RTE_MAX_ETHPORTS];

extern rte_mempool *pktmbuf_pool_list[NC_MAX_LCORES];
extern lcore_configuration lcore_conf_list[NC_MAX_LCORES];


#endif //TEST_DPDK_PROGRAM_H
