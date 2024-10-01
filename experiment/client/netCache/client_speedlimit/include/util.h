
#ifndef NETCACHE_UTIL_H
#define NETCACHE_UTIL_H
#define NETLOCK_MODE
#ifdef DEBUG
#define DEBUG_PRINT(...) do{ fprintf( stderr, __VA_ARGS__ ); } while( 0 )
#else
#define DEBUG_PRINT(...) do{ } while ( 0 )
#endif
#define FAILURE_ROLLBACK        0
#define FAILURE_STOP            1
#define FAILURE_NORMAL          2
#define MEM_BIN_PACK            'b'
#define MEM_RAND_WEIGHT         'w'
#define MEM_RAND_12             '1'
#define MEM_RAND_200            '2'
#define PRIMARY_BACKUP          1
#define SECONDARY_BACKUP        2
#define FAILURE_NOTIFICATION    3
#define MAX_CLIENT_NUM          8
#define SYNCHRONOUS             's'
#define ASYNCHRONOUS            'a'
#define SAMPLE_SIZE             10000
#define RING_SIZE               16384
#define MAX_PKTS_BURST          32
#define MICROBENCHMARK_SHARED   's'
#define MICROBENCHMARK_EXCLUSIVE 'x'
#define NORMALBENCHMARK         'n'
#define ZIPFBENCHAMRK           'z'
#define UNIFORMBENCHMARK        'u'
#define TPCC_UNIFORMBENCHMARK   'v'
#define TPCCBENCHMARK           't'
#define SHARED_LOCK             0x00
#define EXCLUSIVE_LOCK          0x01
#define MAX_TXN_NUM             1000000
#define MAX_LOCK_NUM            7010000 
#define MAX_LOCK_NUM_IN_SWITCH  60000
#define LEN_ONE_SEGMENT         100
char benchmark = MICROBENCHMARK_SHARED;
char task_id = 's';
uint32_t *batch_map = NULL;
int locks_each_txn[MAX_CLIENT_NUM][MAX_TXN_NUM] = {0};
#define BIN_SIZE                100
#define BIN_RANGE               10
#define BIN_MAX                 (BIN_SIZE * BIN_RANGE)
//-------------------------------------------------
#define ROLE_CLIENT             'c'
#define ROLE_SERVER_POOL        's'

#define NC_MAX_PAYLOAD_SIZE     1500
#define NC_NB_MBUF              8192 //2048
#define NC_MBUF_SIZE            (2048+sizeof(struct rte_mbuf)+RTE_PKTMBUF_HEADROOM)
#define NC_MBUF_CACHE_SIZE      32
#define NC_MAX_BURST_SIZE       32
#define NC_MAX_LCORES           64
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
#define IP_LOCAL                "10.0.0.1"
#define MAC_LOCAL               {0xb8, 0x59, 0x9f, 0xe9, 0x6b, 0x1c} //amax 1
#define CLIENT_PORT             1111
#define HC_MASTER               1
#define HC_SLAVE_NUM            1//4//2
char* ip_dst_hc_arr[HC_SLAVE_NUM] = {
    "10.0.0.2"
    // "10.0.0.3",
    // "10.0.0.5",
    // "10.0.0.8"
};
struct rte_ether_addr mac_dst_hc_arr[HC_SLAVE_NUM] = {
    {.addr_bytes = {0x98, 0x03, 0x9b, 0xca, 0x48, 0x38}}
    // {.addr_bytes = {0x98, 0x03, 0x9b, 0xc7, 0xc8, 0x18}},
    // {.addr_bytes = {0x04, 0x3f, 0x72, 0xde, 0xba, 0x44}},
    // {.addr_bytes = {0x0c, 0x42, 0xa1, 0x2b, 0x0d, 0x70}}
};
#define CLIENT_ID_BIAS 0

#define BN_NUM            1
char* ip_dst_arr[BN_NUM] = {
    "10.0.0.4"
};
struct rte_ether_addr mac_dst_arr[BN_NUM] = {
    {.addr_bytes = {0x98, 0x03, 0x9b, 0xc7, 0xc0, 0xa8}}
};

// #define BN_NUM            2
// char* ip_dst_arr[BN_NUM] = {
//     "10.0.0.7",
//     "10.0.0.8"
// };
// struct rte_ether_addr mac_dst_arr[BN_NUM] = {
//     {.addr_bytes = {0x98, 0x03, 0x9b, 0xca, 0x40, 0x18}},
//     {.addr_bytes = {0x0c, 0x42, 0xa1, 0x2b, 0x0d, 0x70}}
// };

#define IP_FPGA                 "10.0.0.2"//
#define MAC_FPGA                {0x98, 0x03, 0x9b, 0xca, 0x48, 0x38}//
#define SERVICE_PORT            5001
#define HC_PORT                 0x23
#define OP_HC                   0x23
uint32_t n_rcv_cores = 1; 
volatile int hc_flag = 0;
// volatile int hc_flag = -2;
#define APP_KVS 1
//-----------------------end-----------------------
//-------------------------------------------------


char role = ROLE_CLIENT;
uint32_t enabled_port_mask = 1;

int clientTotalNum = 0;
int clientInLcoreNum = 0;
int lockInTxnNum = 0;
int lockInSwitchValidNum = 0;

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

#define KEY_BYTES               16
#define VALUE_BYTES             16

typedef struct  MessageHeader_ {
    uint8_t     opType;
    uint8_t     lockMode;
    uint16_t    clientID;
    uint32_t    lockID;
    uint32_t    txnID;
    uint64_t    timestamp;
    uint16_t    padding;
} __attribute__((__packed__)) MessageHeader;

typedef struct  MessageHeader_KVSS {
    uint8_t     opType;
    unsigned char        key[KEY_BYTES];
} __attribute__((__packed__)) MessageHeader_KVSS;

// typedef struct  MessageHeader_KVSS {
//     uint8_t     opType;
//     unsigned char        key[KEY_BYTES];
//     uint8_t     padding_8b;
//     uint32_t    padding_32b;
//     uint64_t    padding_64b;
// } __attribute__((__packed__)) MessageHeader_KVSS;

