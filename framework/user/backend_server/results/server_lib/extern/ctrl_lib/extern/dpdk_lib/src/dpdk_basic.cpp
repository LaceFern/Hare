//
// Created by Alicia on 2023/8/30.
//

#include <iostream>
#include "dpdk_basic.h"

//static uint32_t rcv_pkt_user(
//        uint32_t lcore_id, uint8_t *pkt_list, uint32_t *pkt_len_list,
//        uint32_t pkt_verify_flag, uint32_t pkt_verify_position, uint8_t pkt_verify_value,
//        uint32_t rcv_count_flag, uint32_t rcv_count_thres,
//        uint32_t rcv_us_flag, uint64_t rcv_us_thres){
//    lcore_configuration *lconf = &lcore_conf_list[lcore_id];
//    rte_mbuf *mbuf_rcv;
//    rte_mbuf *mbuf_burst[NC_MAX_BURST_SIZE];
//
//    uint32_t rcv_count_stop_flag = 1;
//    uint32_t rcv_us_stop_flag = 1;
//    if(rcv_count_flag == 1){
//        rcv_count_stop_flag = 0;
//    }
//    if(rcv_us_flag == 1){
//        rcv_us_stop_flag = 0;
//    }
//    uint32_t rcv_count = 0;
//    uint32_t rcv_bias = 0;
//
//    uint64_t start_tsc = rte_rdtsc();
//    uint64_t end_tsc = start_tsc + rte_get_tsc_hz() / 1000000 * rcv_us_thres;
//
//    while(rcv_count_stop_flag == 0 || rcv_us_stop_flag == 0) {
//        for (int i = 0; i < lconf->n_rx_queue; i++) {
//            uint32_t nb_rx = rte_eth_rx_burst(lconf->port, lconf->rx_queue_list[i], mbuf_burst, NC_MAX_BURST_SIZE);
//
//            for (int j = 0; j < nb_rx; j++) {
//                mbuf_rcv = mbuf_burst[j];
//                //rte_prefetch0(rte_pktmbuf_mtod(mbuf_rcv, void *));
//                uint8_t *pkt_mem_rcv = rte_pktmbuf_mtod(mbuf_rcv, uint8_t *);
//
//                if(pkt_verify_flag == 1){
//                    uint8_t pkt_verify_value_tmp = pkt_mem_rcv[pkt_verify_position];
//                    if(pkt_verify_value_tmp == pkt_verify_value){
//                        pkt_len_list[rcv_count] = mbuf_rcv->pkt_len;
//                        rte_memcpy(pkt_list + rcv_bias, pkt_mem_rcv, mbuf_rcv->pkt_len);
//                        rcv_count++;
//                        rcv_bias += mbuf_rcv->pkt_len;
//                    }
//                    else{
//                        rcv_count++;
//                        rcv_bias += mbuf_rcv->pkt_len;
//                    }
//                }
//                else{
//                    pkt_len_list[rcv_count] = mbuf_rcv->pkt_len;
//                    rte_memcpy(pkt_list + rcv_bias, pkt_mem_rcv, pkt_len_list[rcv_count]);
//                    rcv_count++;
//                    rcv_bias += mbuf_rcv->pkt_len;
//                }
//                rte_pktmbuf_free(mbuf_rcv);
//            }
//        }
//        if(rcv_count_stop_flag == 0 && rcv_count >= rcv_count_thres){
//            rcv_count_stop_flag = 1;
//        }
//        uint64_t cur_tsc = rte_rdtsc();
//        if(rcv_us_stop_flag == 0 && cur_tsc >= end_tsc){
//            rcv_us_stop_flag = 1;
//        }
//    }
//    return rcv_count;
//}

uint32_t n_enabled_port = 0;
uint32_t enabled_port_list[RTE_MAX_ETHPORTS];
uint32_t n_lcore = 0;
uint32_t lcore_id_2_rx_queue_id_list[NC_MAX_LCORES];
uint32_t rx_queue_id_2_lcore_id_list[NC_MAX_LCORES];
rte_ether_addr port_eth_addr_list[RTE_MAX_ETHPORTS];

rte_mempool *pktmbuf_pool_list[NC_MAX_LCORES];
lcore_configuration lcore_conf_list[NC_MAX_LCORES];

