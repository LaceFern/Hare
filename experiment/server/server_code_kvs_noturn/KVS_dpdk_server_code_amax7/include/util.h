#ifdef DEBUG
#define DEBUG_PRINT(...) do{ fprintf( stderr, __VA_ARGS__ ); } while( 0 )
#else
#define DEBUG_PRINT(...) do{ } while ( 0 )
#endif

#define BIN_SIZE                100
#define BIN_RANGE               10
#define BIN_MAX                 (BIN_SIZE * BIN_RANGE)
//-------------------------------------------------
#define ROLE_CLIENT             'c'
#define ROLE_SERVER_POOL        's'

#define NC_MAX_PAYLOAD_SIZE     1500
#define NC_NB_MBUF              32768
#define NC_MBUF_SIZE            (2048+sizeof(struct rte_mbuf)+RTE_PKTMBUF_HEADROOM)
#define NC_MBUF_CACHE_SIZE      32
#define NC_MAX_BURST_SIZE       32
#define NC_MAX_LCORES           48
#define NC_RX_QUEUE_PER_LCORE   1
#define NC_TX_QUEUE_PER_LCORE   1
#define NC_NB_RXD               1024 // RX descriptors
#define NC_NB_TXD               1024 // TX descriptors
#define NC_DRAIN_US             10
#define MAX_PATTERN_NUM		8
#define MAX_ACTION_NUM		8

#define RSS_HASH_KEY_LENGTH 40
#define HEADER_TEMPLATE_SIZE    42
//-------------------------------------------------
//-----------------------user begin---------------------
#define IP_LOCAL                "10.0.0.7"
#define MAC_LOCAL               {0x98, 0x03, 0x9b, 0xca, 0x40, 0x18} //amax 1
#define CLIENT_PORT             1111
#define HC_MASTER               0
#define HC_SLAVE_NUM            4//2
char* ip_dst_hc_arr[HC_SLAVE_NUM] = {
    "10.0.0.2",
    "10.0.0.4",
    "10.0.0.7",
    "10.0.0.8"
};
struct rte_ether_addr mac_dst_hc_arr[HC_SLAVE_NUM] = {
    {.addr_bytes = {0x98, 0x03, 0x9b, 0xca, 0x48, 0x38}},
    {.addr_bytes = {0x98, 0x03, 0x9b, 0xc7, 0xc0, 0xa8}},
    {.addr_bytes = {0x98, 0x03, 0x9b, 0xca, 0x40, 0x18}},
    {.addr_bytes = {0x0c, 0x42, 0xa1, 0x2b, 0x0d, 0x70}}
};
#define CLIENT_ID_BIAS 0

#define IP_FPGA                 "10.0.0.7"//"10.0.0.235""10.0.0.6"//
#define MAC_FPGA                {0x98, 0x03, 0x9b, 0xca, 0x40, 0x18} //{0x98, 0x03, 0x9b, 0xca, 0x40, 0x18} //fpga{0x98, 0x03, 0x9b, 0xc7, 0xc7, 0xfc}//
#define SERVICE_PORT            5001 
#define HC_PORT                 0x23
#define OP_HC                   0x23

volatile int hc_flag = 0;

#define CONTROLLER_IP           "10.201.124.31"
#define CONTROLLER_MAC          {0x98, 0x03, 0x9b, 0xc7, 0xc7, 0xfc}
#define CONTROLLER_PORT         8890
#define CLIENT_MASTER_IP        "127.0.0.1"
#define CLIENT_MASTER_PORT      8891
//-----------------------end-----------------------
//-------------------------------------------------
#define RSS_HASH_KEY_LENGTH 40
#define HEADER_TEMPLATE_SIZE         42

