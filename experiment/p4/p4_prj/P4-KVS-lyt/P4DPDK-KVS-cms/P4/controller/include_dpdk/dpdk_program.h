extern "C" {
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
}

#ifdef DEBUG
#define DEBUG_PRINT(...) do{ fprintf( stderr, __VA_ARGS__ ); } while( 0 )
#else
#define DEBUG_PRINT(...) do{ } while ( 0 )
#endif

#define NC_NB_MBUF              2048 //2048
#define NC_MBUF_SIZE            (2048+sizeof(struct rte_mbuf)+RTE_PKTMBUF_HEADROOM)
#define NC_MBUF_CACHE_SIZE      32
#define NC_MAX_BURST_SIZE       32
#define NC_MAX_LCORES           64
#define NC_RX_QUEUE_PER_LCORE   1
#define NC_TX_QUEUE_PER_LCORE   1
#define NC_NB_RXD               512 // RX descriptors
#define NC_NB_TXD               512 // TX descriptors

uint32_t n_enabled_ports = 0;
uint32_t enabled_ports[RTE_MAX_ETHPORTS];
uint32_t n_lcores = 0;
uint32_t lcore_id_2_rx_queue_id[NC_MAX_LCORES];
uint32_t rx_queue_id_2_lcore_id[NC_MAX_LCORES];
struct rte_ether_addr port_eth_addrs[RTE_MAX_ETHPORTS];

struct mbuf_table {
    uint32_t len;
    struct rte_mbuf *m_table[NC_MAX_BURST_SIZE];
};

struct lcore_configuration {
    uint32_t vid; // virtual core id
    uint32_t port; // one port
    uint32_t tx_queue_id; // one TX queue
    uint32_t n_rx_queue;  // number of RX queues
    uint32_t rx_queue_list[NC_RX_QUEUE_PER_LCORE]; // list of RX queues
    struct mbuf_table tx_mbufs; // mbufs to hold TX queue
} __rte_cache_aligned;

struct rte_mempool *pktmbuf_pool[NC_MAX_LCORES];
struct lcore_configuration lcore_conf[NC_MAX_LCORES];

// -----------------------packet functions--------------------------
// send packets, drain TX queue
static void send_pkt_burst(uint32_t lcore_id) {
    struct lcore_configuration *lconf = &lcore_conf[lcore_id];
    struct rte_mbuf **m_table = (struct rte_mbuf **)lconf->tx_mbufs.m_table;

    uint32_t n = lconf->tx_mbufs.len;

    uint32_t ret = rte_eth_tx_burst(
        lconf->port,
        lconf->tx_queue_id,
        m_table,
        lconf->tx_mbufs.len);
    // tput_stat[lcore_id].tx += ret;
    if (unlikely(ret < n)) {
        do {
            rte_pktmbuf_free(m_table[ret]);
        } while (++ret < n);
    }
    lconf->tx_mbufs.len = 0;

    // struct lcore_configuration *lconf = &lcore_conf[lcore_id];
    // struct rte_mbuf **m_table = (struct rte_mbuf **)lconf->tx_mbufs.m_table;

    // uint32_t n = lconf->tx_mbufs.len;
    // int len = rand() % (lconf->tx_mbufs.len);

    // uint32_t ret = rte_eth_tx_burst(
    //     lconf->port,
    //     lconf->tx_queue_id,
    //     m_table,
    //     len);
    // tput_stat[lcore_id].tx += ret;
    // if (unlikely(ret < n)) {
    //     tput_stat[lcore_id].dropped_pkt += (n - ret);
    //     do {
    //         rte_pktmbuf_free(m_table[ret]);
    //     } while (++ret < n);
    // }
    // lconf->tx_mbufs.len = 0;

}

// put packet into TX queue
static void enqueue_pkt(uint32_t lcore_id, struct rte_mbuf *mbuf) {
    struct lcore_configuration *lconf = &lcore_conf[lcore_id];
    lconf->tx_mbufs.m_table[lconf->tx_mbufs.len++] = mbuf;

    // enough packets in TX queue
    if (unlikely(lconf->tx_mbufs.len == NC_MAX_BURST_SIZE)) {
    // if (unlikely(lconf->tx_mbufs.len == 1)) {
        send_pkt_burst(lcore_id);
    }
}

static void acquire_enqueue_pkt(uint32_t lcore_id, struct rte_mbuf *mbuf, int locks_per_txn) {
    struct lcore_configuration *lconf = &lcore_conf[lcore_id];
    lconf->tx_mbufs.m_table[lconf->tx_mbufs.len++] = mbuf;

    // enough packets in TX queue
    // if (unlikely(lconf->tx_mbufs.len == NC_MAX_BURST_SIZE)) {
    if (unlikely(lconf->tx_mbufs.len == locks_per_txn)) {
        send_pkt_burst(lcore_id);
    }
}

