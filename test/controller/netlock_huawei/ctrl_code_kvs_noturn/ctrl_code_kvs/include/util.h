#define NETLOCK_MODE
#ifdef DEBUG
#define DEBUG_PRINT(...) do{ fprintf( stderr, __VA_ARGS__ ); } while( 0 )
#else
#define DEBUG_PRINT(...) do{ } while ( 0 )
#endif

#define MEM_BIN_PACK            'b'
#define MEM_RAND_WEIGHT         'w'
#define MEM_RAND_12             '1'
#define MEM_RAND_200            '2'

#define RING_SIZE               16384
#define MAX_PKTS_BURST          32

#define ROLE_CLIENT             'c'
#define ROLE_SERVER_POOL        's'

#define LEN_ONE_SEGMENT         100

#define NC_MAX_PAYLOAD_SIZE     1500
#define NC_NB_MBUF              32768//16384//8191
#define NC_MBUF_SIZE            2048//(2048+sizeof(struct rte_mbuf)+RTE_PKTMBUF_HEADROOM)
#define NC_MBUF_CACHE_SIZE      32//32
#define NC_MAX_BURST_SIZE       32
#define NC_MAX_LCORES           48
#define NC_RX_QUEUE_PER_LCORE   1
#define NC_TX_QUEUE_PER_LCORE   1
#define NC_NB_RXD               8192//4096//1024 // RX descriptors
#define NC_NB_TXD               8192//1024 // TX descriptors
#define NC_DRAIN_US             10
#define MAX_PATTERN_NUM		8
#define MAX_ACTION_NUM		8

#define NODE_NUM                128
#define IP_SRC                  "10.0.0.8"
#define CLIENT_SLAVE_IP         "10.1.0.13"// no use
#define IP_DST                  "10.0.0.2"
#define CLIENT_PORT             5001
#define SERVICE_PORT            8900
#define KEY_SPACE_SIZE          1000000
#define PER_NODE_KEY_SPACE_SIZE (KEY_SPACE_SIZE / NODE_NUM)
#define BIN_SIZE                100
#define BIN_RANGE               10
#define BIN_MAX                 (BIN_SIZE * BIN_RANGE)

#define RSS_HASH_KEY_LENGTH 40



char role = ROLE_CLIENT;
uint32_t enabled_port_mask = 1;

uint16_t lcore_id_2_rx_queue_id[NC_MAX_LCORES];
uint32_t core_arr[NC_MAX_LCORES];

static uint8_t hash_key[RSS_HASH_KEY_LENGTH] = {
    0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A,
    0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A,
    0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A,
    0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A,
    0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A, 0x6D, 0x5A};
    
   /*
static uint8_t hash_key[RSS_HASH_KEY_LENGTH] = {
    0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01,
    0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01,
    0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01,
    0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01,
    0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, };
    */
static const struct rte_eth_conf port_conf_client = {
    .rxmode = {
        // .mq_mode = ETH_MQ_RX_NONE,//ETH_MQ_RX_RSS,
        // .max_rx_pkt_len = RTE_ETHER_MAX_LEN,
        .split_hdr_size = 0,
    },
    
    // .rx_adv_conf = {
    //     .rss_conf = {
    //         .rss_key = hash_key,
    //         .rss_hf = ETH_RSS_UDP,
    //         .rss_key_len = 40,
    //     },
    // },
    
    .txmode = {
        .mq_mode = ETH_MQ_TX_NONE,
    },
};

static const struct rte_eth_conf port_conf_server = {
    .rxmode = {
         .mq_mode = ETH_MQ_RX_RSS,
        .max_rx_pkt_len = RTE_ETHER_MAX_LEN,
        .split_hdr_size = 0,
    },
    
    
    .rx_adv_conf = {
        .rss_conf = {
            .rss_key = hash_key,
            .rss_hf = ETH_RSS_UDP,
            .rss_key_len = 40,
            //.rss_hf = ETH_RSS_PORT,
        },
    },
    
    .txmode = {
        .mq_mode = ETH_MQ_TX_NONE,
    },
};