typedef struct  MessageHeader_KVSL {
    uint8_t     opType;
    uint8_t     keyLen;
    unsigned char        key[KEY_BYTES];
    uint8_t     valueLen;
    uint64_t    value[VALUE_BYTES];
} __attribute__((__packed__)) MessageHeader_KVSL;

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
    uint64_t tx                     ; 
    uint64_t tx_grant               ; 
    uint64_t tx_grant_re4wait       ; 
    uint64_t tx_release             ; 
    uint64_t tx_release_re4modfail  ; 
    uint64_t tx_release_re4wait     ; 
    uint64_t rx                     ; //
    uint64_t rx_grantSucc           ; //
    uint64_t rx_queueSucc           ; //
    uint64_t rx_releaseSucc         ; //
    uint64_t rx_grantFail           ; //
    uint64_t rx_grantModFail        ; //
    uint64_t rx_releaseFail         ; //
    uint64_t rx_releaseModFail      ; //
    uint64_t rx_p4                  ; //
    uint64_t rx_grantSucc_p4        ; //
    uint64_t rx_queueSucc_p4        ; //
    uint64_t rx_releaseSucc_p4      ; //
    uint64_t rx_grantFail_p4        ; //
    uint64_t rx_releaseFail_p4      ; //
    uint64_t rx_bdFail_p4           ; //
    uint64_t rx_mirror_p4           ; //
    uint64_t rx_fpga                ; //
    uint64_t rx_grantSucc_fpga      ; //
    uint64_t rx_queueSucc_fpga      ; //
    uint64_t rx_releaseSucc_fpga    ; //
    uint64_t rx_grantFail_fpga      ; //
    uint64_t rx_grantModFail_fpga   ; //
    uint64_t rx_grantModHitFail_fpga   ; //
    uint64_t rx_grantModNfsFail_fpga   ; //
    uint64_t rx_releaseFail_fpga    ; //     
    uint64_t rx_releaseModFail_fpga ; // 
    uint64_t rx_releaseModHitFail_fpga ; // 
    uint64_t rx_releaseModNfsFail_fpga ; // 
    uint64_t dropped_pkt            ; 
    uint64_t txn_fail               ; 
    uint64_t txn_total              ; 
    uint64_t last_tx                   ; 
    uint64_t last_tx_grant             ; 
    uint64_t last_tx_grant_re4wait     ; 
    uint64_t last_tx_release           ; 
    uint64_t last_tx_release_re4modfail; 
    uint64_t last_tx_release_re4wait   ; 
    uint64_t last_rx                   ; 
    uint64_t last_rx_grantSucc         ; 
    uint64_t last_rx_queueSucc         ; 
    uint64_t last_rx_releaseSucc       ; 
    uint64_t last_rx_grantFail         ;
    uint64_t last_rx_grantModFail      ; 
    uint64_t last_rx_releaseFail       ; 
    uint64_t last_rx_releaseModFail    ; 
    uint64_t last_rx_p4                ; 
    uint64_t last_rx_grantSucc_p4      ; 
    uint64_t last_rx_queueSucc_p4      ; 
    uint64_t last_rx_releaseSucc_p4    ; 
    uint64_t last_rx_grantFail_p4      ; 
    uint64_t last_rx_releaseFail_p4    ; 
    uint64_t last_rx_bdFail_p4         ;
    uint64_t last_rx_mirror_p4         ;
    uint64_t last_rx_fpga              ; 
    uint64_t last_rx_grantSucc_fpga    ; 
    uint64_t last_rx_queueSucc_fpga    ; 
    uint64_t last_rx_releaseSucc_fpga  ; 
    uint64_t last_rx_grantFail_fpga    ;
    uint64_t last_rx_grantModFail_fpga ; 
    uint64_t last_rx_grantModHitFail_fpga   ; //
    uint64_t last_rx_grantModNfsFail_fpga   ; //
    uint64_t last_rx_releaseFail_fpga  ;    
    uint64_t last_rx_releaseModFail_fpga;  
    uint64_t last_rx_releaseModHitFail_fpga ; // 
    uint64_t last_rx_releaseModNfsFail_fpga ; // 
    uint64_t last_dropped_pkt          ; 
    uint64_t last_txn_fail             ; 
    uint64_t last_txn_total            ; 

    uint64_t tx_get               ;
    uint64_t tx_put               ;
    uint64_t tx_del               ;
    uint64_t rx_getSucc          ;
    uint64_t rx_putSucc          ;
    uint64_t rx_delSucc          ;
    uint64_t rx_getFail          ;
    uint64_t rx_putFail          ;
    uint64_t rx_delFail          ;
    uint64_t last_tx_get               ;
    uint64_t last_tx_put               ;
    uint64_t last_tx_del               ;
    uint64_t last_rx_getSucc          ;
    uint64_t last_rx_putSucc          ;
    uint64_t last_rx_delSucc          ;
    uint64_t last_rx_getFail          ;
    uint64_t last_rx_putFail          ;
    uint64_t last_rx_delFail          ;

} __rte_cache_aligned;
/*
 * global variables
 */

uint32_t num_worker = 9;
uint32_t enabled_ports[RTE_MAX_ETHPORTS];
uint32_t n_enabled_ports = 0;
uint32_t n_rx_queues = 0;
uint32_t n_lcores = 0;

struct rte_mempool *pktmbuf_pool[NC_MAX_LCORES];
struct rte_ether_addr port_eth_addrs[RTE_MAX_ETHPORTS];
struct lcore_configuration lcore_conf[NC_MAX_LCORES];
struct throughput_statistics tput_stat[NC_MAX_LCORES];
struct throughput_statistics tput_stat_avg[NC_MAX_LCORES];
struct throughput_statistics tput_stat_total;
struct throughput_statistics tput_stat_time[800];

// uint64_t txn_rate_stat[NC_MAX_LCORES];

// uint8_t header_template[
//     sizeof(struct rte_ether_hdr)
//     + sizeof(struct rte_ipv4_hdr)
//     + sizeof(struct rte_udp_hdr)];
uint8_t header_template[HEADER_TEMPLATE_SIZE];
/*
 * display memmory block
 */
void mem_display(const void *address, int len) {
    const unsigned char *p = address;
    DEBUG_PRINT("MEMORY DUMP\n");
    for (size_t i = 0; i < len; i++) {
        DEBUG_PRINT("%02hhx", p[i]);
        if (i % 2 == 1)
            DEBUG_PRINT(" ");
        if (i % 16 == 15)
            DEBUG_PRINT("\n");
    }
    if (len % 16 != 0)
        DEBUG_PRINT("\n");
}

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
    // tput_stat[lcore_id].tx += ret;
    if (unlikely(ret < n)) {
        tput_stat[lcore_id].dropped_pkt += (n - ret);
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

/*
 * functions for initialization
 */

// init header template
static void init_header_template_local(uint8_t header_template_local[HEADER_TEMPLATE_SIZE]) {
    memset(header_template_local, 0, HEADER_TEMPLATE_SIZE);
    struct rte_ether_hdr *eth = (struct rte_ether_hdr *)header_template_local;
    struct rte_ipv4_hdr *ip = (struct rte_ipv4_hdr *)((uint8_t*) eth + sizeof(struct rte_ether_hdr));
    struct rte_udp_hdr *udp = (struct rte_udp_hdr *)((uint8_t*)ip + sizeof(struct rte_ipv4_hdr));
    uint32_t pkt_len = HEADER_TEMPLATE_SIZE + sizeof(MessageHeader);

    // eth header
    eth->ether_type = rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4);//rte_cpu_to_be_16(0x2331);//rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4);
//-------------------------------------------------
//-----------------------begin---------------------
    struct rte_ether_addr src_addr = {
        // .addr_bytes = {0x98, 0x03, 0x9b, 0xca, 0x40, 0x18}}; //amax 7
        // .addr_bytes = {0x04, 0x3f, 0x72, 0xde, 0xba, 0x44}}; //amax 8
        // .addr_bytes = {0x98, 0x03, 0x9b, 0xca, 0x40, 0x08}};
        .addr_bytes = MAC_LOCAL};//amax1
    struct rte_ether_addr dst_addr = {
        // .addr_bytes = {0x04, 0x3f, 0x72, 0xde, 0xba, 0x44}}; //amax 8
        // .addr_bytes = {0x98, 0x03, 0x9b, 0xca, 0x40, 0x18}}; //amax 7
        .addr_bytes = MAC_FPGA}; //fpga
        // .addr_bytes = {0x98, 0x03, 0x9b, 0xca, 0x48, 0x38}}; //amax 2
    // rte_ether_addr_copy(&port_eth_addrs[0], &eth->s_addr);
//-----------------------end-----------------------
//-------------------------------------------------
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
    /* create the action sequence. one action only,  move packet to queue */
    action[0].type = RTE_FLOW_ACTION_TYPE_QUEUE;//choose a queue
    action[0].conf = &queue;
    action[1].type = RTE_FLOW_ACTION_TYPE_END; //A list of actions is terminated by a END action.
    // printf("check point 1.\n");
    /* set the first level of the pattern (ETH). since in this example we just want to get the ipv4 we set this level to allow all. */
	pattern[0].type = RTE_FLOW_ITEM_TYPE_ETH; // need to build correct stack
	pattern[1].type = RTE_FLOW_ITEM_TYPE_IPV4;
	pattern[2].type = RTE_FLOW_ITEM_TYPE_UDP;
	pattern[3].type = RTE_FLOW_ITEM_TYPE_END;
    // printf("check point 2.\n");
    // /*
    //  * setting the second level of the pattern (IP). in this example this is the level we care about so we set it according to the parameters.
    //  */
    memset(&udp_spec, 0, sizeof(struct rte_flow_item_udp));
    memset(&udp_mask, 0, sizeof(struct rte_flow_item_udp));
    udp_spec.hdr.dst_port = htons(port_udp); 
    udp_mask.hdr.dst_port = 0xffff; 
    pattern[2].spec = &udp_spec;
    pattern[2].mask = &udp_mask;

    // printf("check point 3.\n");
	res = rte_flow_validate(port_phys, &attr, pattern, action, error);
	if (!res)
		flow = rte_flow_create(port_phys, &attr, pattern, action, error);
    // printf("check point 4.\n");
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
    // uint32_t rx_queues_per_port = rx_queues_per_lcore * (n_lcores - 1) / n_enabled_ports;
    // uint32_t tx_queues_per_port = (n_lcores - 1) / n_enabled_ports;
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
            // printf("i = %d \t", i);
            clientTotalNum += clientInLcoreNum;
            lcore_id_2_rx_queue_id[i] = rx_queue_id;
            core_arr[rx_queue_id] = i;

            lcore_conf[i].vid = vid++;
            // if(i == 0){
            //     lcore_conf[i].n_rx_queue = 0;
            // }
            // else{
                lcore_conf[i].n_rx_queue = rx_queues_per_lcore;
                for (j = 0; j < lcore_conf[i].n_rx_queue; j++) {
                    lcore_conf[i].rx_queue_list[j] = rx_queue_id++;
                }
                lcore_conf[i].tx_queue_id = tx_queue_id++;
            // }
            lcore_conf[i].port = enabled_ports[portid_offset];
            if (tx_queue_id % tx_queues_per_port == 0) {
                portid_offset++;
                rx_queue_id = 0;
                tx_queue_id = 0;
            }
        }
    }

    // for (i=0;i<NC_MAX_LCORES;i++) {
    //     DEBUG_PRINT("l_core_%d, n_rx:%d\n", i, lcore_conf[i].n_rx_queue);
    //     DEBUG_PRINT("l_core_%d, tx_q_id:%d\n",i, lcore_conf[i].tx_queue_id);
    //     for (j = 0; j < lcore_conf[i].n_rx_queue; j++) {
    //         DEBUG_PRINT("rx_q[%d]:%d\n", j, lcore_conf[i].rx_queue_list[j]);
    //     }
    // }
    // DEBUG_PRINT("rx_queues_per_port:%d\n", rx_queues_per_port);

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
        //rte_flow
        struct rte_flow_error error;
        for(i = 0; i < rx_queues_per_port; i++){
            // printf("i + CLIENT_PORT = %d\n", i + CLIENT_PORT);
            struct rte_flow * flow;
            if(i == 0){
                flow = generate_udp_flow(portid, HC_PORT, i / NC_RX_QUEUE_PER_LCORE, &error);
            }
            else{
                flow = generate_udp_flow(portid, i + CLIENT_PORT, i / NC_RX_QUEUE_PER_LCORE, &error);
            }
            
            if (!flow) {
                printf("Flow can't be created %d message: %s\n", error.type, error.message ? error.message : "(no stated reason)");
                rte_exit(EXIT_FAILURE, "error in creating flow");
            } 
        }
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