// check link status
void check_link_status() {
    const uint32_t check_interval_ms = 100;
    const uint32_t check_iterations = 90;
    uint32_t i, j;
    rte_eth_link link{};
    for (i = 0; i < check_iterations; i++) {
        uint8_t all_ports_up = 1;
        for (j = 0; j < n_enabled_port; j++) {
            uint32_t portid = enabled_port_list[j];
            memset(&link, 0, sizeof(link));
            rte_eth_link_get_nowait(portid, &link);
            if (link.link_status) {
                printf("\tport %u link up - speed %u Mbps - %s\n",
                       portid,
                       link.link_speed,
                       (link.link_duplex == ETH_LINK_FULL_DUPLEX) ?
                       "full-duplex" : "half-duplex");
            } else {
                all_ports_up = 0;
            }
        }
        if (all_ports_up == 1) {
            printf("check link status finish: all ports are up\n");
            break;
        } else if (i == check_iterations - 1) {
            printf("check link status finish: not all ports are up\n");
        } else {
            rte_delay_ms(check_interval_ms);
        }
    }
}


int nc_init(int argc, char **argv){
    // parser dpdk parameters
    int ret = rte_eal_init(argc, argv);
    if (ret < 0) {
        rte_exit(EXIT_FAILURE, "invalid EAL arguments\n");
    }

    // check enabled ports
    printf("rte_socket_id = %d", rte_socket_id());
    printf("create enabled ports\n");
    uint32_t n_total_port = rte_eth_dev_count_total();
    if (n_total_port == 0) {
        rte_exit(EXIT_FAILURE, "cannot detect ethernet ports\n");
    }
    if (n_total_port > RTE_MAX_ETHPORTS) {
        n_total_port = RTE_MAX_ETHPORTS;
    }
    printf("%d enabled ports\n", n_total_port);

    // get info for each enabled port
    rte_eth_dev_info dev_info{};
    n_enabled_port = 0;
    printf("\tenabled ports: ");
    for (uint32_t i = 0; i < n_total_port; i++) {
        enabled_port_list[n_enabled_port++] = i;
        rte_eth_dev_info_get(i, &dev_info);
        printf("%u ", i);
    }
    printf("\n");

    // check mac address
    for (uint32_t portid_offset = 0; portid_offset < n_enabled_port; portid_offset++) {
        char mac_buf[RTE_ETHER_ADDR_FMT_SIZE];
        uint32_t portid = enabled_port_list[portid_offset];
        rte_ether_format_addr(mac_buf, RTE_ETHER_ADDR_FMT_SIZE, &port_eth_addr_list[portid]);
        printf("initiaze queues and start port %u, MAC address:%s\n", portid, mac_buf);
    }

    // find number of active lcores
    printf("create enabled cores\n\tcores: ");
    n_lcore = 0;
    for(uint32_t i = 0; i < NC_MAX_LCORES; i++) {
        if(rte_lcore_is_enabled(i)) {
            n_lcore++;
            printf("%u ",i);
        }
    }

    // ensure numbers are correct
    printf("WARNING: by default each lcore has 1 rx queue and 1 tx queue!\n");
    uint32_t rx_queues_per_port = 0;
    uint32_t tx_queues_per_port = 0;
    if (n_lcore > 0){
        if(n_lcore % n_enabled_port == 0){
            rx_queues_per_port = n_lcore / n_enabled_port;
            tx_queues_per_port = n_lcore / n_enabled_port;
        }
        else{
            rte_exit(EXIT_FAILURE,
                     "the number of lcores (%d) should be multiple of the number of enabled ports (%d).\n",
                     n_lcore, n_enabled_port);
        }
    }
    else{
        rte_exit(EXIT_FAILURE,
                 "number of cores (%u) must be larger than 0.\n",
                 n_lcore);
    }

    // assign each lcore some RX queues and a port
    printf("set up %d RX queues per port and %d TX queues per port\n", rx_queues_per_port, tx_queues_per_port);
    uint32_t portid_offset = 0;
    uint32_t rx_queue_id = 0;
    uint32_t tx_queue_id = 0;
    uint32_t vid = 0;
    for (uint32_t i = 0; i < NC_MAX_LCORES; i++) {
        if(rte_lcore_is_enabled(i)) {
            lcore_id_2_rx_queue_id_list[i] = rx_queue_id;
            rx_queue_id_2_lcore_id_list[rx_queue_id] = i;

            lcore_conf_list[i].vid = vid++;
            lcore_conf_list[i].n_rx_queue = 1;
            for (uint32_t j = 0; j < lcore_conf_list[i].n_rx_queue; j++) {
                lcore_conf_list[i].rx_queue_list[j] = rx_queue_id++;
            }
            lcore_conf_list[i].tx_queue_id = tx_queue_id++;
            lcore_conf_list[i].port = enabled_port_list[portid_offset];
            if (tx_queue_id % tx_queues_per_port == 0) {
                portid_offset++;
                rx_queue_id = 0;
                tx_queue_id = 0;
            }
        }
    }

    // initialize each port
    rte_eth_conf port_conf{};
    port_conf.rxmode.split_hdr_size = 0;
    port_conf.txmode.mq_mode = ETH_MQ_TX_NONE;
    for (portid_offset = 0; portid_offset < n_enabled_port; portid_offset++) {
        uint32_t portid = enabled_port_list[portid_offset];
        int32_t ret = rte_eth_dev_configure(portid, rx_queues_per_port,
                                            tx_queues_per_port, &port_conf);
        if (ret < 0) {
            rte_exit(EXIT_FAILURE, "cannot configure device: err=%d, port=%u\n",
                     ret, portid);
        }
        rte_eth_macaddr_get(portid, &port_eth_addr_list[portid]);

        // initialize RX queues
        for (uint32_t i = 0; i < rx_queues_per_port; i++) {
            // create mbuf pool
            printf("create mbuf pool for port %d, rx queue %d\n", portid, i);
            char name[50];
            sprintf(name, "mbuf_pool_%d", portid * rx_queues_per_port + i);
            pktmbuf_pool_list[portid * rx_queues_per_port + i] = rte_mempool_create(
                    name,
                    NC_NB_MBUF,
                    NC_MBUF_SIZE,
                    NC_MBUF_CACHE_SIZE,
                    sizeof(rte_pktmbuf_pool_private),
                    rte_pktmbuf_pool_init, nullptr,
                    rte_pktmbuf_init, nullptr,
                    rte_socket_id(),
                    0);
            if (pktmbuf_pool_list[portid * rx_queues_per_port + i] == nullptr) {
                rte_exit(EXIT_FAILURE, "cannot init mbuf pool (port %d, rx queue %d)\n", portid, i);
            }

            ret = rte_eth_rx_queue_setup(portid, i, NC_NB_RXD,
                                         rte_eth_dev_socket_id(portid), nullptr, pktmbuf_pool_list[portid * rx_queues_per_port + i]);
            if (ret < 0) {
                rte_exit(EXIT_FAILURE,
                         "rte_eth_rx_queue_setup: err=%d, port=%u\n", ret, portid);
            }
        }

        // initialize TX queues
        for (uint32_t i = 0; i < tx_queues_per_port; i++) {
            ret = rte_eth_tx_queue_setup(portid, i, NC_NB_TXD,
                                         rte_eth_dev_socket_id(portid), nullptr);
            if (ret < 0) {
                rte_exit(EXIT_FAILURE,
                         "rte_eth_tx_queue_setup: err=%d, port=%u\n", ret, portid);
            }
        }

        // start device
        ret = rte_eth_dev_start(portid);
        if (ret < 0) {
            rte_exit(EXIT_FAILURE,
                     "rte_eth_dev_start: err=%d, port=%u\n", ret, portid);
        }

        rte_eth_promiscuous_enable(portid);

        char mac_buf[RTE_ETHER_ADDR_FMT_SIZE];
        rte_ether_format_addr(mac_buf, RTE_ETHER_ADDR_FMT_SIZE, &port_eth_addr_list[portid]);
        printf("initiaze queues and start port %u, MAC address:%s\n", portid, mac_buf);


//        //rte_flow
//        rte_flow_error error{};
//        for(uint32_t i = 0; i < rx_queues_per_port; i++){
//            struct rte_flow * flow;
//            flow = generate_udp_flow(portid, i + RCV_UDP_PORT_BIAS, i / NC_RX_QUEUE_PER_LCORE, &error);
//            if (!flow) {
//                printf("Flow can't be created %d message: %s\n", error.type, error.message ? error.message : "(no stated reason)");
//                rte_exit(EXIT_FAILURE, "error in creating flow");
//            }
//        }
    }

    if (!n_enabled_port) {
        rte_exit(EXIT_FAILURE, "all available ports are disabled. Please set portmask.\n");
    }
    check_link_status();
    // init_header_template();
    return ret;
}