char role = ROLE_CLIENT;
uint32_t enabled_port_mask = 1;
uint16_t lcore_id_2_rx_queue_id[NC_MAX_LCORES];

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
//----------------------------------------------------- KVS -----------------------------------------------------
#define OP_GET                  1		
#define OP_DEL                  2		
#define OP_PUT                  3
#define OP_PUT_SWITCHHIT        4
#define OP_PUT_SWITCHMISS	    5
#define OP_GETSUCC              6		
#define OP_GETFAIL              7		
#define OP_DELSUCC              8		
#define OP_DELFAIL              9		
#define OP_PUTSUCC_SWITCHHIT    10
#define OP_PUTSUCC_SWITCHMISS   11
#define OP_PUTFAIL              12
#define OP_GET_RESEND           13

#define KEY_BYTES               16
#define VALUE_BYTES             128

// BKDR Hash Function
uint32_t BKDRHash(char *str, uint32_t len)
{
    uint32_t seed = 131; // 31 131 1313 13131 131313 etc..
    uint32_t hash = 0;
 
    for(int i=0; i<len; i++){
        hash = hash * seed + (*str++);        
    }
    // hash = *str;
    // while (*str)
    // {
    //     hash = hash * seed + (*str++);
    // }
    return (hash & 0x7FFFFF); //哈希出的最终范�??0-8388607
}


//----------------------------------------------------- ctrl -----------------------------------------------------
//----------------------------opType for ctrl-----------------------------

/*
 * custom types
 */

typedef struct  MessageHeader_ {
    uint8_t     opType;
    unsigned char        key[KEY_BYTES];
    uint8_t     valueLen;
    unsigned char        value[VALUE_BYTES];
} __attribute__((__packed__)) MessageHeader;

typedef struct  MessageHeader_S_ {
    uint8_t     opType;
    unsigned char        key[KEY_BYTES];
    unsigned char        padding[47];         
} __attribute__((__packed__)) MessageHeader_S;

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

struct throughput_statistics {
    uint64_t tx;
    uint64_t rx;
    uint64_t tx_normal;
    uint64_t tx_resend;
    uint64_t txn;
    uint64_t dropped;
    uint64_t last_tx;
    uint64_t last_rx;
    uint64_t last_tx_normal;
    uint64_t last_tx_resend;
    uint64_t last_dropped;
    uint64_t last_txn;
    uint64_t last_txncount;
    uint64_t txncount;
    uint64_t last_deadlock_txncount;
    uint64_t deadlock_txncount;
    uint64_t last_rx_fail;
    uint64_t rx_fail;
    uint64_t last_rx_succ;
    uint64_t rx_succ;
    uint64_t last_rx_enq_succ;
    uint64_t rx_enq_succ;
    uint64_t last_rx_mod_fail;
    uint64_t rx_mod_fail;
} __rte_cache_aligned;

/*
 * global variables
 */

uint32_t num_worker = 9;
uint32_t enabled_ports[RTE_MAX_ETHPORTS];
uint32_t n_enabled_ports = 0;
uint32_t n_rx_queues = 0;
uint32_t n_lcores = 0;

struct rte_mempool *pktmbuf_pool = NULL;
struct rte_mempool *pktmbuf_pool_ctrl = NULL;
struct rte_ether_addr port_eth_addrs[RTE_MAX_ETHPORTS];
struct lcore_configuration lcore_conf[NC_MAX_LCORES];
struct throughput_statistics tput_stat[NC_MAX_LCORES];
struct throughput_statistics tput_stat_avg[NC_MAX_LCORES];
struct throughput_statistics tput_stat_total;

// uint64_t txn_rate_stat[NC_MAX_LCORES];

// uint8_t header_template[
//     sizeof(struct rte_ether_hdr)
//     + sizeof(struct rte_ipv4_hdr)
//     + sizeof(struct rte_udp_hdr)];
uint8_t header_template[HEADER_TEMPLATE_SIZE];

/*
 * functions for generation
 */

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
    tput_stat[lcore_id].tx += ret;
    if (unlikely(ret < n)) {
        tput_stat[lcore_id].dropped += (n - ret);
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
    if (unlikely(lconf->tx_mbufs.len >= thres)) {
        send_pkt_burst(lcore_id);
    }
}

/*
 * functions for initialization
 */