static void enqueue_pkt_with_thres(uint32_t lcore_id, struct rte_mbuf *mbuf, int thres, int drainFlag) {
    struct lcore_configuration *lconf = &lcore_conf[lcore_id];
    if(drainFlag == 0){
        lconf->tx_mbufs.m_table[lconf->tx_mbufs.len++] = mbuf;
    }
    // enough packets in TX queue
    int true_thres = (thres < NC_MAX_BURST_SIZE) ? thres : NC_MAX_BURST_SIZE;
    if (unlikely(lconf->tx_mbufs.len >= true_thres)) {
        send_pkt_burst(lcore_id);
    }
}

static void send_pkt_seq(uint32_t lcore_id, char *pkt, uint32_t pkt_size){

    struct lcore_configuration *lconf = &lcore_conf[lcore_id];
    struct rte_mbuf *mbuf_send;
    uint32_t rx_queue_id = lcore_id_2_rx_queue_id[lcore_id];
    mbuf_send = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]);

    assert(mbuf_send != NULL);
    mbuf_send->next = NULL;
    mbuf_send->nb_segs = 1;
    mbuf_send->ol_flags = 0;
    mbuf_send->data_len = 0;
    mbuf_send->pkt_len = 0;

    char * pkt_mem_send = rte_pktmbuf_mtod(mbuf_send, char *);
    rte_memcpy(pkt_mem_send, pkt, pkt_size);

    mbuf_send->data_len += pkt_size;
    mbuf_send->pkt_len += pkt_size;

    enqueue_pkt_with_thres(lcore_id, mbuf_send, 1, 0);
}

static void rcv_pkt_seq(uint32_t lcore_id, char *pkt, uint32_t pkt_size){

    struct lcore_configuration *lconf = &lcore_conf[lcore_id];
    struct rte_mbuf *mbuf_rcv;  
    struct rte_mbuf *mbuf_burst[NC_MAX_BURST_SIZE];

    uint32_t return_flag = 0;

    // printf("rcv_func: lcore_id = %d, lconf->port = %d, lconf->n_rx_queue = %d, lconf->rx_queue_list[i] = %d\n", 
    //     lcore_id, lconf->port, lconf->n_rx_queue, lconf->rx_queue_list[0]);

    while(1){
        for (int i = 0; i < lconf->n_rx_queue; i++) {
            // uint32_t nb_rx = rte_eth_rx_burst(lconf->port, lconf->rx_queue_list[i], mbuf_burst, NC_MAX_BURST_SIZE);
            uint32_t nb_rx = rte_eth_rx_burst(lconf->port, lconf->rx_queue_list[i], mbuf_burst, NC_MAX_BURST_SIZE);
            if(nb_rx != 0){
                // printf("checkpoint 1: receive !\n");
            }
            for (int j = 0; j < nb_rx; j++) {
                mbuf_rcv = mbuf_burst[j];
                // rte_prefetch0(rte_pktmbuf_mtod(mbuf_rcv, void *)); 
                char* pkt_mem_rcv = rte_pktmbuf_mtod(mbuf_rcv, char *);
                rte_memcpy(pkt, pkt_mem_rcv, pkt_size);
                rte_pktmbuf_free(mbuf_rcv);
                return_flag = 1;
                break;
            }
            if(return_flag == 1){
                break;
            }
        }
        if(return_flag == 1){
            break;
        }
    }
}
// -----------------------------------------------------------------


