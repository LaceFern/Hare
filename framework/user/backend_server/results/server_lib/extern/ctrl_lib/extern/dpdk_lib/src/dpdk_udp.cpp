//
// Created by Alicia on 2023/8/30.
//

#include <iostream>
#include "../include/dpdk_udp.h"
#include "../include/dpdk_basic.h"

rte_flow *generate_udp_flow(uint16_t port_phys, uint16_t port_udp, uint16_t rx_q, struct rte_flow_error *error){
    rte_flow_attr attr{};
    rte_flow_item pattern[MAX_PATTERN_NUM];
    rte_flow_action action[MAX_ACTION_NUM];
    rte_flow *flow = nullptr;
    rte_flow_action_queue queue = { .index = rx_q };
    rte_flow_item_udp udp_spec{};
    rte_flow_item_udp udp_mask{};
    int res;

    printf("check: rx_q = %d, port_udp = %d\n", rx_q, port_udp);
    /* set the rule attribute. in this case only ingress packets will be checked. */
    memset(pattern, 0, sizeof(pattern));
    memset(action, 0, sizeof(action));
    memset(&attr, 0, sizeof(rte_flow_attr));
    attr.ingress = 1;
    /* create the action sequence. one action only,  move packet to queue */
    action[0].type = RTE_FLOW_ACTION_TYPE_QUEUE;//choose a queue
    action[0].conf = &queue;
    action[1].type = RTE_FLOW_ACTION_TYPE_END; //A list of actions is terminated by a END action.
    /* set the first level of the pattern (ETH). since in this example we just want to get the ipv4 we set this level to allow all. */
    pattern[0].type = RTE_FLOW_ITEM_TYPE_ETH; // need to build correct stack
    pattern[1].type = RTE_FLOW_ITEM_TYPE_IPV4;
    pattern[2].type = RTE_FLOW_ITEM_TYPE_UDP;
    pattern[3].type = RTE_FLOW_ITEM_TYPE_END;
    // /*
    //  * setting the second level of the pattern (IP). in this example this is the level we care about so we set it according to the parameters.
    //  */
    memset(&udp_spec, 0, sizeof(rte_flow_item_udp));
    memset(&udp_mask, 0, sizeof(rte_flow_item_udp));
    udp_spec.hdr.dst_port = htons(port_udp);
    udp_mask.hdr.dst_port = 0xffff;
    pattern[2].spec = &udp_spec;
    pattern[2].mask = &udp_mask;

    res = rte_flow_validate(port_phys, &attr, pattern, action, error);
    if (!res) flow = rte_flow_create(port_phys, &attr, pattern, action, error);
    return flow;
}

int nc_init_udpflow(int argc, char **argv){
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


        //rte_flow
        rte_flow_error error{};
        for(uint32_t i = 0; i < rx_queues_per_port; i++){
            struct rte_flow * flow;
            flow = generate_udp_flow(portid, i + RCV_UDP_PORT_BIAS, i / NC_RX_QUEUE_PER_LCORE, &error);
            if (!flow) {
                printf("Flow can't be created %d message: %s\n", error.type, error.message ? error.message : "(no stated reason)");
                rte_exit(EXIT_FAILURE, "error in creating flow");
            }
        }
    }

    if (!n_enabled_port) {
        rte_exit(EXIT_FAILURE, "all available ports are disabled. Please set portmask.\n");
    }
    check_link_status();
    // init_header_template();
    return ret;
}


uint32_t rcv_pkt_user(
        uint32_t lcore_id, uint8_t *payload_list, uint32_t *payload_len_list,
        uint32_t payload_verify_flag, uint32_t payload_verify_position, uint8_t payload_verify_value,
        uint32_t rcv_count_flag, uint32_t rcv_count_thres,
        uint32_t rcv_us_flag, uint64_t rcv_us_thres){
    lcore_configuration *lconf = &lcore_conf_list[lcore_id];
    rte_mbuf *mbuf_rcv;
    rte_mbuf *mbuf_burst[NC_MAX_BURST_SIZE];

    uint32_t rcv_count_stop_flag = 1;
    uint32_t rcv_us_stop_flag = 1;
    if(rcv_count_flag == 1){
        rcv_count_stop_flag = 0;
    }
    if(rcv_us_flag == 1){
        rcv_us_stop_flag = 0;
    }
//    uint32_t no_verify_pkt_count = 0;
    uint32_t payload_count = 0;
//    uint32_t no_verify_pkt_bias = 0;
    uint32_t payload_bias = 0;

    uint64_t start_tsc = rte_rdtsc();
    uint64_t end_tsc = start_tsc + rte_get_tsc_hz() / 1000000 * rcv_us_thres;

    uint32_t end_flag = 0;
    while(end_flag == 0) {
        for (int i = 0; i < lconf->n_rx_queue; i++) {
            uint32_t nb_rx = rte_eth_rx_burst(lconf->port, lconf->rx_queue_list[i], mbuf_burst, NC_MAX_BURST_SIZE);

            for (int j = 0; j < nb_rx; j++) {
                mbuf_rcv = mbuf_burst[j];
                rte_prefetch0(rte_pktmbuf_mtod(mbuf_rcv, void *));
                uint8_t *pkt_mem_rcv = rte_pktmbuf_mtod(mbuf_rcv, uint8_t *);

                if(payload_verify_flag == 1){
                    if(pkt_mem_rcv[UDP_HEADER_SIZE + payload_verify_position] == payload_verify_value){
                        payload_len_list[payload_count] = mbuf_rcv->pkt_len - UDP_HEADER_SIZE;
                        rte_memcpy(payload_list + payload_bias, pkt_mem_rcv + UDP_HEADER_SIZE, payload_len_list[payload_count]);
                        payload_bias += payload_len_list[payload_count];
                        payload_count++;
                    }
                }
                else{
                    payload_len_list[payload_count] = mbuf_rcv->pkt_len - UDP_HEADER_SIZE;
                    rte_memcpy(payload_list + payload_bias, pkt_mem_rcv + UDP_HEADER_SIZE, payload_len_list[payload_count]);
                    payload_bias += payload_len_list[payload_count];
                    payload_count++;
                }
                rte_pktmbuf_free(mbuf_rcv);
            }
        }
        if(rcv_count_stop_flag == 0 && payload_count >= rcv_count_thres){
            rcv_count_stop_flag = 1;
        }
        uint64_t cur_tsc = rte_rdtsc();
        if(rcv_us_stop_flag == 0 && cur_tsc >= end_tsc){
            rcv_us_stop_flag = 1;
        }

        if(rcv_count_stop_flag == 1 || rcv_us_stop_flag == 1 ||
           rcv_count_flag == 0 && rcv_us_flag == 0){
            end_flag = 1;
        }
    }
    return payload_count;
}