// init header template TODO: dst_addr需要修改成server的MAC地址
static void init_header_template_local(uint8_t header_template_local[HEADER_TEMPLATE_SIZE]) {
    memset(header_template_local, 0, HEADER_TEMPLATE_SIZE);
    struct rte_ether_hdr *eth = (struct rte_ether_hdr *)header_template_local;
    struct rte_ipv4_hdr *ip = (struct rte_ipv4_hdr *)((uint8_t*) eth + sizeof(struct rte_ether_hdr));
    struct rte_udp_hdr *udp = (struct rte_udp_hdr *)((uint8_t*)ip + sizeof(struct rte_ipv4_hdr));
    uint32_t pkt_len = HEADER_TEMPLATE_SIZE + sizeof(MessageHeader);

    // eth header
    eth->ether_type = rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4);
    // eth->ether_type = htons(ether_type_response);    
    struct rte_ether_addr src_addr = {.addr_bytes = MAC_LOCAL}; 
    struct rte_ether_addr dst_addr = {.addr_bytes = MAC_FPGA}; 
    rte_ether_addr_copy(&src_addr, &eth->s_addr);
    rte_ether_addr_copy(&dst_addr, &eth->d_addr);

    // ip header
    char src_ip[] = IP_LOCAL;
    char dst_ip[] = IP_FPGA;
    int st1 = inet_pton(AF_INET, src_ip, &(ip->src_addr));
    int st2 = inet_pton(AF_INET, dst_ip, &(ip->dst_addr));
    if(st1 != 1 || st2 != 1) {
        fprintf(stderr, "inet_pton() failed.Error message: %s %s",
            strerror(st1), strerror(st2));
        exit(EXIT_FAILURE);
    }
    ip->total_length = rte_cpu_to_be_16(pkt_len - sizeof(struct rte_ether_hdr));
    ip->version_ihl = 0x45;
    ip->type_of_service = 0;
    ip->packet_id = 0;
    ip->fragment_offset = 0;
    ip->time_to_live = 64;
    ip->next_proto_id = IPPROTO_UDP;
    uint32_t ip_cksum;
    uint16_t *ptr16 = (uint16_t *)ip;
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
    udp->src_port = htons(CLIENT_PORT);
    udp->dst_port = htons(SERVICE_PORT);
    udp->dgram_len = rte_cpu_to_be_16(pkt_len
        - sizeof(struct rte_ether_hdr)
        - sizeof(struct rte_ipv4_hdr));
    udp->dgram_cksum = 0;
}

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
    struct rte_ether_addr src_addr = {.addr_bytes = CONTROLLER_MAC};

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

struct rte_flow *generate_udp_flow(uint16_t port_phys, uint16_t port_udp, uint16_t rx_q, struct rte_flow_error *error){
    struct rte_flow_attr attr;
    struct rte_flow_item pattern[MAX_PATTERN_NUM];
    struct rte_flow_action action[MAX_ACTION_NUM];
    struct rte_flow *flow = NULL;
    struct rte_flow_action_queue queue = { .index = rx_q };
    struct rte_flow_item_udp udp_spec;
    struct rte_flow_item_udp udp_mask;
    int res;

    printf("check: rx_q = %d, port_udp = %d\n", rx_q, port_udp);
    /* set the rule attribute. in this case only ingress packets will be checked. */
	memset(pattern, 0, sizeof(pattern));
	memset(action, 0, sizeof(action));
	memset(&attr, 0, sizeof(struct rte_flow_attr));    
    attr.ingress = 1;
    attr.priority = 1;
    /* create the action sequence. one action only,  move packet to queue */
    action[0].type = RTE_FLOW_ACTION_TYPE_QUEUE;//choose a queue
    action[0].conf = &queue;
    action[1].type = RTE_FLOW_ACTION_TYPE_END; //A list of actions is terminated by a END action.
    printf("check point 1.\n");
    /* set the first level of the pattern (ETH). since in this example we just want to get the ipv4 we set this level to allow all. */
	pattern[0].type = RTE_FLOW_ITEM_TYPE_ETH; // need to build correct stack
	pattern[1].type = RTE_FLOW_ITEM_TYPE_IPV4;
	pattern[2].type = RTE_FLOW_ITEM_TYPE_UDP;
	pattern[3].type = RTE_FLOW_ITEM_TYPE_END;
    printf("check point 2.\n");
    // /*
    //  * setting the second level of the pattern (IP). in this example this is the level we care about so we set it according to the parameters.
    //  */
    memset(&udp_spec, 0, sizeof(struct rte_flow_item_udp));
    memset(&udp_mask, 0, sizeof(struct rte_flow_item_udp));
    udp_spec.hdr.dst_port = htons(port_udp); 
    udp_mask.hdr.dst_port = 0xffff; 
    pattern[2].spec = &udp_spec;
    pattern[2].mask = &udp_mask;