/*
 * custom types
 */
//------------------------------------------------------

// void maxheap_downAdjust_fake(struct_coldobj heap[], int low, int high){
//     int i = low, j = i * 2;
//     while(j <= high){
//         if(j+1 <= high && heap[j+1].cold_count >= heap[j].cold_count){ //TOPKCOLD�Ļ�С�ں���Ҫ�ĳɴ��ں�
//             j = j + 1;
//         }
//         if(heap[j].cold_count >= heap[i].cold_count){  //TOPKCOLD�Ļ���Ҫ�ĳɴ��ں�
//             struct_coldobj temp = heap[i];
//             heap[i] = heap[j];
//             heap[j] = temp;
//             i = j;
//             j = i * 2;
//         }
//         else{
//             break;
//         }
//     }
// }

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

/*
 * global variables
 */

uint32_t enabled_ports[RTE_MAX_ETHPORTS];
uint32_t n_enabled_ports = 0;
uint32_t n_rx_queues = 0;
uint32_t n_lcores = 0;

// struct rte_mempool *pktmbuf_pool = NULL;
struct rte_mempool *pktmbuf_pool[NC_MAX_LCORES];
struct rte_ether_addr port_eth_addrs[RTE_MAX_ETHPORTS];
struct lcore_configuration lcore_conf[NC_MAX_LCORES];

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
    if (unlikely(ret < n)) {
        do {
            rte_pktmbuf_free(m_table[ret]);
        } while (++ret < n);
    }
    lconf->tx_mbufs.len = 0;
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


static void enqueue_pkt_with_thres(uint32_t lcore_id, struct rte_mbuf *mbuf, int thres, int drainFlag) {
    struct lcore_configuration *lconf = &lcore_conf[lcore_id];
    if(drainFlag == 0){
        lconf->tx_mbufs.m_table[lconf->tx_mbufs.len++] = mbuf;
    }
    // enough packets in TX queue
    if (unlikely(lconf->tx_mbufs.len >= thres)) {
        send_pkt_burst(lcore_id);
    }
}

/*
 * functions for initialization
 */

// init header template


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

struct rte_flow *generate_eth_flow(uint16_t port_phys, uint16_t src_mac, uint16_t rx_q, struct rte_flow_error *error){
    struct rte_flow_attr attr;
    struct rte_flow_item pattern[MAX_PATTERN_NUM];
    struct rte_flow_action action[MAX_ACTION_NUM];
    struct rte_flow *flow = NULL;
    struct rte_flow_action_queue queue = { .index = rx_q };
    struct rte_flow_item_eth eth_spec;
    struct rte_flow_item_eth eth_mask = {.src.addr_bytes = "\xff\xff\xff\xff\xff\xff"};
    int res;

    printf("generate flow\n");
    printf("check: rx_q = %d, src_mac = %d\n", rx_q, src_mac);
    /* set the rule attribute. in this case only ingress packets will be checked. */
	memset(pattern, 0, sizeof(pattern));
	memset(action, 0, sizeof(action));
	memset(&attr, 0, sizeof(struct rte_flow_attr));    
    attr.ingress = 1;
    /* create the action sequence. one action only,  move packet to queue */
    action[0].type = RTE_FLOW_ACTION_TYPE_QUEUE;//choose a queue
    action[0].conf = &queue;
    action[1].type = RTE_FLOW_ACTION_TYPE_END; //A list of actions is terminated by a END action.
    /* set the first level of the pattern (ETH). since in this example we just want to get the ipv4 we set this level to allow all. */
	pattern[0].type = RTE_FLOW_ITEM_TYPE_ETH; // need to build correct stack
	pattern[1].type = RTE_FLOW_ITEM_TYPE_END;
    // /*
    //  * setting the second level of the pattern (IP). in this example this is the level we care about so we set it according to the parameters.
    //  */