uint32_t rcv_complete_pkt_user(
        uint32_t lcore_id, uint8_t *pkt_list, uint32_t *pkt_len_list,
        uint32_t pkt_verify_flag, uint32_t pkt_verify_position, uint8_t pkt_verify_value,
        uint32_t rcv_count_flag, uint32_t rcv_count_thres,
        uint32_t rcv_us_flag, uint64_t rcv_us_thres){
    lcore_configuration *lconf = &lcore_conf_list[lcore_id];
    rte_mbuf *mbuf_rcv;
    rte_mbuf *mbuf_burst[NC_MAX_BURST_SIZE];

    uint32_t rcv_count_stop_flag = 1;
    uint32_t rcv_us_stop_flag = 1;
    if(rcv_count_flag == 1){
        rcv_count_stop_flag = 0;
    }
    if(rcv_us_flag == 1){
        rcv_us_stop_flag = 0;
    }
//    uint32_t no_verify_pkt_count = 0;
    uint32_t pkt_count = 0;
//    uint32_t no_verify_pkt_bias = 0;
    uint32_t pkt_bias = 0;

    uint64_t start_tsc = rte_rdtsc();
    uint64_t end_tsc = start_tsc + rte_get_tsc_hz() / 1000000 * rcv_us_thres;

    uint32_t end_flag = 0;
    while(end_flag == 0) {
        for (int i = 0; i < lconf->n_rx_queue; i++) {
            uint32_t nb_rx = rte_eth_rx_burst(lconf->port, lconf->rx_queue_list[i], mbuf_burst, NC_MAX_BURST_SIZE);

            for (int j = 0; j < nb_rx; j++) {
                mbuf_rcv = mbuf_burst[j];
                rte_prefetch0(rte_pktmbuf_mtod(mbuf_rcv, void *));
                uint8_t *pkt_mem_rcv = rte_pktmbuf_mtod(mbuf_rcv, uint8_t *);

                if(pkt_verify_flag == 1){
                    if(pkt_mem_rcv[pkt_verify_position] == pkt_verify_value){
                        pkt_len_list[pkt_count] = mbuf_rcv->pkt_len;
                        rte_memcpy(pkt_list + pkt_bias, pkt_mem_rcv, pkt_len_list[pkt_count]);
                        pkt_bias += pkt_len_list[pkt_count];
                        pkt_count++;
                    }
                }
                else{
                    pkt_len_list[pkt_count] = mbuf_rcv->pkt_len;
                    rte_memcpy(pkt_list + pkt_bias, pkt_mem_rcv, pkt_len_list[pkt_count]);
                    pkt_bias += pkt_len_list[pkt_count];
                    pkt_count++;
                }
                rte_pktmbuf_free(mbuf_rcv);
            }
        }

        if(rcv_count_stop_flag == 0 && pkt_count >= rcv_count_thres){
            rcv_count_stop_flag = 1;
        }
        uint64_t cur_tsc = rte_rdtsc();
        if(rcv_us_stop_flag == 0 && cur_tsc >= end_tsc){
            rcv_us_stop_flag = 1;
        }

        if(rcv_count_stop_flag == 1 || rcv_us_stop_flag == 1 ||
           rcv_count_flag == 0 && rcv_us_flag == 0){
            end_flag = 1;
        }
//        if(rcv_count_stop_flag == 1 && rcv_us_stop_flag == 1 ||
//           rcv_count_flag == 0 && rcv_us_stop_flag == 1 ||
//           rcv_count_stop_flag == 1 && rcv_us_flag == 0 ||
//           rcv_count_flag == 0 && rcv_us_flag == 0){
//            end_flag = 1;
//        }
    }
    return pkt_count;
}