    printf("check point 3.\n");
	res = rte_flow_validate(port_phys, &attr, pattern, action, error);
	if (!res)
		flow = rte_flow_create(port_phys, &attr, pattern, action, error);
    printf("check point 4.\n");
	return flow;


    // struct rte_flow_attr attr;
	// struct rte_flow_item pattern[MAX_PATTERN_NUM];
	// struct rte_flow_action action[MAX_ACTION_NUM];
	// struct rte_flow *flow = NULL;
	// struct rte_flow_action_queue queue = { .index = rx_q };
	// struct rte_flow_item_ipv4 ip_spec;
	// struct rte_flow_item_ipv4 ip_mask;
	// int res;

	// memset(pattern, 0, sizeof(pattern));
	// memset(action, 0, sizeof(action));
	// memset(&attr, 0, sizeof(struct rte_flow_attr));
	// attr.ingress = 1;
	// action[0].type = RTE_FLOW_ACTION_TYPE_QUEUE;
	// action[0].conf = &queue;
	// action[1].type = RTE_FLOW_ACTION_TYPE_END;
	// pattern[0].type = RTE_FLOW_ITEM_TYPE_ETH;
	// // memset(&ip_spec, 0, sizeof(struct rte_flow_item_ipv4));
	// // memset(&ip_mask, 0, sizeof(struct rte_flow_item_ipv4));
	// // ip_spec.hdr.dst_addr = 0;
	// // ip_mask.hdr.dst_addr = 0;
	// // ip_spec.hdr.src_addr = 0;
	// // ip_mask.hdr.src_addr = 0;
	// pattern[1].type = RTE_FLOW_ITEM_TYPE_IPV4;
	// // pattern[1].spec = &ip_spec;
	// // pattern[1].mask = &ip_mask;
	// pattern[2].type = RTE_FLOW_ITEM_TYPE_END;
    // printf("check point 3.\n");
	// res = rte_flow_validate(port_phys, &attr, pattern, action, error);
	// if (!res)
	// 	flow = rte_flow_create(port_phys, &attr, pattern, action, error);
    // printf("check point 4.\n");
	// return flow;
}