void send_pkt_user(uint32_t lcore_id, uint8_t *pkt, uint32_t pkt_size, uint32_t seq_flag, uint32_t thres,
                   uint32_t drain_flag) {
    rte_mbuf *mbuf_send;
    uint32_t rx_queue_id = lcore_id_2_rx_queue_id_list[lcore_id];
    mbuf_send = rte_pktmbuf_alloc(pktmbuf_pool_list[rx_queue_id]);

    assert(mbuf_send != nullptr);
    mbuf_send->next = nullptr;
    mbuf_send->nb_segs = 1;
    mbuf_send->ol_flags = 0;
    mbuf_send->data_len = 0;
    mbuf_send->pkt_len = 0;

    uint8_t * pkt_mem_send = rte_pktmbuf_mtod(mbuf_send, uint8_t *);
    rte_memcpy(pkt_mem_send, pkt, pkt_size);

    mbuf_send->data_len += pkt_size;
    mbuf_send->pkt_len += pkt_size;

    if(seq_flag == 1){
        enqueue_pkt_with_thres(lcore_id, mbuf_send, 1, 0);
    }
    else{
        enqueue_pkt_with_thres(lcore_id, mbuf_send, thres, drain_flag);
    }
}

void send_pkt_burst(uint32_t lcore_id) {
    lcore_configuration *lconf = &lcore_conf_list[lcore_id];
    auto **m_table = (rte_mbuf **)lconf->tx_mbufs.m_table;
    uint32_t n = lconf->tx_mbufs.len;
    uint32_t ret = rte_eth_tx_burst(
            lconf->port,
            lconf->tx_queue_id,
            m_table,
            lconf->tx_mbufs.len);
    // tput_stat[lcore_id].tx += ret;
//    std::cout << "send burst! ret = " << ret << std::endl;
    if (unlikely(ret < n)) {
        do {
            rte_pktmbuf_free(m_table[ret]);
        } while (++ret < n);
    }
    lconf->tx_mbufs.len = 0;
}