// print current time
static void print_time(void) {
    time_t timer;
    char buffer[26];
    struct tm* tm_info;
    time(&timer);
    tm_info = localtime(&timer);
    strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    printf("%s\n", buffer);
}

// print per-core throughput

// FILE* fout;

static void print_per_core_throughput_lock(void) {
    // time is in second
    // printf("%lld\nthroughput\n", (long long)time(NULL));
    uint32_t i, j;
    // uint32_t core_arr[NC_MAX_LCORES] = {0,1,2,3,4,5,6,7,8,9,10,11, \
    //                         24,25,26,27,28,29,30,31,32,33,34,35};
    

    fflush(stdout);

    float tx                   = 0; 
    float tx_grant             = 0; 
    float tx_grant_re4wait     = 0; 
    float tx_release           = 0; 
    float tx_release_re4modfail= 0; 
    float tx_release_re4wait   = 0; 
    float rx                   = 0; //
    float rx_grantSucc         = 0; //
    float rx_queueSucc         = 0; //
    float rx_releaseSucc       = 0; //
    float rx_grantFail         = 0; //
    float rx_grantModFail      = 0; //
    float rx_releaseFail       = 0; //
    float rx_releaseModFail    = 0; //
    float rx_p4                = 0; //
    float rx_grantSucc_p4      = 0; //
    float rx_queueSucc_p4      = 0; //
    float rx_releaseSucc_p4    = 0; //
    float rx_grantFail_p4      = 0; //
    float rx_releaseFail_p4    = 0; //
    float rx_bdFail_p4         = 0;
    float rx_mirror_p4         = 0;
    float rx_fpga              = 0; //
    float rx_grantSucc_fpga    = 0; //
    float rx_queueSucc_fpga    = 0; //
    float rx_releaseSucc_fpga  = 0; //
    float rx_grantFail_fpga    = 0; //
    float rx_grantModFail_fpga = 0; //
    float rx_grantModHitFail_fpga = 0; // 
    float rx_grantModNfsFail_fpga = 0; //  
    float rx_releaseFail_fpga  = 0; //     
    float rx_releaseModFail_fpga = 0 ; //
    float rx_releaseModHitFail_fpga = 0; // 
    float rx_releaseModNfsFail_fpga = 0; //  
    float dropped_pkt          = 0; 
    float txn_fail             = 0; 
    float txn_total            = 0; 

    for (j = 0; j < n_lcores; j++) {
        i = core_arr[j];
        // printf("\tcore %"PRIu32"\t"
        //     "tx: %"PRIu64"\t"
        //     "tx_grant: %"PRIu64"\t"
        //     "tx_grant_re4wait: %"PRIu64"\t"
        //     "tx_release: %"PRIu64"\t"
        //     "tx_release_re4modfail: %"PRIu64"\t"
        //     "tx_release_re4wait: %"PRIu64"\n"

        //     "\trx: %"PRIu64"\t"
        //     "rx_grantSucc: %"PRIu64"\t"
        //     "rx_queueSucc: %"PRIu64"\t"
        //     "rx_releaseSucc: %"PRIu64"\t"
        //     "rx_grantFail: %"PRIu64"\t"
        //     "rx_grantModFail: %"PRIu64"\t"
        //     "rx_releaseFail: %"PRIu64"\t"
        //     "rx_releaseModFail: %"PRIu64"\n"

        //     "\trx_p4: %"PRIu64"\t"
        //     "rx_grantSucc_p4: %"PRIu64"\t"
        //     "rx_queueSucc_p4: %"PRIu64"\t"
        //     "rx_releaseSucc_p4: %"PRIu64"\t"
        //     "rx_grantFail_p4: %"PRIu64"\t"
        //     "rx_releaseFail_p4: %"PRIu64"\n"

        //     "\trx_fpga: %"PRIu64"\t"
        //     "rx_grantSucc_fpga: %"PRIu64"\t"
        //     "rx_queueSucc_fpga: %"PRIu64"\t"
        //     "rx_releaseSucc_fpga: %"PRIu64"\t"
        //     "rx_grantFail_fpga: %"PRIu64"\t"
        //     "rx_grantModFail_fpga: %"PRIu64"\t"
        //     "rx_releaseFail_fpga: %"PRIu64"\t"
        //     "rx_releaseModFail_fpga: %"PRIu64"\n"

        //     "\tdropped_pkt: %"PRIu64"\t"
        //     "txn_fail: %"PRIu64"\t"
        //     "txn_total: %"PRIu64"\n\n",
        //     i, 

        //     tput_stat[i].tx                      - tput_stat[i].last_tx                       ,
        //     tput_stat[i].tx_grant                - tput_stat[i].last_tx_grant                 ,
        //     tput_stat[i].tx_grant_re4wait        - tput_stat[i].last_tx_grant_re4wait         ,
        //     tput_stat[i].tx_release              - tput_stat[i].last_tx_release               ,
        //     tput_stat[i].tx_release_re4modfail   - tput_stat[i].last_tx_release_re4modfail    ,
        //     tput_stat[i].tx_release_re4wait      - tput_stat[i].last_tx_release_re4wait       ,
        //     tput_stat[i].rx                      - tput_stat[i].last_rx                       ,
        //     tput_stat[i].rx_grantSucc            - tput_stat[i].last_rx_grantSucc             ,
        //     tput_stat[i].rx_queueSucc            - tput_stat[i].last_rx_queueSucc             ,
        //     tput_stat[i].rx_releaseSucc          - tput_stat[i].last_rx_releaseSucc           ,
        //     tput_stat[i].rx_grantFail            - tput_stat[i].last_rx_grantFail             ,
        //     tput_stat[i].rx_grantModFail         - tput_stat[i].last_rx_grantModFail          ,
        //     tput_stat[i].rx_releaseFail          - tput_stat[i].last_rx_releaseFail           ,
        //     tput_stat[i].rx_releaseModFail       - tput_stat[i].last_rx_releaseModFail        ,
        //     tput_stat[i].rx_p4                   - tput_stat[i].last_rx_p4                    ,
        //     tput_stat[i].rx_grantSucc_p4         - tput_stat[i].last_rx_grantSucc_p4          ,
        //     tput_stat[i].rx_queueSucc_p4         - tput_stat[i].last_rx_queueSucc_p4          ,
        //     tput_stat[i].rx_releaseSucc_p4       - tput_stat[i].last_rx_releaseSucc_p4        ,
        //     tput_stat[i].rx_grantFail_p4         - tput_stat[i].last_rx_grantFail_p4          ,
        //     tput_stat[i].rx_releaseFail_p4       - tput_stat[i].last_rx_releaseFail_p4        ,
        //     tput_stat[i].rx_fpga                 - tput_stat[i].last_rx_fpga                  ,
        //     tput_stat[i].rx_grantSucc_fpga       - tput_stat[i].last_rx_grantSucc_fpga        ,
        //     tput_stat[i].rx_queueSucc_fpga       - tput_stat[i].last_rx_queueSucc_fpga        ,
        //     tput_stat[i].rx_releaseSucc_fpga     - tput_stat[i].last_rx_releaseSucc_fpga      ,
        //     tput_stat[i].rx_grantFail_fpga       - tput_stat[i].last_rx_grantFail_fpga        ,
        //     tput_stat[i].rx_grantModFail_fpga    - tput_stat[i].last_rx_grantModFail_fpga     ,
        //     tput_stat[i].rx_releaseFail_fpga     - tput_stat[i].last_rx_releaseFail_fpga      ,
        //     tput_stat[i].rx_releaseModFail_fpga  - tput_stat[i].last_rx_releaseModFail_fpga   ,
        //     tput_stat[i].dropped_pkt             - tput_stat[i].last_dropped_pkt              ,
        //     tput_stat[i].txn_fail                - tput_stat[i].last_txn_fail                 ,
        //     tput_stat[i].txn_total               - tput_stat[i].last_txn_total                  
        // );

        tx                      +=  tput_stat[i].tx                      -   tput_stat[i].last_tx                    ;
        tx_grant                +=  tput_stat[i].tx_grant                -   tput_stat[i].last_tx_grant              ;
        tx_grant_re4wait        +=  tput_stat[i].tx_grant_re4wait        -   tput_stat[i].last_tx_grant_re4wait      ;
        tx_release              +=  tput_stat[i].tx_release              -   tput_stat[i].last_tx_release            ;
        tx_release_re4modfail   +=  tput_stat[i].tx_release_re4modfail   -   tput_stat[i].last_tx_release_re4modfail ;
        tx_release_re4wait      +=  tput_stat[i].tx_release_re4wait      -   tput_stat[i].last_tx_release_re4wait    ;
        rx                      +=  tput_stat[i].rx                      -   tput_stat[i].last_rx                    ;
        rx_grantSucc            +=  tput_stat[i].rx_grantSucc            -   tput_stat[i].last_rx_grantSucc          ;
        rx_queueSucc            +=  tput_stat[i].rx_queueSucc            -   tput_stat[i].last_rx_queueSucc          ;
        rx_releaseSucc          +=  tput_stat[i].rx_releaseSucc          -   tput_stat[i].last_rx_releaseSucc        ;
        rx_grantFail            +=  tput_stat[i].rx_grantFail            -   tput_stat[i].last_rx_grantFail          ;
        rx_grantModFail         +=  tput_stat[i].rx_grantModFail         -   tput_stat[i].last_rx_grantModFail       ;
        rx_releaseFail          +=  tput_stat[i].rx_releaseFail          -   tput_stat[i].last_rx_releaseFail        ;
        rx_releaseModFail       +=  tput_stat[i].rx_releaseModFail       -   tput_stat[i].last_rx_releaseModFail     ;
        rx_p4                   +=  tput_stat[i].rx_p4                   -   tput_stat[i].last_rx_p4                 ;
        rx_grantSucc_p4         +=  tput_stat[i].rx_grantSucc_p4         -   tput_stat[i].last_rx_grantSucc_p4       ;
        rx_queueSucc_p4         +=  tput_stat[i].rx_queueSucc_p4         -   tput_stat[i].last_rx_queueSucc_p4       ;
        rx_releaseSucc_p4       +=  tput_stat[i].rx_releaseSucc_p4       -   tput_stat[i].last_rx_releaseSucc_p4     ;
        rx_grantFail_p4         +=  tput_stat[i].rx_grantFail_p4         -   tput_stat[i].last_rx_grantFail_p4       ;
        rx_releaseFail_p4       +=  tput_stat[i].rx_releaseFail_p4       -   tput_stat[i].last_rx_releaseFail_p4     ;
        rx_bdFail_p4       +=  tput_stat[i].rx_bdFail_p4       -   tput_stat[i].last_rx_bdFail_p4     ;
        rx_mirror_p4       +=  tput_stat[i].rx_mirror_p4       -   tput_stat[i].last_rx_mirror_p4     ;
        
        rx_fpga                 +=  tput_stat[i].rx_fpga                 -   tput_stat[i].last_rx_fpga               ;
        rx_grantSucc_fpga       +=  tput_stat[i].rx_grantSucc_fpga       -   tput_stat[i].last_rx_grantSucc_fpga     ;
        rx_queueSucc_fpga       +=  tput_stat[i].rx_queueSucc_fpga       -   tput_stat[i].last_rx_queueSucc_fpga     ;
        rx_releaseSucc_fpga     +=  tput_stat[i].rx_releaseSucc_fpga     -   tput_stat[i].last_rx_releaseSucc_fpga   ;
        rx_grantFail_fpga       +=  tput_stat[i].rx_grantFail_fpga       -   tput_stat[i].last_rx_grantFail_fpga     ;
        rx_grantModFail_fpga    +=  tput_stat[i].rx_grantModFail_fpga    -   tput_stat[i].last_rx_grantModFail_fpga  ;
        rx_grantModHitFail_fpga  +=  tput_stat[i].rx_grantModHitFail_fpga  -   tput_stat[i].last_rx_grantModHitFail_fpga   ; // 
        rx_grantModNfsFail_fpga  +=  tput_stat[i].rx_grantModNfsFail_fpga  -   tput_stat[i].last_rx_grantModNfsFail_fpga   ; // 

        rx_releaseFail_fpga     +=  tput_stat[i].rx_releaseFail_fpga     -   tput_stat[i].last_rx_releaseFail_fpga   ;
        rx_releaseModFail_fpga  +=  tput_stat[i].rx_releaseModFail_fpga  -   tput_stat[i].last_rx_releaseModFail_fpga;
        rx_releaseModHitFail_fpga  +=  tput_stat[i].rx_releaseModHitFail_fpga  -   tput_stat[i].last_rx_releaseModHitFail_fpga   ; // 
        rx_releaseModNfsFail_fpga  +=  tput_stat[i].rx_releaseModNfsFail_fpga  -   tput_stat[i].last_rx_releaseModNfsFail_fpga   ; //  

        dropped_pkt             +=  tput_stat[i].dropped_pkt             -   tput_stat[i].last_dropped_pkt           ;
        txn_fail                +=  tput_stat[i].txn_fail                -   tput_stat[i].last_txn_fail              ;
        txn_total               +=  tput_stat[i].txn_total               -   tput_stat[i].last_txn_total             ;


        tput_stat[i].last_tx                      =   tput_stat[i].tx                    ;
        tput_stat[i].last_tx_grant                =   tput_stat[i].tx_grant              ;
        tput_stat[i].last_tx_grant_re4wait        =   tput_stat[i].tx_grant_re4wait      ;
        tput_stat[i].last_tx_release              =   tput_stat[i].tx_release            ;
        tput_stat[i].last_tx_release_re4modfail   =   tput_stat[i].tx_release_re4modfail ;
        tput_stat[i].last_tx_release_re4wait      =   tput_stat[i].tx_release_re4wait    ;
        tput_stat[i].last_rx                      =   tput_stat[i].rx                    ;
        tput_stat[i].last_rx_grantSucc            =   tput_stat[i].rx_grantSucc          ;
        tput_stat[i].last_rx_queueSucc            =   tput_stat[i].rx_queueSucc          ;
        tput_stat[i].last_rx_releaseSucc          =   tput_stat[i].rx_releaseSucc        ;
        tput_stat[i].last_rx_grantFail            =   tput_stat[i].rx_grantFail          ;
        tput_stat[i].last_rx_grantModFail         =   tput_stat[i].rx_grantModFail       ;
        tput_stat[i].last_rx_releaseFail          =   tput_stat[i].rx_releaseFail        ;
        tput_stat[i].last_rx_releaseModFail       =   tput_stat[i].rx_releaseModFail     ;
        tput_stat[i].last_rx_p4                   =   tput_stat[i].rx_p4                 ;
        tput_stat[i].last_rx_grantSucc_p4         =   tput_stat[i].rx_grantSucc_p4       ;
        tput_stat[i].last_rx_queueSucc_p4         =   tput_stat[i].rx_queueSucc_p4       ;
        tput_stat[i].last_rx_releaseSucc_p4       =   tput_stat[i].rx_releaseSucc_p4     ;
        tput_stat[i].last_rx_grantFail_p4         =   tput_stat[i].rx_grantFail_p4       ;
        tput_stat[i].last_rx_releaseFail_p4       =   tput_stat[i].rx_releaseFail_p4     ;
        tput_stat[i].last_rx_bdFail_p4       =   tput_stat[i].rx_bdFail_p4     ;
        tput_stat[i].last_rx_mirror_p4       =   tput_stat[i].rx_mirror_p4     ;

        tput_stat[i].last_rx_fpga                 =   tput_stat[i].rx_fpga               ;
        tput_stat[i].last_rx_grantSucc_fpga       =   tput_stat[i].rx_grantSucc_fpga     ;
        tput_stat[i].last_rx_queueSucc_fpga       =   tput_stat[i].rx_queueSucc_fpga     ;
        tput_stat[i].last_rx_releaseSucc_fpga     =   tput_stat[i].rx_releaseSucc_fpga   ;
        tput_stat[i].last_rx_grantFail_fpga       =   tput_stat[i].rx_grantFail_fpga     ;
        tput_stat[i].last_rx_grantModFail_fpga    =   tput_stat[i].rx_grantModFail_fpga  ;
        tput_stat[i].last_rx_grantModHitFail_fpga =   tput_stat[i].rx_grantModHitFail_fpga   ;
        tput_stat[i].last_rx_grantModNfsFail_fpga =   tput_stat[i].rx_grantModNfsFail_fpga   ;

        tput_stat[i].last_rx_releaseFail_fpga     =   tput_stat[i].rx_releaseFail_fpga   ;
        tput_stat[i].last_rx_releaseModFail_fpga  =   tput_stat[i].rx_releaseModFail_fpga;
        tput_stat[i].last_rx_releaseModHitFail_fpga =   tput_stat[i].rx_releaseModHitFail_fpga   ;
        tput_stat[i].last_rx_releaseModNfsFail_fpga =   tput_stat[i].rx_releaseModNfsFail_fpga   ;

        tput_stat[i].last_dropped_pkt             =   tput_stat[i].dropped_pkt           ;
        tput_stat[i].last_txn_fail                =   tput_stat[i].txn_fail              ;
        tput_stat[i].last_txn_total               =   tput_stat[i].txn_total             ;  


        
    }

    printf("\ttotal!!!!!\n"
        "tx\t%0.2f\t"
        "tx_grant\t%0.2f\t"
        "tx_grant_re4wait\t%0.2f\t"
        "tx_release\t%0.2f\t"
        "tx_release_re4modfail\t%0.2f\t"
        "tx_release_re4wait\t%0.2f\n"
        "\trx\t%0.2f\t"
        "rx_grantSucc\t%0.2f\t"
        "rx_queueSucc\t%0.2f\t"
        "rx_releaseSucc\t%0.2f\t"
        "rx_grantFail\t%0.2f\t"
        "rx_grantModFail\t%0.2f\t"
        "rx_releaseFail\t%0.2f\t"
        "rx_releaseModFail\t%0.2f\n"
        "\trx_p4\t%0.2f\t"
        "rx_grantSucc_p4\t%0.2f\t"
        "rx_queueSucc_p4\t%0.2f\t"
        "rx_releaseSucc_p4\t%0.2f\t"
        "rx_grantFail_p4\t%0.2f\t"
        "rx_releaseFail_p4\t%0.2f\t"
        "rx_bdFail_p4\t%0.2f\t"
        "rx_mirror_p4\t%0.2f\n"

        "\trx_fpga\t%0.2f\t"
        "rx_grantSucc_fpga\t%0.2f\t"
        "rx_queueSucc_fpga\t%0.2f\t"
        "rx_releaseSucc_fpga\t%0.2f\t"
        "rx_grantFail_fpga\t%0.2f\t"
        "rx_grantModFail_fpga\t%0.2f\t"
        "rx_grantModHitFail_fpga\t%0.2f\t"
        "rx_grantModNfsFail_fpga\t%0.2f\t"

        "rx_releaseFail_fpga\t%0.2f\t"
        "rx_releaseModFail_fpga\t%0.2f\t"
        "rx_releaseModHitFail_fpga\t%0.2f\t"
        "rx_releaseModNfsFail_fpga\t%0.2f\n"

        "\tdropped_pkt\t%0.2f\t"
        "txn_fail\t%0.2f\t"
        "txn_total\t%0.2f\n\n",
        tx                    ,
        tx_grant              ,
        tx_grant_re4wait      ,
        tx_release            ,
        tx_release_re4modfail ,
        tx_release_re4wait    ,
        rx                    ,
        rx_grantSucc          ,
        rx_queueSucc          ,
        rx_releaseSucc        ,
        rx_grantFail          ,
        rx_grantModFail       ,
        rx_releaseFail        ,
        rx_releaseModFail     ,
        rx_p4                 ,
        rx_grantSucc_p4       ,
        rx_queueSucc_p4       ,
        rx_releaseSucc_p4     ,
        rx_grantFail_p4       ,
        rx_releaseFail_p4     , 
        rx_bdFail_p4     , 
        rx_mirror_p4     , 
        rx_fpga               ,
        rx_grantSucc_fpga     ,
        rx_queueSucc_fpga     ,
        rx_releaseSucc_fpga   ,
        rx_grantFail_fpga     ,
        rx_grantModFail_fpga  ,
        rx_grantModHitFail_fpga  ,
        rx_grantModNfsFail_fpga  ,

        rx_releaseFail_fpga   ,
        rx_releaseModFail_fpga,
        rx_releaseModHitFail_fpga  ,
        rx_releaseModNfsFail_fpga  ,

        dropped_pkt           ,
        txn_fail              ,
        txn_total               
    );

    static int seconds = 0;
    // printf("ms = %d\n", seconds);
    tput_stat_time[seconds].tx                          = tx                       ; 
    tput_stat_time[seconds].tx_grant                    = tx_grant                 ; 
    tput_stat_time[seconds].tx_grant_re4wait            = tx_grant_re4wait         ; 
    tput_stat_time[seconds].tx_release                  = tx_release               ; 
    tput_stat_time[seconds].tx_release_re4modfail       = tx_release_re4modfail    ; 
    tput_stat_time[seconds].tx_release_re4wait          = tx_release_re4wait       ; 
    tput_stat_time[seconds].rx                          = rx                       ; //
    tput_stat_time[seconds].rx_grantSucc                = rx_grantSucc             ; //
    tput_stat_time[seconds].rx_queueSucc                = rx_queueSucc             ; //
    tput_stat_time[seconds].rx_releaseSucc              = rx_releaseSucc           ; //
    tput_stat_time[seconds].rx_grantFail                = rx_grantFail             ; //
    tput_stat_time[seconds].rx_grantModFail             = rx_grantModFail          ; //
    tput_stat_time[seconds].rx_releaseFail              = rx_releaseFail           ; //
    tput_stat_time[seconds].rx_releaseModFail           = rx_releaseModFail        ; //
    tput_stat_time[seconds].rx_p4                       = rx_p4                    ; //
    tput_stat_time[seconds].rx_grantSucc_p4             = rx_grantSucc_p4          ; //
    tput_stat_time[seconds].rx_queueSucc_p4             = rx_queueSucc_p4          ; //
    tput_stat_time[seconds].rx_releaseSucc_p4           = rx_releaseSucc_p4        ; //
    tput_stat_time[seconds].rx_grantFail_p4             = rx_grantFail_p4          ; //
    tput_stat_time[seconds].rx_releaseFail_p4           = rx_releaseFail_p4        ; //
    tput_stat_time[seconds].rx_bdFail_p4                = rx_bdFail_p4             ;
    tput_stat_time[seconds].rx_mirror_p4                = rx_mirror_p4             ;
    tput_stat_time[seconds].rx_fpga                     = rx_fpga                  ; //
    tput_stat_time[seconds].rx_grantSucc_fpga           = rx_grantSucc_fpga        ; //
    tput_stat_time[seconds].rx_queueSucc_fpga           = rx_queueSucc_fpga        ; //
    tput_stat_time[seconds].rx_releaseSucc_fpga         = rx_releaseSucc_fpga      ; //
    tput_stat_time[seconds].rx_grantFail_fpga           = rx_grantFail_fpga        ; //
    tput_stat_time[seconds].rx_grantModFail_fpga        = rx_grantModFail_fpga     ; //
    tput_stat_time[seconds].rx_grantModHitFail_fpga     = rx_grantModHitFail_fpga  ; // 
    tput_stat_time[seconds].rx_grantModNfsFail_fpga     = rx_grantModNfsFail_fpga  ; //  
    tput_stat_time[seconds].rx_releaseFail_fpga         = rx_releaseFail_fpga      ; //     
    tput_stat_time[seconds].rx_releaseModFail_fpga      = rx_releaseModFail_fpga   ; //
    tput_stat_time[seconds].rx_releaseModHitFail_fpga   = rx_releaseModHitFail_fpga; // 
    tput_stat_time[seconds].rx_releaseModNfsFail_fpga   = rx_releaseModNfsFail_fpga; //  
    tput_stat_time[seconds].dropped_pkt                 = dropped_pkt              ; 
    tput_stat_time[seconds].txn_fail                    = txn_fail                 ; 
    tput_stat_time[seconds].txn_total                   = txn_total                ;
    seconds++;

    

    // printf("deadlock ratio: %.2f, rx rate: %.2fMrps, rx_succ rate: %.2fMrps, txn rate: %.2fMrps, drop rate: %.2fMrps, clientNum*lockInTxnNum/lockInSwitchValid: %0.2f, txnRate/lockInSwitchValid: %0.2frps/lock \n", \
    // txn_fail / txn_total, rx_count / 1000000, rx_succ_count / 1000000, txn_count / 1000000, drop_count / 1000000, (1.0 * clientTotalNum * lockInTxnNum) / lockInSwitchValidNum, txn_count / lockInSwitchValidNum);
    // printf("deadlock ratio: %f, rx rate: %frps, rx_succ rate: %frps, txn rate: %frps, drop rate: %frps, clientNum*lockInTxnNum/lockInSwitchValid: %0.2f, txnRate/lockInSwitchValid: %0.2frps/lock \n", \
    // deadlock_count / txn_count, rx_count, rx_succ_count, txn_count, drop_count, (1.0 * clientTotalNum * lockInTxnNum) / lockInSwitchValidNum, txn_count / lockInSwitchValidNum);

    // fprintf(fout, "deadlock ratio: %f, rx rate: %frps, rx_succ rate: %frps, txn rate: %frps, drop rate: %frps, clientNum*lockInTxnNum/lockInSwitchValid: %0.2f, txnRate/lockInSwitchValid: %0.2frps/lock \n", 
    // deadlock_count / txn_count, rx_count, rx_succ_count, txn_count, drop_count, (1.0 * clientTotalNum * lockInTxnNum) / lockInSwitchValidNum, txn_count / lockInSwitchValidNum);
}