// initialize all status
static void nc_init(void) {
    uint32_t i, j;

    // create mbuf pool
    printf("create mbuf pool\n");
    pktmbuf_pool = rte_mempool_create(
        "mbuf_pool",
        NC_NB_MBUF,
        NC_MBUF_SIZE,
        NC_MBUF_CACHE_SIZE,
        sizeof(struct rte_pktmbuf_pool_private),
        rte_pktmbuf_pool_init, NULL,
        rte_pktmbuf_init, NULL,
        rte_socket_id(),
        0);
    pktmbuf_pool_ctrl = rte_mempool_create(
        "mbuf_pool_ctrl",
        NC_NB_MBUF,
        NC_MBUF_SIZE,
        NC_MBUF_CACHE_SIZE,
        sizeof(struct rte_pktmbuf_pool_private),
        rte_pktmbuf_pool_init, NULL,
        rte_pktmbuf_init, NULL,
        rte_socket_id(),
        0);
    if (pktmbuf_pool == NULL) {
        rte_exit(EXIT_FAILURE, "cannot init mbuf pool\n");
    }
    printf("rte_socket_id = %d", rte_socket_id());

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
            lcore_id_2_rx_queue_id[i] = rx_queue_id;
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
        ret = rte_eth_rx_queue_setup(portid, 0, NC_NB_RXD+1024,
                rte_eth_dev_socket_id(portid), NULL, pktmbuf_pool_ctrl);
        for (i = 1; i < rx_queues_per_port; i++) {
            ret = rte_eth_rx_queue_setup(portid, i, NC_NB_RXD,
                rte_eth_dev_socket_id(portid), NULL, pktmbuf_pool);
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
        //rte_flow
        struct rte_flow_error error;
        for(i = 1; i < rx_queues_per_port; i++){
            printf("i + CLIENT_PORT = %d\n", i + 5002 - 1);
            struct rte_flow * flow = generate_udp_flow(portid, i + 5002 - 1, i / NC_RX_QUEUE_PER_LCORE, &error); // udpport,queue,error
            if (!flow) {
                printf("Flow can't be created %d message: %s\n", error.type, error.message ? error.message : "(no stated reason)");
                rte_exit(EXIT_FAILURE, "error in creating flow");
            } 
        }
        // struct rte_flow * flow = generate_eth_flow(portid, 0, 0, &error);
        // if (!flow) {
        //     printf("Flow can't be created %d message: %s\n", error.type, error.message ? error.message : "(no stated reason)");
        //     rte_exit(EXIT_FAILURE, "error in creating flow");
        // } 
    }

    if (!n_enabled_ports) {
        rte_exit(EXIT_FAILURE, "all available ports are disabled. Please set portmask.\n");
    }
    check_link_status();
    // init_header_template();
}

/*
 * functions for print
 */

// print per-core throughput
// static void print_per_core_throughput(void) {
//     // time is in second
//     printf("%lld\nthroughput\n", (long long)time(NULL));
//     uint32_t i, j;
//     uint32_t core_0_arr[] = {0,1,2,3,4,5,6,7,8,9,10,11, \
//                             24,25,26,27,28,29,30,31,32,33,34,35, \
//                             12,13,14,15,16,17,18,19,20,21,22,23, \
//                             36,37,38,39,40,41,42,43,44,45,46,47};
    

//     fflush(stdout);

//     float deadlock_count = 0;
//     float rx_count = 0;
//     float rx_succ_count = 0;
//     float rx_enq_succ_count = 0;
//     float rx_mod_fail_count = 0;
//     float txn_count = 0;

//     for (j = 0; j < n_lcores; j++) {
//         i = core_0_arr[j];
//         printf(" core %"PRIu32"  "
//             "tx: %"PRIu64"  "
//             "rx: %"PRIu64"  "
//             "tx_normal: %"PRIu64"  "
//             "tx_resend: %"PRIu64"  "
//             "dropped: %"PRIu64"  "
//             "deadlock txn: %"PRIu64"  "
//             "txn: %"PRIu64"  "
//             "rx_fail: %"PRIu64"  "
//             "rx_mod_fail: %"PRIu64"  "
//             "rx_enq_succ: %"PRIu64"  "
//             "rx_succ: %"PRIu64"\n",
//             i, 
//             tput_stat[i].tx       - tput_stat[i].last_tx      ,
//             tput_stat[i].rx       - tput_stat[i].last_rx      ,
//             tput_stat[i].tx_normal  - tput_stat[i].last_tx_normal ,
//             tput_stat[i].tx_resend - tput_stat[i].last_tx_resend,
//             tput_stat[i].dropped  - tput_stat[i].last_dropped ,
//             tput_stat[i].deadlock_txncount - tput_stat[i].last_deadlock_txncount,
//             tput_stat[i].txncount - tput_stat[i].last_txncount,
//             tput_stat[i].rx_fail - tput_stat[i].last_rx_fail,
//             tput_stat[i].rx_mod_fail - tput_stat[i].last_rx_mod_fail,
//             tput_stat[i].rx_enq_succ - tput_stat[i].last_rx_enq_succ,
//             tput_stat[i].rx_succ - tput_stat[i].last_rx_succ
//             );