// ---------------------initialize all status-----------------------
// check link status
static void check_link_status(void) {
    const uint32_t check_interval_ms = 100;
    const uint32_t check_iterations = 90;
    uint32_t i, j;
    struct rte_eth_link link;
    for (i = 0; i < check_iterations; i++) {
        uint8_t all_ports_up = 1;
        for (j = 0; j < n_enabled_ports; j++) {
            uint32_t portid = enabled_ports[j];
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

static int nc_init(int argc, char **argv) {
    // parser dpdk parameters
    int ret = rte_eal_init(argc, argv);
    if (ret < 0) {
        rte_exit(EXIT_FAILURE, "invalid EAL arguments\n");
    }

    // check enabled ports 
    printf("rte_socket_id = %d", rte_socket_id());
    printf("create enabled ports\n");
    uint32_t n_total_ports = rte_eth_dev_count_total();
    if (n_total_ports == 0) {
      rte_exit(EXIT_FAILURE, "cannot detect ethernet ports\n");
    }
    if (n_total_ports > RTE_MAX_ETHPORTS) {
        n_total_ports = RTE_MAX_ETHPORTS;
    }
    printf("%d enabled ports\n", n_total_ports);
    if (n_total_ports > 2){
        rte_exit(EXIT_FAILURE,
            "don't support %d (larger than 2) enabled ports.\n",
            n_total_ports);
    }

    // get info for each enabled port 
    struct rte_eth_dev_info dev_info;
    n_enabled_ports = 0;
    printf("\tenabled ports: ");
    for (uint32_t i = 0; i < n_total_ports; i++) {
        enabled_ports[n_enabled_ports++] = i;
        rte_eth_dev_info_get(i, &dev_info);
        printf("%u ", i);
    }
    printf("\n");

    // find number of active lcores
    printf("create enabled cores\n\tcores: ");
    n_lcores = 0;
    for(uint32_t i = 0; i < NC_MAX_LCORES; i++) {
        if(rte_lcore_is_enabled(i)) {
            n_lcores++;
            printf("%u ",i);
        }
    }

    // ensure numbers are correct
    printf("WARNING: by default each lcore has 1 rx queue and 1 tx queue!\n");
    uint32_t rx_queues_per_port = 0;
    uint32_t tx_queues_per_port = 0;
    if (n_lcores > 0){
        if(n_lcores % n_enabled_ports == 0){
            rx_queues_per_port = n_lcores / n_enabled_ports;
            tx_queues_per_port = n_lcores / n_enabled_ports;
        }
        else{
            rte_exit(EXIT_FAILURE,
                "the number of lcores (%d) should be multiple of the number of enabled ports (%d).\n",
                n_lcores, n_enabled_ports);
        }
    }
    else{
        rte_exit(EXIT_FAILURE,
            "number of cores (%u) must be larger than 0.\n",
            n_lcores, n_enabled_ports);
    }

    // assign each lcore some RX queues and a port
    printf("set up %d RX queues per port and %d TX queues per port\n", rx_queues_per_port, tx_queues_per_port);
    uint32_t portid_offset = 0;
    uint32_t rx_queue_id = 0;
    uint32_t tx_queue_id = 0;
    uint32_t vid = 0;
    for (uint32_t i = 0; i < NC_MAX_LCORES; i++) {
        if(rte_lcore_is_enabled(i)) {
            lcore_id_2_rx_queue_id[i] = rx_queue_id;
            rx_queue_id_2_lcore_id[rx_queue_id] = i;

            lcore_conf[i].vid = vid++;
            lcore_conf[i].n_rx_queue = 1;
            for (uint32_t j = 0; j < lcore_conf[i].n_rx_queue; j++) {
                lcore_conf[i].rx_queue_list[j] = rx_queue_id++;
            }
            lcore_conf[i].tx_queue_id = tx_queue_id++;
            lcore_conf[i].port = enabled_ports[portid_offset];
            if (tx_queue_id % tx_queues_per_port == 0) {
                portid_offset++;
                rx_queue_id = 0;
                tx_queue_id = 0;
            }
        }
    }

    // initialize each port
    static struct rte_eth_conf port_conf;
    port_conf.rxmode.split_hdr_size = 0;
    port_conf.txmode.mq_mode = ETH_MQ_TX_NONE;
    for (portid_offset = 0; portid_offset < n_enabled_ports; portid_offset++) {
        uint32_t portid = enabled_ports[portid_offset];
        int32_t ret = rte_eth_dev_configure(portid, rx_queues_per_port,
                        tx_queues_per_port, &port_conf);
        if (ret < 0) {
            rte_exit(EXIT_FAILURE, "cannot configure device: err=%d, port=%u\n", 
                    ret, portid);
        }
        rte_eth_macaddr_get(portid, &port_eth_addrs[portid]);

        // initialize RX queues
        for (uint32_t i = 0; i < rx_queues_per_port; i++) {
            // create mbuf pool
            printf("create mbuf pool for port %d, rx queue %d\n", portid, i);
            char name[50];
            sprintf(name, "mbuf_pool_%d", portid * rx_queues_per_port + i);
            pktmbuf_pool[portid * rx_queues_per_port + i] = rte_mempool_create(
                name,
                NC_NB_MBUF,
                NC_MBUF_SIZE,
                NC_MBUF_CACHE_SIZE,
                sizeof(struct rte_pktmbuf_pool_private),
                rte_pktmbuf_pool_init, NULL,
                rte_pktmbuf_init, NULL,
                rte_socket_id(),
                0);
            if (pktmbuf_pool[portid * rx_queues_per_port + i] == NULL) {
                rte_exit(EXIT_FAILURE, "cannot init mbuf pool (port %d, rx queue %d)\n", portid, i);
            }

            ret = rte_eth_rx_queue_setup(portid, i, NC_NB_RXD,
                rte_eth_dev_socket_id(portid), NULL, pktmbuf_pool[portid * rx_queues_per_port + i]);
            if (ret < 0) {
                rte_exit(EXIT_FAILURE,
                    "rte_eth_rx_queue_setup: err=%d, port=%u\n", ret, portid);
            }
        }

        // initialize TX queues
        for (uint32_t i = 0; i < tx_queues_per_port; i++) {
            ret = rte_eth_tx_queue_setup(portid, i, NC_NB_TXD,
                rte_eth_dev_socket_id(portid), NULL);
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
        rte_ether_format_addr(mac_buf, RTE_ETHER_ADDR_FMT_SIZE, &port_eth_addrs[portid]);
        printf("initiaze queues and start port %u, MAC address:%s\n", portid, mac_buf);
    }

    if (!n_enabled_ports) {
        rte_exit(EXIT_FAILURE, "all available ports are disabled. Please set portmask.\n");
    }
    check_link_status();
    // init_header_template();
    return ret;
}
// -----------------------------------------------------------------