static void print_per_core_throughput_kvs(void) {
    // time is in second
    // printf("%lld\nthroughput\n", (long long)time(NULL));
    uint32_t i, j;
    // uint32_t core_arr[NC_MAX_LCORES] = {0,1,2,3,4,5,6,7,8,9,10,11, \
    //                         24,25,26,27,28,29,30,31,32,33,34,35};
    

    fflush(stdout);

    float tx                    = 0; 
    float tx_get                = 0; 
    float tx_put                = 0; 
    float tx_del                = 0; 
    float rx                    = 0; //
    float rx_getSucc            = 0; //
    float rx_putSucc            = 0; //
    float rx_delSucc            = 0; //
    float rx_getFail            = 0; 
    float rx_putFail            = 0; 
    float rx_delFail            = 0; 
    float dropped_pkt          = 0; 

    for (j = 0; j < n_lcores; j++) {
        i = core_arr[j];
        // printf("\tcore %"PRIu32"\t"
        //     "tx: %"PRIu64"\t"
        //     "tx_grant: %"PRIu64"\t"
        //     "tx_grant_re4wait: %"PRIu64"\t"
        //     "tx_release: %"PRIu64"\t"
        //     "tx_release_re4modfail: %"PRIu64"\t"
        //     "tx_release_re4wait: %"PRIu64"\n"

        //     "\trx: %"PRIu64"\t"
        //     "rx_grantSucc: %"PRIu64"\t"
        //     "rx_queueSucc: %"PRIu64"\t"
        //     "rx_releaseSucc: %"PRIu64"\t"
        //     "rx_grantFail: %"PRIu64"\t"
        //     "rx_grantModFail: %"PRIu64"\t"
        //     "rx_releaseFail: %"PRIu64"\t"
        //     "rx_releaseModFail: %"PRIu64"\n"

        //     "\trx_p4: %"PRIu64"\t"
        //     "rx_grantSucc_p4: %"PRIu64"\t"
        //     "rx_queueSucc_p4: %"PRIu64"\t"
        //     "rx_releaseSucc_p4: %"PRIu64"\t"
        //     "rx_grantFail_p4: %"PRIu64"\t"
        //     "rx_releaseFail_p4: %"PRIu64"\n"

        //     "\trx_fpga: %"PRIu64"\t"
        //     "rx_grantSucc_fpga: %"PRIu64"\t"
        //     "rx_queueSucc_fpga: %"PRIu64"\t"
        //     "rx_releaseSucc_fpga: %"PRIu64"\t"
        //     "rx_grantFail_fpga: %"PRIu64"\t"
        //     "rx_grantModFail_fpga: %"PRIu64"\t"
        //     "rx_releaseFail_fpga: %"PRIu64"\t"
        //     "rx_releaseModFail_fpga: %"PRIu64"\n"

        //     "\tdropped_pkt: %"PRIu64"\t"
        //     "txn_fail: %"PRIu64"\t"
        //     "txn_total: %"PRIu64"\n\n",
        //     i, 

        //     tput_stat[i].tx                      - tput_stat[i].last_tx                       ,
        //     tput_stat[i].tx_grant                - tput_stat[i].last_tx_grant                 ,
        //     tput_stat[i].tx_grant_re4wait        - tput_stat[i].last_tx_grant_re4wait         ,
        //     tput_stat[i].tx_release              - tput_stat[i].last_tx_release               ,
        //     tput_stat[i].tx_release_re4modfail   - tput_stat[i].last_tx_release_re4modfail    ,
        //     tput_stat[i].tx_release_re4wait      - tput_stat[i].last_tx_release_re4wait       ,
        //     tput_stat[i].rx                      - tput_stat[i].last_rx                       ,
        //     tput_stat[i].rx_grantSucc            - tput_stat[i].last_rx_grantSucc             ,
        //     tput_stat[i].rx_queueSucc            - tput_stat[i].last_rx_queueSucc             ,
        //     tput_stat[i].rx_releaseSucc          - tput_stat[i].last_rx_releaseSucc           ,
        //     tput_stat[i].rx_grantFail            - tput_stat[i].last_rx_grantFail             ,
        //     tput_stat[i].rx_grantModFail         - tput_stat[i].last_rx_grantModFail          ,
        //     tput_stat[i].rx_releaseFail          - tput_stat[i].last_rx_releaseFail           ,
        //     tput_stat[i].rx_releaseModFail       - tput_stat[i].last_rx_releaseModFail        ,
        //     tput_stat[i].rx_p4                   - tput_stat[i].last_rx_p4                    ,
        //     tput_stat[i].rx_grantSucc_p4         - tput_stat[i].last_rx_grantSucc_p4          ,
        //     tput_stat[i].rx_queueSucc_p4         - tput_stat[i].last_rx_queueSucc_p4          ,
        //     tput_stat[i].rx_releaseSucc_p4       - tput_stat[i].last_rx_releaseSucc_p4        ,
        //     tput_stat[i].rx_grantFail_p4         - tput_stat[i].last_rx_grantFail_p4          ,
        //     tput_stat[i].rx_releaseFail_p4       - tput_stat[i].last_rx_releaseFail_p4        ,
        //     tput_stat[i].rx_fpga                 - tput_stat[i].last_rx_fpga                  ,
        //     tput_stat[i].rx_grantSucc_fpga       - tput_stat[i].last_rx_grantSucc_fpga        ,
        //     tput_stat[i].rx_queueSucc_fpga       - tput_stat[i].last_rx_queueSucc_fpga        ,
        //     tput_stat[i].rx_releaseSucc_fpga     - tput_stat[i].last_rx_releaseSucc_fpga      ,
        //     tput_stat[i].rx_grantFail_fpga       - tput_stat[i].last_rx_grantFail_fpga        ,
        //     tput_stat[i].rx_grantModFail_fpga    - tput_stat[i].last_rx_grantModFail_fpga     ,
        //     tput_stat[i].rx_releaseFail_fpga     - tput_stat[i].last_rx_releaseFail_fpga      ,
        //     tput_stat[i].rx_releaseModFail_fpga  - tput_stat[i].last_rx_releaseModFail_fpga   ,
        //     tput_stat[i].dropped_pkt             - tput_stat[i].last_dropped_pkt              ,
        //     tput_stat[i].txn_fail                - tput_stat[i].last_txn_fail                 ,
        //     tput_stat[i].txn_total               - tput_stat[i].last_txn_total                  
        // );

        tx                      +=  tput_stat[i].tx                         -   tput_stat[i].last_tx                    ;
        tx_get                  +=  tput_stat[i].tx_get                     -   tput_stat[i].last_tx_get                ;
        tx_put                  +=  tput_stat[i].tx_put                     -   tput_stat[i].last_tx_put                ;
        tx_del                  +=  tput_stat[i].tx_del                     -   tput_stat[i].last_tx_del                ;
        rx                      +=  tput_stat[i].rx                         -   tput_stat[i].last_rx                    ;
        rx_getSucc              +=  tput_stat[i].rx_getSucc                 -   tput_stat[i].last_rx_getSucc          ;
        rx_putSucc              +=  tput_stat[i].rx_putSucc                 -   tput_stat[i].last_rx_putSucc          ;
        rx_delSucc              +=  tput_stat[i].rx_delSucc                 -   tput_stat[i].last_rx_delSucc        ;
        rx_getFail              +=  tput_stat[i].rx_getFail                 -   tput_stat[i].last_rx_getFail          ;
        rx_putFail              +=  tput_stat[i].rx_putFail                 -   tput_stat[i].last_rx_putFail          ;
        rx_delFail              +=  tput_stat[i].rx_delFail                 -   tput_stat[i].last_rx_delFail        ;
        dropped_pkt             +=  tput_stat[i].dropped_pkt             -   tput_stat[i].last_dropped_pkt           ;

        tput_stat[i].last_tx                        =   tput_stat[i].tx                    ;
        tput_stat[i].last_tx_get                    =   tput_stat[i].tx_get              ;
        tput_stat[i].last_tx_put                    =   tput_stat[i].tx_put      ;
        tput_stat[i].last_tx_del                    =   tput_stat[i].tx_del            ;
        tput_stat[i].last_rx                        =   tput_stat[i].rx                    ;
        tput_stat[i].last_rx_getSucc                =   tput_stat[i].rx_getSucc          ;
        tput_stat[i].last_rx_putSucc                =   tput_stat[i].rx_putSucc          ;
        tput_stat[i].last_rx_delSucc                =   tput_stat[i].rx_delSucc        ;
        tput_stat[i].last_rx_getFail                =   tput_stat[i].rx_getFail          ;
        tput_stat[i].last_rx_putFail                =   tput_stat[i].rx_putFail          ;
        tput_stat[i].last_rx_delFail                =   tput_stat[i].rx_delFail        ;
        tput_stat[i].last_dropped_pkt             =   tput_stat[i].dropped_pkt           ;
    }

    printf("\ttotal!!!!!\n"
        "tx\t%0.2f\t"
        "tx_get\t%0.2f\t"
        "tx_put\t%0.2f\t"
        "tx_del\t%0.2f\n"
        "\trx\t%0.2f\t"
        "rx_getSucc\t%0.2f\t"
        "rx_putSucc\t%0.2f\t"
        "rx_delSucc\t%0.2f\t"   
        "rx_getFail\t%0.2f\t"
        "rx_putFail\t%0.2f\t"
        "rx_delFail\t%0.2f\t"   
        "\tdropped_pkt\t%0.2f\n",
        tx                      ,
        tx_get                  ,
        tx_put                  ,
        tx_del                  ,
        rx                      ,
        rx_getSucc              ,
        rx_putSucc              ,
        rx_delSucc              ,
        rx_getFail              ,
        rx_putFail              ,
        rx_delFail              ,
        dropped_pkt
    );

    static int seconds = 0;
    // printf("ms = %d\n", seconds);
    tput_stat_time[seconds].tx                          = tx                       ; 
    tput_stat_time[seconds].tx_get                    = tx_get                 ; 
    tput_stat_time[seconds].tx_put            = tx_put         ; 
    tput_stat_time[seconds].tx_del                  = tx_del               ; 
    tput_stat_time[seconds].rx                          = rx                       ; //
    tput_stat_time[seconds].rx_getSucc                = rx_getSucc             ; //
    tput_stat_time[seconds].rx_putSucc                = rx_putSucc             ; //
    tput_stat_time[seconds].rx_delSucc              = rx_delSucc           ; //
    tput_stat_time[seconds].rx_getFail                = rx_getFail             ; //
    tput_stat_time[seconds].rx_putFail                = rx_putFail             ; //
    tput_stat_time[seconds].rx_delFail                = rx_delFail           ; //
    tput_stat_time[seconds].dropped_pkt                 = dropped_pkt              ; 
    seconds++;

    

    // printf("deadlock ratio: %.2f, rx rate: %.2fMrps, rx_succ rate: %.2fMrps, txn rate: %.2fMrps, drop rate: %.2fMrps, clientNum*lockInTxnNum/lockInSwitchValid: %0.2f, txnRate/lockInSwitchValid: %0.2frps/lock \n", \
    // txn_fail / txn_total, rx_count / 1000000, rx_succ_count / 1000000, txn_count / 1000000, drop_count / 1000000, (1.0 * clientTotalNum * lockInTxnNum) / lockInSwitchValidNum, txn_count / lockInSwitchValidNum);
    // printf("deadlock ratio: %f, rx rate: %frps, rx_succ rate: %frps, txn rate: %frps, drop rate: %frps, clientNum*lockInTxnNum/lockInSwitchValid: %0.2f, txnRate/lockInSwitchValid: %0.2frps/lock \n", \
    // deadlock_count / txn_count, rx_count, rx_succ_count, txn_count, drop_count, (1.0 * clientTotalNum * lockInTxnNum) / lockInSwitchValidNum, txn_count / lockInSwitchValidNum);

    // fprintf(fout, "deadlock ratio: %f, rx rate: %frps, rx_succ rate: %frps, txn rate: %frps, drop rate: %frps, clientNum*lockInTxnNum/lockInSwitchValid: %0.2f, txnRate/lockInSwitchValid: %0.2frps/lock \n", 
    // deadlock_count / txn_count, rx_count, rx_succ_count, txn_count, drop_count, (1.0 * clientTotalNum * lockInTxnNum) / lockInSwitchValidNum, txn_count / lockInSwitchValidNum);
}