    // struct rte_ether_addr src_addr = {.addr_bytes = {0x98, 0x03, 0x9b, 0xca, 0x48, 0x38}};//0x98, 0x03, 0x9b, 0xc7, 0xc7, 0xfc
    struct rte_ether_addr src_addr = {.addr_bytes = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};
    src_addr.addr_bytes[5] = src_mac;

    memset(&eth_spec, 0, sizeof(struct rte_flow_item_eth));
    rte_ether_addr_copy(&src_addr, &eth_spec.src);
    // printf("829: eth_spec.src = %d\n", eth_spec.src);
    // eth_spec.type = rte_cpu_to_be_16(0x0800); no use
    pattern[0].spec = &eth_spec;
    pattern[0].mask = &eth_mask;

	res = rte_flow_validate(port_phys, &attr, pattern, action, error);
	if (!res)
		flow = rte_flow_create(port_phys, &attr, pattern, action, error);
	return flow;
}


// initialize all status
static void nc_init(void) {
    uint32_t i, j;

    // // create mbuf pool
    // printf("create mbuf pool\n");
    // pktmbuf_pool = rte_mempool_create(
    //     "mbuf_pool",
    //     NC_NB_MBUF,
    //     NC_MBUF_SIZE,
    //     NC_MBUF_CACHE_SIZE,
    //     sizeof(struct rte_pktmbuf_pool_private),
    //     rte_pktmbuf_pool_init, NULL,
    //     rte_pktmbuf_init, NULL,
    //     rte_socket_id(),
    //     0);
    // if (pktmbuf_pool == NULL) {
    //     rte_exit(EXIT_FAILURE, "cannot init mbuf pool\n");
    // }
    // printf("rte_socket_id = %d", rte_socket_id());

    // determine available ports
    printf("create enabled ports\n");
    uint32_t n_total_ports = 0;
    n_total_ports = rte_eth_dev_count_total();

    if (n_total_ports == 0) {
      rte_exit(EXIT_FAILURE, "cannot detect ethernet ports\n");
    }
    if (n_total_ports > RTE_MAX_ETHPORTS) {
        n_total_ports = RTE_MAX_ETHPORTS;
    }

    // get info for each enabled port 
    struct rte_eth_dev_info dev_info;
    n_enabled_ports = 0;
    printf("\tports: ");
    
    for (i = 0; i < n_total_ports; i++) {
        if ((enabled_port_mask & (1 << i)) == 0) {
            continue;
        }
        enabled_ports[n_enabled_ports++] = i;
        rte_eth_dev_info_get(i, &dev_info);
        printf("%u ", i);
    }
    printf("\n");

    // find number of active lcores
    printf("create enabled cores\n\tcores: ");
    n_lcores = 0;
    for(i = 0; i < NC_MAX_LCORES; i++) {
        if(rte_lcore_is_enabled(i)) {
            n_lcores++;
            printf("%u ",i);
        }
    }

    // ensure numbers are correct
    if (n_lcores % n_enabled_ports != 0) {
        rte_exit(EXIT_FAILURE,
            "number of cores (%u) must be multiple of ports (%u)\n",
            n_lcores, n_enabled_ports);
    }

    uint32_t rx_queues_per_lcore = NC_RX_QUEUE_PER_LCORE;
    uint32_t rx_queues_per_port = rx_queues_per_lcore * n_lcores / n_enabled_ports;
    uint32_t tx_queues_per_port = n_lcores / n_enabled_ports;

    if (rx_queues_per_port < rx_queues_per_lcore) {
        rte_exit(EXIT_FAILURE,
            "rx_queues_per_port (%u) must be >= rx_queues_per_lcore (%u)\n", rx_queues_per_port, rx_queues_per_lcore);
    }

    // assign each lcore some RX queues and a port
    printf("set up %d RX queues per port and %d TX queues per port\n", rx_queues_per_port, tx_queues_per_port);
    uint32_t portid_offset = 0;
    uint32_t rx_queue_id = 0;
    uint32_t tx_queue_id = 0;
    uint32_t vid = 0;
    for (i = 0; i < NC_MAX_LCORES; i++) {
        if(rte_lcore_is_enabled(i)) {
            n_rx_queues++;
            lcore_id_2_rx_queue_id[i] = rx_queue_id;
            core_arr[rx_queue_id] = i;

            lcore_conf[i].vid = vid++;
            lcore_conf[i].n_rx_queue = rx_queues_per_lcore;
            for (j = 0; j < lcore_conf[i].n_rx_queue; j++) {
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

    for (i=0;i<NC_MAX_LCORES;i++) {
        DEBUG_PRINT("l_core_%d, n_rx:%d\n", i, lcore_conf[i].n_rx_queue);
        DEBUG_PRINT("l_core_%d, tx_q_id:%d\n",i, lcore_conf[i].tx_queue_id);
        for (j = 0; j < lcore_conf[i].n_rx_queue; j++) {
            DEBUG_PRINT("rx_q[%d]:%d\n", j, lcore_conf[i].rx_queue_list[j]);
        }
    }
    DEBUG_PRINT("rx_queues_per_port:%d\n", rx_queues_per_port);

    // initialize each port
    for (portid_offset = 0; portid_offset < n_enabled_ports; portid_offset++) {
        uint32_t portid = enabled_ports[portid_offset];
        int32_t ret;
        if (role == ROLE_CLIENT) {
            ret = rte_eth_dev_configure(portid, rx_queues_per_port,
                tx_queues_per_port, &port_conf_client);
        }
        else {
            ret = rte_eth_dev_configure(portid, rx_queues_per_port,
                tx_queues_per_port, &port_conf_server);
        }
        if (ret < 0) {
            rte_exit(EXIT_FAILURE, "cannot configure device: err=%d, port=%u\n",
               ret, portid);
        }
        rte_eth_macaddr_get(portid, &port_eth_addrs[portid]);

        // initialize RX queues
        for (i = 0; i < rx_queues_per_port; i++) {
            // create mbuf pool
            printf("create mbuf pool\n");
            char name[50];
            sprintf(name,"mbuf_pool_%d",i);
            pktmbuf_pool[i] = rte_mempool_create(
                name,
                NC_NB_MBUF,
                NC_MBUF_SIZE,
                NC_MBUF_CACHE_SIZE,
                sizeof(struct rte_pktmbuf_pool_private),
                rte_pktmbuf_pool_init, NULL,
                rte_pktmbuf_init, NULL,
                rte_socket_id(),
                0);
            if (pktmbuf_pool[i] == NULL) {
                rte_exit(EXIT_FAILURE, "cannot init mbuf pool\n");
            }

            ret = rte_eth_rx_queue_setup(portid, i, NC_NB_RXD,
                rte_eth_dev_socket_id(portid), NULL, pktmbuf_pool[i]);
            if (ret < 0) {
                rte_exit(EXIT_FAILURE,
                    "rte_eth_rx_queue_setup: err=%d, port=%u\n", ret, portid);
            }
         }

        // initialize TX queues
        for (i = 0; i < tx_queues_per_port; i++) {
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
        printf("initiaze queues and start port %u, MAC address:%s\n",
           portid, mac_buf);

        //-----------------------------------------------
        // rte_flow
        struct rte_flow_error error;
        printf("n_rx_queues = %d\n", n_rx_queues);
        for(int i = 1; i < n_rx_queues; i++){
            if(i % 2 == 1){
                struct rte_flow * flow = generate_eth_flow(portid, (i - 1) / 2, i, &error); // udpport,queue,error
                if (!flow) {
                    printf("Flow can't be created %d message: %s\n", error.type, error.message ? error.message : "(no stated reason)");
                    rte_exit(EXIT_FAILURE, "error in creating flow");
                } 
            }
        }
    }

    if (!n_enabled_ports) {
        rte_exit(EXIT_FAILURE, "all available ports are disabled. Please set portmask.\n");
    }
    check_link_status();
    // init_header_template();
}