void enqueue_pkt_with_thres(uint32_t lcore_id, rte_mbuf *mbuf, uint32_t thres, uint32_t drain_flag) {
    lcore_configuration *lconf = &lcore_conf_list[lcore_id];
    if(drain_flag == 0){
        lconf->tx_mbufs.m_table[lconf->tx_mbufs.len++] = mbuf;
    }
    // enough packets in TX queue
    int true_thres = (thres < NC_MAX_BURST_SIZE) ? thres : NC_MAX_BURST_SIZE;
    if (unlikely(lconf->tx_mbufs.len >= true_thres)) {
        send_pkt_burst(lcore_id);
    }
}

/******************** time functions *********************/
void Timer::start(){
    time_start = rte_rdtsc();
}

void Timer::finish() {
    time_end = rte_rdtsc();
}

double Timer::get_interval_second() {
    time_interval = time_end - time_start;
    return (double)(time_end - time_start) / (double)get_persecond_tsc();
}

double Timer::get_interval_millisecond() {
    time_interval = time_end - time_start;
    return (double)(time_end - time_start) / (double)get_permillisecond_tsc();
}

double Timer::get_interval_microsecond() {
    time_interval = time_end - time_start;
    return (double)(time_end - time_start) / (double)get_permicrosecond_tsc();
}

uint64_t Timer::get_current_tsc() {
    return rte_rdtsc();
}

uint64_t Timer::get_persecond_tsc() {
    return rte_get_tsc_hz();
}

uint64_t Timer::get_permillisecond_tsc() {
    return rte_get_tsc_hz() / 1000;
}

uint64_t Timer::get_permicrosecond_tsc() {
    return rte_get_tsc_hz() / 1000000;
}