static void print_time_trace_lock(uint64_t times, FILE* fout_stats) {

        fprintf(fout_stats, "second\t"
        "tx\t"
        "tx_grant\t"
        "tx_grant_re4wait\t"
        "tx_release\t"
        "tx_release_re4modfail\t"
        "tx_release_re4wait\t"
        "rx\t"
        "rx_grantSucc\t"
        "rx_queueSucc\t"
        "rx_releaseSucc\t"
        "rx_grantFail\t"
        "rx_grantModFail\t"
        "rx_releaseFail\t"
        "rx_releaseModFail\t"
        "rx_p4\t"
        "rx_grantSucc_p4\t"
        "rx_queueSucc_p4\t"
        "rx_releaseSucc_p4\t"
        "rx_grantFail_p4\t"
        "rx_releaseFail_p4\t"
        "rx_bdFail_p4\t"
        "rx_mirror_p4\t"

        "rx_fpga\t"
        "rx_grantSucc_fpga\t"
        "rx_queueSucc_fpga\t"
        "rx_releaseSucc_fpga\t"
        "rx_grantFail_fpga\t"
        "rx_grantModFail_fpga\t"
        "rx_grantModHitFail_fpga\t"
        "rx_grantModNfsFail_fpga\t"

        "rx_releaseFail_fpga\t"
        "rx_releaseModFail_fpga\t"
        "rx_releaseModHitFail_fpga\t"
        "rx_releaseModNfsFail_fpga\t"

        "dropped_pkt\t"
        "txn_fail\t"
        "txn_total\t\n");

    for(int i = 1; i < times; i++){
        fprintf(fout_stats, "%d\t"
        "%0.2f\t"
        "%0.2f\t"
        "%0.2f\t"
        "%0.2f\t"
        "%0.2f\t"
        "%0.2f\t"
        "%0.2f\t"
        "%0.2f\t"
        "%0.2f\t"
        "%0.2f\t"
        "%0.2f\t"
        "%0.2f\t"
        "%0.2f\t"
        "%0.2f\t"
        "%0.2f\t"
        "%0.2f\t"
        "%0.2f\t"
        "%0.2f\t"
        "%0.2f\t"
        "%0.2f\t"
        "%0.2f\t"
        "%0.2f\t"
        "%0.2f\t"
        "%0.2f\t"
        "%0.2f\t"
        "%0.2f\t"
        "%0.2f\t"
        "%0.2f\t"
        "%0.2f\t"
        "%0.2f\t"
        "%0.2f\t"
        "%0.2f\t"
        "%0.2f\t"
        "%0.2f\t"
        "%0.2f\t"
        "%0.2f\t"
        "%0.2f\t\n",
        i + 1                 ,
        (float)(tput_stat_time[i].tx                        ),
        (float)(tput_stat_time[i].tx_grant                  ),
        (float)(tput_stat_time[i].tx_grant_re4wait          ),
        (float)(tput_stat_time[i].tx_release                ),
        (float)(tput_stat_time[i].tx_release_re4modfail     ),
        (float)(tput_stat_time[i].tx_release_re4wait        ),
        (float)(tput_stat_time[i].rx                        ),
        (float)(tput_stat_time[i].rx_grantSucc              ),
        (float)(tput_stat_time[i].rx_queueSucc              ),
        (float)(tput_stat_time[i].rx_releaseSucc            ),
        (float)(tput_stat_time[i].rx_grantFail              ),
        (float)(tput_stat_time[i].rx_grantModFail           ),
        (float)(tput_stat_time[i].rx_releaseFail            ),
        (float)(tput_stat_time[i].rx_releaseModFail         ),
        (float)(tput_stat_time[i].rx_p4                     ),
        (float)(tput_stat_time[i].rx_grantSucc_p4           ),
        (float)(tput_stat_time[i].rx_queueSucc_p4           ),
        (float)(tput_stat_time[i].rx_releaseSucc_p4         ),
        (float)(tput_stat_time[i].rx_grantFail_p4           ),
        (float)(tput_stat_time[i].rx_releaseFail_p4         ), 
        (float)(tput_stat_time[i].rx_bdFail_p4              ), 
        (float)(tput_stat_time[i].rx_mirror_p4              ), 
        (float)(tput_stat_time[i].rx_fpga                   ),
        (float)(tput_stat_time[i].rx_grantSucc_fpga         ),
        (float)(tput_stat_time[i].rx_queueSucc_fpga         ),
        (float)(tput_stat_time[i].rx_releaseSucc_fpga       ),
        (float)(tput_stat_time[i].rx_grantFail_fpga         ),
        (float)(tput_stat_time[i].rx_grantModFail_fpga      ),
        (float)(tput_stat_time[i].rx_grantModHitFail_fpga   ),
        (float)(tput_stat_time[i].rx_grantModNfsFail_fpga   ),
        (float)(tput_stat_time[i].rx_releaseFail_fpga       ),
        (float)(tput_stat_time[i].rx_releaseModFail_fpga    ),
        (float)(tput_stat_time[i].rx_releaseModHitFail_fpga ),
        (float)(tput_stat_time[i].rx_releaseModNfsFail_fpga ),
        (float)(tput_stat_time[i].dropped_pkt               ),
        (float)(tput_stat_time[i].txn_fail                  ),
        (float)(tput_stat_time[i].txn_total                 ));
    }
}