//         deadlock_count += tput_stat[i].deadlock_txncount - tput_stat[i].last_deadlock_txncount;
//         rx_count += tput_stat[i].rx       - tput_stat[i].last_rx;
//         rx_succ_count += tput_stat[i].rx_succ       - tput_stat[i].last_rx_succ;
//         rx_enq_succ_count += tput_stat[i].rx_enq_succ       - tput_stat[i].last_rx_enq_succ;
//         rx_mod_fail_count += tput_stat[i].rx_mod_fail       - tput_stat[i].last_rx_mod_fail;
//         txn_count += tput_stat[i].txncount - tput_stat[i].last_txncount;

//         tput_stat[i].last_tx        = tput_stat[i].tx      ;
//         tput_stat[i].last_rx        = tput_stat[i].rx      ;
//         tput_stat[i].last_tx_normal   = tput_stat[i].tx_normal ;
//         tput_stat[i].last_tx_resend  = tput_stat[i].tx_resend;
//         tput_stat[i].last_dropped   = tput_stat[i].dropped ;
//         tput_stat[i].last_deadlock_txncount = tput_stat[i].deadlock_txncount;
//         tput_stat[i].last_txncount = tput_stat[i].txncount;
//         tput_stat[i].last_rx_fail = tput_stat[i].rx_fail;
//         tput_stat[i].last_rx_succ = tput_stat[i].rx_succ;
//         tput_stat[i].last_rx_enq_succ = tput_stat[i].rx_enq_succ;
//         tput_stat[i].last_rx_mod_fail = tput_stat[i].rx_mod_fail;
//     }
//     printf("deadlock ratio: %.2f, rx rate: %.2fMrps, rx_succ rate: %.2fMrps, rx_enq_succ rate: %.2fMrps, txn rate: %.2fMrps\n", deadlock_count / txn_count, rx_count / 1000000, rx_succ_count / 1000000, rx_enq_succ_count / 1000000, txn_count / 1000000);
// }

static void print_per_core_throughput(void) {
    // time is in second
    printf("%lld\nthroughput\n", (long long)time(NULL));
    uint32_t i, j;
    uint32_t core_0_arr[24] = {0,1,2,3,4,5,6,7,8,9,10,11, \
                            24,25,26,27,28,29,30,31,32,33,34,35};
    

    fflush(stdout);

    float deadlock_count = 0;
    float rx_count = 0;
    float rx_succ_count = 0;
    float rx_enq_succ_count = 0;
    float rx_mod_fail_count = 0;
    float txn_count = 0;
    float tx_normal = 0;

    for (j = 0; j < n_lcores; j++) {
        i = core_0_arr[j];
        // printf(" core %"PRIu32"  "
        //     "tx: %"PRIu64"  "
        //     "rx: %"PRIu64"  "
        //     "tx_normal: %"PRIu64"  "
        //     "tx_resend: %"PRIu64"  "
        //     "dropped: %"PRIu64"  "
        //     "deadlock txn: %"PRIu64"  "
        //     "txn: %"PRIu64"  "
        //     "rx_fail: %"PRIu64"  "
        //     "rx_mod_fail: %"PRIu64"  "
        //     "rx_enq_succ: %"PRIu64"  "
        //     "rx_succ: %"PRIu64"\n",
        //     i, 
        //     tput_stat[i].tx       - tput_stat[i].last_tx      ,
        //     tput_stat[i].rx       - tput_stat[i].last_rx      ,
        //     tput_stat[i].tx_normal  - tput_stat[i].last_tx_normal ,
        //     tput_stat[i].tx_resend - tput_stat[i].last_tx_resend,
        //     tput_stat[i].dropped  - tput_stat[i].last_dropped ,
        //     tput_stat[i].deadlock_txncount - tput_stat[i].last_deadlock_txncount,
        //     tput_stat[i].txncount - tput_stat[i].last_txncount,
        //     tput_stat[i].rx_fail - tput_stat[i].last_rx_fail,
        //     tput_stat[i].rx_mod_fail - tput_stat[i].last_rx_mod_fail,
        //     tput_stat[i].rx_enq_succ - tput_stat[i].last_rx_enq_succ,
        //     tput_stat[i].rx_succ - tput_stat[i].last_rx_succ
        //     );
        // printf("tx_normal[%d]: %.2fMrps\n", i, 1.0 * (tput_stat[i].tx_normal - tput_stat[i].last_tx_normal) / 1000000);

        deadlock_count += tput_stat[i].deadlock_txncount - tput_stat[i].last_deadlock_txncount;
        rx_count += tput_stat[i].rx       - tput_stat[i].last_rx;
        rx_succ_count += tput_stat[i].rx_succ       - tput_stat[i].last_rx_succ;
        rx_enq_succ_count += tput_stat[i].rx_enq_succ       - tput_stat[i].last_rx_enq_succ;
        rx_mod_fail_count += tput_stat[i].rx_mod_fail       - tput_stat[i].last_rx_mod_fail;
        txn_count += tput_stat[i].txncount - tput_stat[i].last_txncount;
        tx_normal += tput_stat[i].tx_normal       - tput_stat[i].last_tx_normal;

        tput_stat[i].last_tx        = tput_stat[i].tx      ;
        tput_stat[i].last_rx        = tput_stat[i].rx      ;
        tput_stat[i].last_tx_normal   = tput_stat[i].tx_normal ;
        tput_stat[i].last_tx_resend  = tput_stat[i].tx_resend;
        tput_stat[i].last_dropped   = tput_stat[i].dropped ;
        tput_stat[i].last_deadlock_txncount = tput_stat[i].deadlock_txncount;
        tput_stat[i].last_txncount = tput_stat[i].txncount;
        tput_stat[i].last_rx_fail = tput_stat[i].rx_fail;
        tput_stat[i].last_rx_succ = tput_stat[i].rx_succ;
        tput_stat[i].last_rx_enq_succ = tput_stat[i].rx_enq_succ;
        tput_stat[i].last_rx_mod_fail = tput_stat[i].rx_mod_fail;
    }
    printf("tx_normal: %.2fMrps\n", tx_normal / 1000000);
}