static void print_time_trace_kvs(uint64_t times, FILE* fout_stats) {

        fprintf(fout_stats, "second\t"
        "tx\t"
        "tx_get\t"
        "tx_put\t"
        "tx_del\t"
        "rx\t"
        "rx_getSucc\t"
        "rx_putSucc\t"
        "rx_delSucc\n");

    for(int i = 1; i < times; i++){
        fprintf(fout_stats, "%d\t"
        "%0.2f\t"
        "%0.2f\t"
        "%0.2f\t"
        "%0.2f\t"
        "%0.2f\t"
        "%0.2f\t"
        "%0.2f\t"
        "%0.2f\t\n",
        i + 1                 ,
        (float)(tput_stat_time[i].tx                        ),
        (float)(tput_stat_time[i].tx_get                  ),
        (float)(tput_stat_time[i].tx_put          ),
        (float)(tput_stat_time[i].tx_del                ),
        (float)(tput_stat_time[i].rx                        ),
        (float)(tput_stat_time[i].rx_getSucc              ),
        (float)(tput_stat_time[i].rx_putSucc              ),
        (float)(tput_stat_time[i].rx_delSucc            ));
    }
}

/*
 * misc
 */

static float timediff_in_us(uint64_t new_t, uint64_t old_t) {
    return (float)(new_t - old_t) * 1000000 / rte_get_tsc_hz();
}

#endif //NETCACHE_UTIL_H