//-----------------------------------------------------------------------------
struct time_statistics {
    uint64_t accu_time;
    uint64_t last_accu_time;
    uint64_t query_count;
    uint64_t last_query_count;
} __rte_cache_aligned;

struct time_statistics accu_time_stat[NC_MAX_LCORES];

static void print_statistical_time(void) {
    uint32_t i, j;
    uint32_t core_0_arr[24] = {0,1,2,3,4,5,6,7,8,9,10,11, \
                            24,25,26,27,28,29,30,31,32,33,34,35};
    fflush(stdout);

    uint64_t us_tsc = rte_get_tsc_hz() / 1000000; 
    float accu_time_allthreads = 0;
    float query_count_allthreads = 0;
    for (j = 0; j < n_lcores; j++) {
        i = core_0_arr[j];
        accu_time_allthreads += accu_time_stat[i].accu_time - accu_time_stat[i].last_accu_time;
        query_count_allthreads += accu_time_stat[i].query_count - accu_time_stat[i].last_query_count;
        accu_time_stat[i].last_accu_time = accu_time_stat[i].accu_time;
        accu_time_stat[i].last_query_count = accu_time_stat[i].query_count;
    }
    float accu_time_allthreads_alltime = 0;
    float query_count_allthreads_alltime = 0;
    for (j = 0; j < n_lcores; j++) {
        i = core_0_arr[j];
        accu_time_allthreads_alltime += accu_time_stat[i].accu_time;
        query_count_allthreads_alltime += accu_time_stat[i].query_count;
    }
    // printf("accu_time_allthreads: %.2f, accu_time_allthreads_alltime: %.2f, query_count_allthreads:%.2f, query_count_allthreads_alltime:%.2f\n", accu_time_allthreads, accu_time_allthreads_alltime, query_count_allthreads, query_count_allthreads_alltime);
    printf("accu_time_allthreads_per_query: %.2fus, accu_time_allthreads_alltime_per_query: %.2fus\n", accu_time_allthreads / query_count_allthreads / us_tsc, accu_time_allthreads_alltime / query_count_allthreads_alltime / us_tsc);
}