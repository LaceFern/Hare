/*
    1.全局变量存储所有锁的Counter，以及一�??备份Counter_backup
    udp port不�?�的会进�??core0，也即�??识别为控制包，需要看一下控制包的格式，udp port的位�??会�??识别成什么东�??
    用一�??全局变量globalStat来控制各�??线程的状�??
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/queue.h>
#include <time.h> 
#include <assert.h>
#include <arpa/inet.h> 
#include <getopt.h>
#include <stdbool.h> 
 

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

#include <unistd.h>

#define COLLECT_LATENCY
#define DEBUG
#undef DEBUG

#include "./include/util.h"
#include "./include/lock_queue.h"
#include "./include/think_queue.h"
#include "./include/txn_queue.h"
#include "./include/new_order.h"
#include "./include/zipf.h"
#include "./include/collect.h"

#include "./include/cache_upd_ctrl.h"

/*
 * constants
 */
#define EXTENSIVE_LOCKS_NORMAL_BENCHMARK 20
#define OBJ_PER_LOCK_NORMAL_BENCHMARK 150
#define EXTENSIVE_LOCKS_MICRO_BENCHMARK_SHARED 55000
#define OBJ_PER_LOCK_MICRO_BENCHMARK_SHARED 1
#define EXTENSIVE_LOCKS_MICRO_BENCHMARK_EXCLUSIVE 55000
#define OBJ_PER_LOCK_MICRO_BENCHMARK_EXCLUSIVE 1
#define EXTENSIVE_LOCKS_TPCC 7000000
#define OBJ_PER_LOCK_TPCC 1

#define MIN_LOSS_RATE           0.01
#define MAX_LOSS_RATE           0.05
#define PKTS_SEND_LIMIT_MIN_MS  300
#define PKTS_SEND_LIMIT_MAX_MS  2500
#define PKTS_SEND_RESTART_MS    300
#define NUM_LCORES              32

/* 
 * custom types 
 */
int timeout_slot = 400;
int client_node_num = 8;
int server_node_num = 1;
char memn_filename[200], memory_management; 
int extensive_locks = EXTENSIVE_LOCKS_NORMAL_BENCHMARK;
int obj_per_lock = OBJ_PER_LOCK_NORMAL_BENCHMARK;
uint32_t zipf_alpha = 90;
//-------------------
txn_queue_list txn_queues[MAX_CLIENT_NUM][MAX_TXN_NUM];
//-------------------
int current_failure_status = 0;
int last_failure_status = 0;
uint32_t last_txn_idx[MAX_CLIENT_NUM] = {0};
uint32_t idx[MAX_CLIENT_NUM] = {0}; 
char deadlocked[MAX_CLIENT_NUM][MAX_TXN_NUM] = {0};
char txn_finished[MAX_CLIENT_NUM][MAX_TXN_NUM] = {0};
int txn_s[MAX_CLIENT_NUM][MAX_TXN_NUM] = {0};
int txn_r[MAX_CLIENT_NUM][MAX_TXN_NUM] = {0};
int num_retries[MAX_CLIENT_NUM][MAX_TXN_NUM] = {0};
int detect_failure[MAX_CLIENT_NUM] = {0};
uint64_t txn_refresh_time[MAX_CLIENT_NUM][MAX_TXN_NUM] = {0};
uint64_t txn_refresh_time_failure[MAX_CLIENT_NUM][MAX_TXN_NUM] = {0};
uint64_t txn_begin_time[MAX_CLIENT_NUM][MAX_TXN_NUM] = {0};
uint64_t txn_finish_time[MAX_CLIENT_NUM][MAX_TXN_NUM] = {0};

volatile uint32_t num_ex[MAX_LOCK_NUM] = {0};
volatile uint32_t num_sh[MAX_LOCK_NUM] = {0};
uint32_t num_ex_granted[MAX_LOCK_NUM] = {0};
uint32_t num_sh_granted[MAX_LOCK_NUM] = {0};
char busy[MAX_CLIENT_NUM][MAX_LOCK_NUM] = {0};
uint32_t ip_dst_pton, ip_src_pton;

//-------------------------------------------------------- ctrl --------------------------------------------------------
#define STATS_LOCK_STAT 0
#define STATS_GET_HOT 1
#define STATS_HOT_REPORT 2
#define STATS_WAIT_EMPTY 3 
#define STATS_WAIT_COMPLETE 4

#define NUM_SERVER_CORE 23

// uint32_t hotCounter[2][MAX_LOCK_NUM] = {0};
// heapNode TopKNode[2][NUM_SERVER_CORE][TOPK + 1];
// heapNode merge_TopKNode[NUM_SERVER_CORE * TOPK];
// uint32_t stats_stage = STATS_LOCK_STAT;
// volatile bool lock_stat_flag = 0;
// volatile bool clear_flag = 0;
// volatile bool clear_TOPK_flag = 0;
// volatile bool check_queue_lock_flag = 0;
volatile bool queueLockFlag[MAX_LOCK_NUM] = {0};
volatile bool queueNotEmptyFlag[MAX_LOCK_NUM] = {0};
// uint32_t hotNum = 0;
// uint32_t last_hotNum = 0;
// uint32_t hotID[TOPK] = {0};
// uint32_t last_hotID[TOPK] = {0};

// //Descending
// int inc(const void *a, const void *b)
// {
// 	return ( * (heapNode * )b).lockCounter > ( * (heapNode * )a).lockCounter ? 1 : -1;
// }

struct latency_statistics {
    uint64_t max;
    uint64_t num;
    uint64_t total;
    uint64_t overflow;
    uint64_t bin[BIN_SIZE];
} __rte_cache_aligned;

/*
 * global variables  
 */
// key-value workload generation
float rate_adjust_per_sec = 1;
uint32_t write_ratio = 0;

// destination ip address

char ip_list[][32] = {
    "10.0.0.1",
    "10.0.0.2",
    "10.0.0.3",
    "10.0.0.4",
    "10.0.0.5",
    "10.0.0.6",
    "10.0.0.7",
    "10.0.0.8",
    };

uint16_t port_read = 8880;
uint16_t port_write = 5001;
uint32_t socket_number = 3;

// generate packets with zipf
struct zipf_gen_state *zipf_state;

// statistics
struct latency_statistics latency_stat_c[NC_MAX_LCORES];
struct latency_statistics latency_stat_avg[NC_MAX_LCORES];

static uint64_t latency_samples_together[1000000] = {0};
uint64_t latency_samples_together_len = 0;
static uint64_t latency_samples[NC_MAX_LCORES][100000] = {0};
uint64_t latency_sample_num[NC_MAX_LCORES] = {0};
uint64_t latency_sample_interval = 1000;
uint64_t latency_sample_start = 0;

static uint32_t second = 0;
static uint64_t throughput_per_second[3600] = {0};

// adjust client rate
uint32_t adjust_start = 0;
uint64_t last_sent = 0;
uint64_t last_recv = 0;

int count_secondary = 0;
int secondary_locks[MAX_LOCK_NUM];

// tpc-c
uint32_t *(txn_id[MAX_CLIENT_NUM]);
uint32_t *(action_type[MAX_CLIENT_NUM]);
uint32_t *(target_lm_id[MAX_CLIENT_NUM]);
uint32_t *(target_obj_idx[MAX_CLIENT_NUM]);
uint32_t *(lock_type[MAX_CLIENT_NUM]);
uint32_t len[MAX_CLIENT_NUM];
queue_list *lockqueues;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define MAX_LOCK_NOTFIT 16
#define MAX_LOCK_IN_SWITCH 10000000 //16//16384//16382 
#define MAX_LOCK_IN_TXN 8
#define RESEND_CHECK_INTERVAL_US 500//50000000//50//200//50// 5000000 // consider adjusting it dynamically
// #define RESEND_MAX_COUNT 100
#define DEADLOCK_CHECK_INTERVAL_US 1000000//50000000//250//1000

#define RESPFLAG_IDLE 0 
#define RESPFLAG_ACQUIRE_WAIT 1
#define RESPFLAG_ACQUIRE_HAVEGOTTEN 2
#define RESPFLAG_ACQUIRE_HAVEENQUEUE 9
#define RESPFLAG_RELEASE_WAIT 3
#define RESPFLAG_RELEASE_HAVEGOTTEN 4
#define RESPFLAG_ACQUIRE_FAIL 5
#define RESPFLAG_RELEASE_FAIL 6
#define RESPFLAG_ACQUIRE_REWAIT 7
#define RESPFLAG_RELEASE_REWAIT 8

// #define NOT_RECIRCULATE         0x00
// #define ACQUIRE_LOCK            0x01
// #define OP_RELEASE_LOCK            0x02
// #define RE_ACQUIRE_LOCK            0x03
// #define RE_RELEASE_LOCK            0x04

// op code
#define GRANT_LOCK 0x01
#define RELEA_LOCK 0x02
#define RE_GRANT_LOCK 0x81
#define RE_RELEA_LOCK 0x82

#define LOCK_GRANT_SUCC 0x03
#define LOCK_GRANT_FAIL 0x04
#define LOCK_RELEA_SUCC 0x05
#define LOCK_RELEA_FAIL 0x06
#define RE_LOCK_GRANT_SUCC 0x83
#define RE_LOCK_GRANT_FAIL 0x84
#define RE_LOCK_RELEA_SUCC 0x85
#define RE_LOCK_RELEA_FAIL 0x86

#define OP_ACQUIRE_LOCK 1
#define OP_RELEASE_LOCK 2
#define OP_GRANT_SUCCESS 3
#define OP_GRANT_FAILED 4
#define OP_RELEASE_SUCCESS 5
#define OP_RELEASE_FAILED 6
#define OP_ENQUEUE_SUCCESS 7
#define OP_MOD_FAILED_NO_FREESLOT 8
#define OP_MOD_FAILED_HIT 9

//lock mode
#define EXCLU_LOCK 1
#define SHARE_LOCK 2

uint32_t wpkts_send_limit_ms = 435;
uint32_t average_interval = 5;

char ip_src[32] = "10.0.0.7";
char ip_dst[32] = "10.0.0.1";

typedef struct lockInfo{
    uint32_t lock_id;
    uint8_t action_type;
    uint8_t lock_type;
    uint8_t resend_count; 
    uint64_t last_send_time;
}struct_lockInfo;

typedef struct txnInfo{
    uint32_t txn_id;
    uint32_t lock_num;
    uint16_t cid;
}struct_txnInfo;

uint16_t lcore_id_2_rx_queue_id[NC_MAX_LCORES];
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void collect_results() {
    char filename_res[200], filename_lt[200], filename_txn_lt[200];
    char dirname_1[200], dirname_2[200], dirname_3[200];
    FILE *fout;
    char cmd[100];
    int i, j;

    uint64_t latency_total=0, num_total=0;
    for (i=0;i<n_rcv_cores;i++) {//n_rcv_cores = 1 in utils.h !!!!!!!!!!!!!!!!!!!!!!!!!!!!
        latency_total = latency_total + latency_stat_c[i].total - latency_stat_avg[i].total;
        num_total = num_total + latency_stat_c[i].num - latency_stat_avg[i].num;
    }
    printf("Average latency: %.4f ms\n", (latency_total - num_total) / (double) (num_total) / 1000.0);
    printf("Average latency: %.4f ms\n", (latency_total) / (double) (num_total) / 1000.0);

    fflush(stdout);

    // Sort latency_samples
    for (int lc = 0; lc < n_rcv_cores; lc++) {
        for (i = 0; i < latency_sample_num[lc]; ++i) {
            latency_samples_together[latency_samples_together_len] = latency_samples[lc][i];
            latency_samples_together_len ++;
        }
    }
    for (i = 0; i < latency_samples_together_len; i++) {
        uint64_t lat = latency_samples_together[i];
        j = i;
        while (j > 0 && latency_samples_together[j - 1] > lat) {
            latency_samples_together[j] = latency_samples_together[j - 1];
            j --;
        }
        latency_samples_together[j] = lat;
    }
    printf("Median latency: %.4f ms\n", latency_samples_together[latency_samples_together_len / 2] / 1000.0);
    fflush(stdout);
}
//----------------------
// print latency
static void print_latency(struct latency_statistics * latency_stat) {
    uint64_t max = 0;
    uint64_t num = 0;
    uint64_t total = 0;
    uint64_t overflow = 0;
    uint64_t bin[BIN_SIZE];
    memset(&bin, 0, sizeof(bin));

    uint32_t i, j;
    double average_latency = 0;

    if (latency_stat[1].num > 0) {
        average_latency = latency_stat[1].total / (double)latency_stat[1].num;
    }
    printf("\tcount: %"PRIu64"\t"
        "average latency: %.4f ms\t"
        "max latency: %.4f ms\t"
        "overflow: %"PRIu64"\n",
        latency_stat[1].num, average_latency / 1000.0, latency_stat[1].max / 1000.0, overflow);
}

// generate write request packet for server
static void generate_ctrl_pkt(uint16_t rx_queue_id, uint8_t header_template[HEADER_TEMPLATE_SIZE_CTRL], uint32_t lcore_id, struct rte_mbuf *mbuf, \
            uint16_t ether_type, uint8_t opType) {
    struct lcore_configuration *lconf = &lcore_conf[lcore_id];
    assert(mbuf != NULL);

    mbuf->next = NULL;
    mbuf->nb_segs = 1;
    mbuf->ol_flags = 0;
    mbuf->data_len = 0;
    mbuf->pkt_len = 0;

    // init packet header
    struct rte_ether_hdr* eth = rte_pktmbuf_mtod(mbuf, struct rte_ether_hdr *);
    rte_memcpy(eth, header_template, HEADER_TEMPLATE_SIZE_CTRL);
    mbuf->data_len += HEADER_TEMPLATE_SIZE_CTRL;
    mbuf->pkt_len += HEADER_TEMPLATE_SIZE_CTRL;

    mbuf->data_len += sizeof(CtrlHeader_fpga);
    mbuf->pkt_len += sizeof(CtrlHeader_fpga);
    CtrlHeader_fpga* message_header = (CtrlHeader_fpga*) ((uint8_t*)eth + HEADER_TEMPLATE_SIZE_CTRL);
    message_header->opType = opType;
}

// // generate write request packet for server
// static void generate_ctrl_pkt_hotReport(uint16_t rx_queue_id, uint8_t header_template[HEADER_TEMPLATE_SIZE_CTRL], uint32_t lcore_id, struct rte_mbuf *mbuf, \
//             uint16_t ether_type, uint8_t opType, heapNode TopKhotNode[]) {
//     struct lcore_configuration *lconf = &lcore_conf[lcore_id];
//     assert(mbuf != NULL);

//     mbuf->next = NULL;
//     mbuf->nb_segs = 1;
//     mbuf->ol_flags = 0;
//     mbuf->data_len = 0;
//     mbuf->pkt_len = 0;

//     // init packet header
//     struct rte_ether_hdr* eth = rte_pktmbuf_mtod(mbuf, struct rte_ether_hdr *);
//     rte_memcpy(eth, header_template, HEADER_TEMPLATE_SIZE_CTRL);
//     mbuf->data_len += HEADER_TEMPLATE_SIZE_CTRL;
//     mbuf->pkt_len += HEADER_TEMPLATE_SIZE_CTRL;

//     mbuf->data_len += sizeof(CtrlHeader_fpga_big);
//     mbuf->pkt_len += sizeof(CtrlHeader_fpga_big);
//     CtrlHeader_fpga_big* message_header = (CtrlHeader_fpga_big*) ((uint8_t*)eth + HEADER_TEMPLATE_SIZE_CTRL);
//     message_header->opType = opType;
//     message_header->replaceNum = 0;
//     for(int i=0; i<TOPK; i++){
//         message_header->lockId[i] = htonl(TopKhotNode[i].lockId);
//         if(message_header->lockId[i] == 4294967295){
//             message_header->lockCounter[i] = 0;
//         }
//         else{
//             message_header->lockCounter[i] = htonl(TopKhotNode[i].lockCounter);
//         }
//     }
// }

// generate write request packet
static void generate_write_request_pkt(uint16_t rx_queue_id, uint8_t header_template[HEADER_TEMPLATE_SIZE], uint32_t lcore_id, struct rte_mbuf *mbuf, \
            struct_lockInfo lockInfo, struct_txnInfo txnInfo, uint32_t ip_src_addr, uint32_t ip_dst_addr, uint64_t timestamp) {
    struct lcore_configuration *lconf = &lcore_conf[lcore_id];
    assert(mbuf != NULL);

    mbuf->next = NULL;
    mbuf->nb_segs = 1;
    mbuf->ol_flags = 0;
    mbuf->data_len = 0;
    mbuf->pkt_len = 0;

    // init packet header
    struct rte_ether_hdr* eth = rte_pktmbuf_mtod(mbuf, struct rte_ether_hdr *);
    struct rte_ipv4_hdr *ip = (struct rte_ipv4_hdr *)((uint8_t*) eth
        + sizeof(struct rte_ether_hdr));
    struct rte_udp_hdr *udp = (struct rte_udp_hdr *)((uint8_t*) ip
        + sizeof(struct rte_ipv4_hdr));
    rte_memcpy(eth, header_template, HEADER_TEMPLATE_SIZE);
    mbuf->data_len += HEADER_TEMPLATE_SIZE;
    mbuf->pkt_len += HEADER_TEMPLATE_SIZE;
    
    if (ip_src_addr != 0) 
        ip->src_addr = ip_src_addr;
    else 
        inet_pton(AF_INET, ip_src, &(ip->src_addr));

    if (ip_dst_addr != 0)
        ip->dst_addr = ip_dst_addr;
    else
        inet_pton(AF_INET, ip_dst, &(ip->dst_addr));

    // printf("ip->src_addr = %x\n", ip->src_addr);
    // printf("ip->dst_addr = %x\n", ip->dst_addr);

    
    udp->src_port = htons(CLIENT_PORT + rx_queue_id);//htons(port_write + (uint32_t)(txnInfo.txn_id) % 128);
    // udp->src_port = htons(port_write); 
    // if(lcore_id == 2) printf("src_port = %d\n", CLIENT_PORT + rx_queue_id);
    udp->dst_port = htons(port_write);

    MessageHeader* message_header = (MessageHeader*) ((uint8_t*)eth + HEADER_TEMPLATE_SIZE);
    message_header->opType = lockInfo.action_type;
    message_header->lockMode = lockInfo.lock_type;
    // message_header->clientID = htons(txnInfo.cid);
    // message_header->lockID = htonl(lockInfo.lock_id);
    message_header->lockID = lockInfo.lock_id;
    // message_header->txnID = htonl(txnInfo.txn_id);
    message_header->txnID = txnInfo.txn_id;
    message_header->clientID = txnInfo.cid;
    if (timestamp == 0) {
        message_header->timestamp = rte_rdtsc();
    } else {
        message_header->timestamp = timestamp;
    }
    message_header->padding = 0;
    //printf("send message_header->payload = %lu\n", message_header->payload);

    // message_header->clientID = txnInfo.cid;
    

    mbuf->data_len += sizeof(MessageHeader);
    mbuf->pkt_len += sizeof(MessageHeader);

    // if(lcore_id == 1){
    //     uint8_t* check = rte_pktmbuf_mtod(mbuf, uint8_t*);
    //     printf("check-tmp: \n");
    //     for(int i = 0; i < HEADER_TEMPLATE_SIZE; i ++){
    //         printf("%x ", header_template[i]);
    //     }
    //     printf("\n");
    //     printf("check-pkt: \n");
    //     for(int i = 0; i < HEADER_TEMPLATE_SIZE; i ++){
    //         printf("%x ", check[i]);
    //     }
    //     printf("\n");
    // }
}

// generate write request packet for server
static void generate_write_request_pkt_server(uint16_t rx_queue_id, uint8_t header_template[HEADER_TEMPLATE_SIZE], uint32_t lcore_id, struct rte_mbuf *mbuf, \
            uint8_t opType, uint8_t lockMode, uint16_t clientID, uint32_t lockID, uint32_t txnID, uint32_t ip_src_addr, uint32_t ip_dst_addr, uint16_t dst_port, uint64_t timestamp) {
    struct lcore_configuration *lconf = &lcore_conf[lcore_id];
    assert(mbuf != NULL);

    mbuf->next = NULL;
    mbuf->nb_segs = 1;
    mbuf->ol_flags = 0;
    mbuf->data_len = 0;
    mbuf->pkt_len = 0;

    // init packet header
    struct rte_ether_hdr* eth = rte_pktmbuf_mtod(mbuf, struct rte_ether_hdr *);
    struct rte_ipv4_hdr *ip = (struct rte_ipv4_hdr *)((uint8_t*) eth
        + sizeof(struct rte_ether_hdr));
    struct rte_udp_hdr *udp = (struct rte_udp_hdr *)((uint8_t*) ip
        + sizeof(struct rte_ipv4_hdr));
    rte_memcpy(eth, header_template, HEADER_TEMPLATE_SIZE);
    mbuf->data_len += HEADER_TEMPLATE_SIZE;
    mbuf->pkt_len += HEADER_TEMPLATE_SIZE;
    
    // for(int i = 0; i < NUM_CLIENT; i++){
    //     uint32_t tmp_dst_addr;
    //     inet_pton(AF_INET, ip_client_arr[i], &tmp_dst_addr);
    //     if(tmp_dst_addr == ip_dst_addr){
    //         ip->dst_addr = tmp_dst_addr;
    //         rte_ether_addr_copy(mac_client_arr + i, &eth->d_addr);
    //         break;
    //     }
    // }   

    uint8_t break_flag = 0;
    for(int i = 0; i < NUM_CLIENT; i++){
        if(ip_client_pton_arr[i] == ip_dst_addr){
            rte_ether_addr_copy(mac_client_arr + i, &eth->d_addr);
            break_flag = 1;
            break;
        }
    }
    if(break_flag == 0){
        printf("something error!\n");
        exit(0);
    }
    ip->src_addr = ip_src_addr;
    ip->dst_addr = ip_dst_addr;

    // //DEBUG!!
    // eth->s_addr = eth_client->d_addr;
    // eth->d_addr = eth_client->s_addr;
    // ip->src_addr = ip_src_addr;
    // ip->dst_addr = ip_dst_addr;

    // printf("ip->src_addr = %x\n", ip->src_addr);
    // printf("ip->dst_addr = %x\n", ip->dst_addr);

    
    udp->src_port = htons(CLIENT_PORT + rx_queue_id - 1);//htons(port_write + (uint32_t)(txnInfo.txn_id) % 128);
    // udp->src_port = htons(port_write); 
    // if(lcore_id == 2) printf("src_port = %d\n", CLIENT_PORT + rx_queue_id);
    udp->dst_port = htons(dst_port);

    MessageHeader* message_header = (MessageHeader*) ((uint8_t*)eth + HEADER_TEMPLATE_SIZE);
    message_header->opType = opType;
    message_header->lockMode = lockMode;
    // message_header->clientID = htons(txnInfo.cid);
    // message_header->lockID = htonl(lockInfo.lock_id);
    message_header->lockID = htonl(lockID);
    // message_header->txnID = htonl(txnInfo.txn_id);
    message_header->txnID = htonl(txnID);
    message_header->clientID = htons(clientID);
    if (timestamp == 0) {
        message_header->timestamp = rte_rdtsc();
    } else {
        message_header->timestamp = timestamp;
    }
    message_header->padding = 0xffff;
    //printf("send message_header->payload = %lu\n", message_header->payload);

    // message_header->clientID = txnInfo.cid;
    

    mbuf->data_len += sizeof(MessageHeader);
    mbuf->pkt_len += sizeof(MessageHeader);


}

static void compute_latency(struct latency_statistics *latency_stat,
    uint64_t latency) {
    latency_stat->num++;
    latency_stat->total += latency;
    // printf("latency_num = %d, latency_total = %"PRIu64"\n", latency_stat->num, latency_stat->total);
    if(latency_stat->max < latency) {
        latency_stat->max = latency;
    }
    if(latency < BIN_MAX) {
        latency_stat->bin[latency/BIN_RANGE]++;
    } else {
        latency_stat->overflow++;
    } 
}

int mapping_func(var_obj obj){
    return obj; //单台backend node时候使�?，�?�台则需要划�?
}

// TX loop for test, fixed write rate
static int32_t np_client_txrx_loop(uint32_t lcore_id, uint32_t client_id, uint16_t rx_queue_id) {
    printf("%lld entering TX loop for write on lcore %u, numa_id_of_lcore = %d\n", (long long)time(NULL), lcore_id, rte_socket_id());

    //struct_lockInfo lockInTxn_arr[MAX_LOCK_IN_TXN];
    //int getRespFlag_arr[MAX_LOCK_IN_TXN];
    // struct_lockInfo * lockInTxn_arr = (struct_lockInfo *)malloc(sizeof(struct_lockInfo) * MAX_LOCK_IN_TXN);
    // int * getRespFlag_arr = (int *)malloc(sizeof(int) * MAX_LOCK_IN_TXN); 

    struct_txnInfo txn_arr;

    // initiate header_template_local
    uint8_t header_template_local[HEADER_TEMPLATE_SIZE];
    init_header_template_local(header_template_local);
    // initiate lconf
    struct lcore_configuration *lconf = &lcore_conf[lcore_id];
    // memory related points
    struct rte_mbuf *mbuf;
    struct rte_mbuf *mbuf2;
    
    // time related configurations 
    uint64_t cur_tsc = rte_rdtsc();
    uint64_t ms_tsc = rte_get_tsc_hz() / 1000; 
    uint64_t next_ms_tsc = cur_tsc + ms_tsc;
    uint64_t pkts_send_ms = 0;

    // mine
    uint32_t cid = client_id;
    int stepFlag = 0;
    uint64_t resend_check_interval_us = rte_get_tsc_hz() / 1000000 * RESEND_CHECK_INTERVAL_US;
    uint64_t deadlock_check_interval_us = rte_get_tsc_hz() / 1000000 * DEADLOCK_CHECK_INTERVAL_US;
    uint64_t padding_us = rte_get_tsc_hz() / 1000000 * RESEND_CHECK_INTERVAL_US;
    uint64_t next_deadlock_check_tsc = 0;

    struct rte_mbuf *mbuf_burst[NC_MAX_BURST_SIZE];
    struct rte_mbuf *mbuf_rcv;
    int x;

    // for(int i = 0; i < MAX_LOCK_IN_TXN; i++){
    //     getRespFlag_arr[i] = RESPFLAG_IDLE;
    // }
    
    while (1) {
        // read current time
        cur_tsc = rte_rdtsc();
        //usleep(1);

        if (unlikely(cur_tsc > next_ms_tsc)) {
            pkts_send_ms = 0;
            next_ms_tsc += ms_tsc;
        } 

        // RX
        for (int i = 0; i < lconf->n_rx_queue; i++) {
            uint32_t nb_rx = rte_eth_rx_burst(lconf->port, lconf->rx_queue_list[i], mbuf_burst, NC_MAX_BURST_SIZE);
            tput_stat[lcore_id].rx += nb_rx;
            for (int j = 0; j < nb_rx; j++) {
            
                mbuf_rcv = mbuf_burst[j];
                rte_prefetch0(rte_pktmbuf_mtod(mbuf_rcv, void *)); 
                struct rte_ether_hdr* eth = rte_pktmbuf_mtod(mbuf_rcv, struct rte_ether_hdr *);
                struct rte_ipv4_hdr *ip = (struct rte_ipv4_hdr *)((uint8_t*) eth + sizeof(struct rte_ether_hdr));
                struct rte_udp_hdr *udp = (struct rte_udp_hdr *)((uint8_t*) ip + sizeof(struct rte_ipv4_hdr));
                uint32_t srcIP = ip->src_addr;
                uint32_t dstIP = ip->dst_addr;
                // char str_srcIP[INET_ADDRSTRLEN];
                // char *ptr = inet_ntop(AF_INET,&foo.sin_addr, str, sizeof(str)); 

                if(ntohs(eth->ether_type) != ether_type_IPV4){
                    printf("---------------data rcv ERROR!!!!!!: error ether type = %x!---------------\n", ntohs(eth->ether_type));
                    //return 1;                    
                }

                //printf("rx_queue_id = %d, dst_port = %d, src_port = %d\n",lconf->rx_queue_list[i], ntohs(udp->dst_port), ntohs(udp->src_port));

                // parse NetLock header,could be changed
                MessageHeader* message_header = (MessageHeader*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE);
                uint8_t mode = (uint8_t) message_header->lockMode;
                uint8_t opt = (uint8_t) message_header->opType;
                uint32_t txn_id = ntohl(message_header->txnID);
                uint32_t lock_id = ntohl(message_header->lockID);
                uint16_t cid = ntohs(message_header->clientID);
                // uint32_t txn_id = message_header->txnID;
                // uint32_t lock_id = message_header->lockID;
                // uint16_t cid = message_header->clientID;
                float latency = timediff_in_us(rte_rdtsc(), message_header->timestamp);

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
                if (pkts_send_ms < wpkts_send_limit_ms){
                    pkts_send_ms++;

                    service_statistic_update(lock_id, rx_queue_id - 1, mapping_func);
                    // //CTRL!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!: count lock 
                    // if(!lock_stat_flag){
                    //     //DEBUG! 4500000 NOT COUNT
                    //     if(lock_id != 4500000){
                    //         if(clear_flag){
                    //             hotCounter[1][lock_id] ++;
                    //         }
                    //         else{
                    //             hotCounter[0][lock_id] ++;
                    //         }
                    //     }
                    // }
                    // else{
                    //     heapNode tempNode;
                    //     tempNode.lockId = lock_id;
                    //     if(clear_flag){
                    //         tempNode.lockCounter = hotCounter[1][lock_id];   
                    //     }
                    //     else{
                    //         tempNode.lockCounter = hotCounter[0][lock_id]; 
                    //     }
                    //     bool jump_flag = 0;
                    //     if(clear_TOPK_flag){
                    //         for(int k=1; k<=TOPK; k++){
                    //             if(TopKNode[1][rx_queue_id - 1][k].lockId == tempNode.lockId){
                    //                 jump_flag = 1;
                    //                 break;
                    //             }
                    //         }
                    //         for(int m=0; m<last_hotNum; m++){
                    //             if(last_hotID[m] == tempNode.lockId){
                    //                 jump_flag = 1;
                    //                 break;                                
                    //             }
                    //         }
                    //         if(!jump_flag && tempNode.lockCounter > TopKNode[1][rx_queue_id - 1][1].lockCounter){ //TOPKCOLD的话大于号需要改成小于号
                    //             TopKNode[1][rx_queue_id - 1][1] = tempNode;
                    //             downAdjust(TopKNode[1][rx_queue_id - 1], 1, TOPK);
                    //         }
                    //     }
                    //     else{
                    //         for(int k=1; k<=TOPK; k++){
                    //             if(TopKNode[0][rx_queue_id - 1][k].lockId == tempNode.lockId){
                    //                 jump_flag = 1;
                    //                 break;
                    //             }
                    //         }
                    //         for(int m=0; m<last_hotNum; m++){
                    //             if(last_hotID[m] == tempNode.lockId){
                    //                 jump_flag = 1;
                    //                 break;                                
                    //             }
                    //         }
                    //         if(!jump_flag && tempNode.lockCounter > TopKNode[0][rx_queue_id - 1][1].lockCounter){ //TOPKCOLD的话大于号需要改成小于号
                    //             TopKNode[0][rx_queue_id - 1][1] = tempNode;
                    //             downAdjust(TopKNode[0][rx_queue_id - 1], 1, TOPK);
                    //         }
                    //     }
                    // }
                    //------------------------------------------------------------------------------
                    //------------------------------------------------------------------------------

                    // if(lock_id < 0 || lock_id > (MAX_LOCK_NUM - 1))
                    //     printf("lock_id ERROR!! rcv lock_id:%d, txn_id:%d, cid:%d, opType:%d, mode:%d, latency:%f\n", lock_id, txn_id, cid, opt, mode, latency);
                    // if(lock_id % 23 != (rx_queue_id - 1))
                    //     printf("queue ERROR!! rcv lock_id:%d, rx_queue_id:%d, dst_port = %d, src_port = %d opType:%d, mode:%d, dst_ip = %d, src_ip = %d\n", lock_id, rx_queue_id, ntohs(udp->dst_port), ntohs(udp->src_port), opt, mode, dstIP, srcIP);

                    compute_latency(&latency_stat_c[lcore_id], latency);
                    if (latency_sample_start == 1) {
                        if (latency_stat_c[lcore_id].num % latency_sample_interval == 0) {
                            // printf("latency_stat_c[lcore_id].num = %d\n", latency_stat_c[lcore_id].num);
                            latency_samples[lcore_id][latency_sample_num[lcore_id]] = latency;
                            latency_sample_num[lcore_id]++;
                        }
                    }


                    //添加了�?�于�??重入的支�??
                    if (opt == OP_ACQUIRE_LOCK) {
                        //跟switch上一样是有个缓存队列的，存着并不代表正在�??处理
                        //发送地址有问题，还需要debug
                        //这个queue�??缓存队列，并不是实际建立的锁的队�??
                        //----------------------------------------------------------
                        // rte_spinlock_lock(&lock[lock_id]);
                        // 从新的队列头部得到�?�应锁的指针（即下一�??待�?�理锁）
                        queue_node *ptr = (&(lockqueues[lock_id]))->head;
                        //printf("-----------------------------DEBUG step 1---------------------------\n"); 

                        //CTRL!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

                        int item_idx = 1;
                        int reEntry_flag = 0;
                        int acquired_lockNum = num_ex_granted[lock_id] + num_sh_granted[lock_id]; 
                        //CTRL!!!!!!!!!!!!!!! reentry close
                        while (ptr != NULL) {
                            //DEBUG_PRINT("lock_id:%d, txn_id:%d, ip_src_addr:%d, cid:%d\n", lock_id, ntohl(ptr->txnID), ptr->ip_src_addr, ptr->client_id);
                            if(ptr->mode == mode && ptr->client_id == cid){
                                if(item_idx <= acquired_lockNum){
                                    mbuf = rte_pktmbuf_alloc(pktmbuf_pool);
                                    generate_write_request_pkt_server(rx_queue_id, header_template_local, lcore_id, mbuf, OP_GRANT_SUCCESS, mode, cid, lock_id, txn_id, dstIP, srcIP, ntohs(udp->src_port), message_header->timestamp);
                                    //generate_write_request_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, lockInfo, txnInfo, ip_src_pton, ip_dst_pton, cur_tsc);
                                    //enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                                    enqueue_pkt_with_thres(lcore_id, mbuf, NC_MAX_BURST_SIZE, 0);
                                    tput_stat[lcore_id].tx_normal += 1;
                                    reEntry_flag = 1;
                                    break;
                                }
                                else{
                                    mbuf = rte_pktmbuf_alloc(pktmbuf_pool);
                                    generate_write_request_pkt_server(rx_queue_id, header_template_local, lcore_id, mbuf, OP_ENQUEUE_SUCCESS, mode, cid, lock_id, txn_id, dstIP, srcIP, ntohs(udp->src_port), message_header->timestamp);
                                    //generate_write_request_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, lockInfo, txnInfo, ip_src_pton, ip_dst_pton, cur_tsc);
                                    //enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                                    enqueue_pkt_with_thres(lcore_id, mbuf, NC_MAX_BURST_SIZE, 0);
                                    tput_stat[lcore_id].tx_normal += 1;
                                    reEntry_flag = 1;
                                    break;
                                }
                            }
                            ptr = ptr->next;
                            item_idx++;                     
                        }   
                        if(reEntry_flag == 0){
                            //------------------------------------------------------------------------------
                            //------------------------------------------------------------------------------
                            if(service_object_state_get(lock_id) != OBJSTATE_IDLE){
                            // if(check_queue_lock_flag && queueLockFlag[lock_id] == 1){ // queueLockFlag[lock_id]执�?�状态�?��?
                            //------------------------------------------------------------------------------
                            //------------------------------------------------------------------------------
                                mbuf = rte_pktmbuf_alloc(pktmbuf_pool);
                                generate_write_request_pkt_server(rx_queue_id, header_template_local, lcore_id, mbuf, OP_GRANT_FAILED, mode, cid, lock_id, txn_id, dstIP, srcIP, ntohs(udp->src_port), message_header->timestamp);
                                enqueue_pkt_with_thres(lcore_id, mbuf, NC_MAX_BURST_SIZE, 0);     
                                tput_stat[lcore_id].tx_resend += 1;                            
                            }
                            // 在这�?分支里�?�查入队操作是否成�?
                            else if (get_queue_size(&(lockqueues[lock_id])) > 3 || enqueue(&(lockqueues[lock_id]), mode, cid, ntohs(udp->src_port), txn_id, srcIP) != 0) {
                                //fprintf(stderr, "ENQUEUE ERROR.\n");
                                mbuf = rte_pktmbuf_alloc(pktmbuf_pool);
                                generate_write_request_pkt_server(rx_queue_id, header_template_local, lcore_id, mbuf, OP_GRANT_FAILED, mode, cid, lock_id, txn_id, dstIP, srcIP, ntohs(udp->src_port), message_header->timestamp);
                                enqueue_pkt_with_thres(lcore_id, mbuf, NC_MAX_BURST_SIZE, 0);     
                                tput_stat[lcore_id].tx_resend += 1;                       
                            }
                            else{
                                // rte_spinlock_unlock(&lock[lock_id]);
                                if (mode == SHARED_LOCK) {
                                    if (num_ex[lock_id] == 0) { //如果当前lockid并没有�?�在建立�??的exclusive lock，那么�?�为这个shared lock�??成功建立了，所以发出返回包
                                        //DEBUG_PRINT("RCV&SND: lock_id:%d, txn_id:%d, mac_src_addr_part:%d, ip_src_addr:%d, cid:%d\n", lock_id, ntohl(message_header->txnID), mac_src_addr_part, ip->src_addr, message_header->client_id);
                                        mbuf = rte_pktmbuf_alloc(pktmbuf_pool);
                                        generate_write_request_pkt_server(rx_queue_id, header_template_local, lcore_id, mbuf, OP_GRANT_SUCCESS, mode, cid, lock_id, txn_id, dstIP, srcIP, ntohs(udp->src_port), message_header->timestamp);
                                        //generate_write_request_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, lockInfo, txnInfo, ip_src_pton, ip_dst_pton, cur_tsc);
                                        //enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                                        enqueue_pkt_with_thres(lcore_id, mbuf, NC_MAX_BURST_SIZE, 0);
                                        /*
                                        * Here, rx_read marks the rate server grants the lock,
                                        * rx_write marks the rate server pushs locks back to switch
                                        */
                                        tput_stat[lcore_id].tx_normal += 1;
                                        num_sh_granted[lock_id] ++;
                                    }
                                    else{
                                        mbuf = rte_pktmbuf_alloc(pktmbuf_pool);
                                        generate_write_request_pkt_server(rx_queue_id, header_template_local, lcore_id, mbuf, OP_ENQUEUE_SUCCESS, mode, cid, lock_id, txn_id, dstIP, srcIP, ntohs(udp->src_port), message_header->timestamp);
                                        //generate_write_request_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, lockInfo, txnInfo, ip_src_pton, ip_dst_pton, cur_tsc);
                                        //enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                                        enqueue_pkt_with_thres(lcore_id, mbuf, NC_MAX_BURST_SIZE, 0);  
                                        tput_stat[lcore_id].tx_resend += 1;                                                                  
                                    }
                                    num_sh[lock_id] ++;
                                    queueNotEmptyFlag[lock_id] = 1;
                                } else if (mode == EXCLUSIVE_LOCK) {
                                    if ((num_ex[lock_id] == 0) && (num_sh[lock_id] == 0)) {//如果当前lockid并没有�?�在建立�??的任何lock，那么�?�为这个exclusive lock�??成功建立了，所以发出返回包
                                        //DEBUG_PRINT("RCV&SND: lock_id:%d, txn_id:%d, mac_src_addr_part:%d, ip_src_addr:%d, cid:%d\n", lock_id, ntohl(message_header->txnID), mac_src_addr_part, ip->src_addr, message_header->client_id);
                                        mbuf = rte_pktmbuf_alloc(pktmbuf_pool);
                                        generate_write_request_pkt_server(rx_queue_id, header_template_local, lcore_id, mbuf, OP_GRANT_SUCCESS, mode, cid, lock_id, txn_id, dstIP, srcIP, ntohs(udp->src_port), message_header->timestamp);
                                        //generate_write_request_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, lockInfo, txnInfo, ip_src_pton, ip_dst_pton, cur_tsc);
                                        //enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                                        enqueue_pkt_with_thres(lcore_id, mbuf, NC_MAX_BURST_SIZE, 0);
                                        /*
                                        * Here, rx_read marks the rate server grants the lock,
                                        * rx_write marks the rate server pushs locks back to switch
                                        */
                                        tput_stat[lcore_id].tx_normal += 1;
                                        num_ex_granted[lock_id] ++;
                                    }
                                    else{
                                        mbuf = rte_pktmbuf_alloc(pktmbuf_pool);
                                        generate_write_request_pkt_server(rx_queue_id, header_template_local, lcore_id, mbuf, OP_ENQUEUE_SUCCESS, mode, cid, lock_id, txn_id, dstIP, srcIP, ntohs(udp->src_port), message_header->timestamp);
                                        //generate_write_request_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, lockInfo, txnInfo, ip_src_pton, ip_dst_pton, cur_tsc);
                                        //enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                                        enqueue_pkt_with_thres(lcore_id, mbuf, NC_MAX_BURST_SIZE, 0);   
                                        tput_stat[lcore_id].tx_resend += 1;                                
                                    }
                                    num_ex[lock_id] ++;
                                    queueNotEmptyFlag[lock_id] = 1;
                                }
                            }

                        }                        

                    }                
                    else if (opt == OP_RELEASE_LOCK) {  
                        uint8_t head_mode;
                        uint16_t head_cid;
                        uint16_t head_srcPort;
                        uint32_t head_txnID;
                        uint32_t head_srcIP;
                        queue_node *check_ptr = (&(lockqueues[lock_id]))->head;

                        //------------------------------------------------------------------------------
                        //------------------------------------------------------------------------------
                        // 这里也需要queueLockFlag[lock_id]执�?�状态�?��?
                        if(service_object_state_get(lock_id) != OBJSTATE_IDLE){
                            mbuf = rte_pktmbuf_alloc(pktmbuf_pool);
                            generate_write_request_pkt_server(rx_queue_id, header_template_local, lcore_id, mbuf, OP_RELEASE_FAILED, mode, cid, lock_id, txn_id, dstIP, srcIP, ntohs(udp->src_port), message_header->timestamp);
                            enqueue_pkt_with_thres(lcore_id, mbuf, NC_MAX_BURST_SIZE, 0);   
                            tput_stat[lcore_id].tx_resend += 1;    
                            continue; 
                        }
                        //------------------------------------------------------------------------------
                        //------------------------------------------------------------------------------

                        //把队列最前头的待处理锁从队列�??拿出�??,作为当前锁，放进前面定义的变量里（队列头指针发生变化�??
                        //----------------------------------------------------------
                        // rte_spinlock_lock(&lock[lock_id]);
                        //static void dequeue(queue_list *q, uint8_t* mode, uint16_t* client_id, uint16_t* srcPort, uint32_t* txnID, uint32_t* srcIP)
                        //dequeue(&(lockqueues[lock_id]), &head_mode, &txn_id, &mac_src_addr_part, &ip_src_addr, &timestamp, &cid);
                        //队列�??没有等待�??的锁
                        // if (dequeue(&(lockqueues[lock_id]), &head_mode, &head_cid, &head_srcPort, &head_txnID, &head_srcIP) != 0) {
                        //     //表示release失败，发送OP_RELEASE_FAILED返回�??
                        //     mbuf = rte_pktmbuf_alloc(pktmbuf_pool);
                        //     generate_write_request_pkt_server(rx_queue_id, header_template_local, lcore_id, mbuf, OP_RELEASE_FAILED, mode, cid, lock_id, txn_id, ip_src_pton, ip_dst_pton, ntohs(udp->src_port), message_header->timestamp);
                        //     //generate_write_request_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, lockInfo, txnInfo, ip_src_pton, ip_dst_pton, cur_tsc);
                        //     //enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                        //     enqueue_pkt_with_thres(lcore_id, mbuf, NC_MAX_BURST_SIZE, 0); 
                        //     tput_stat[lcore_id].tx_resend += 1;   
                        // }
                        // rte_spinlock_unlock(&lock[lock_id]);
                        // if(head_mode != mode){
                        //     //表示release失败，发送OP_RELEASE_FAILED返回�??
                        //     mbuf = rte_pktmbuf_alloc(pktmbuf_pool);
                        //     generate_write_request_pkt_server(rx_queue_id, header_template_local, lcore_id, mbuf, OP_RELEASE_FAILED, mode, cid, lock_id, txn_id, ip_src_pton, ip_dst_pton, ntohs(udp->src_port), message_header->timestamp);
                        //     //generate_write_request_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, lockInfo, txnInfo, ip_src_pton, ip_dst_pton, cur_tsc);
                        //     //enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                        //     enqueue_pkt_with_thres(lcore_id, mbuf, NC_MAX_BURST_SIZE, 0);                        
                        // }

                        // 从新的队列头部得到�?�应锁的指针（即下一�??待�?�理锁）
                        //queue_node *ptr = (&(lockqueues[lock_id]))->head;
                        int release_succ = 0;
                        int release_idx = 1;
                        int granted_lockNum = num_ex_granted[lock_id] + num_sh_granted[lock_id]; 
                        // while (check_ptr != NULL) {
                        //     if(check_ptr->mode == mode && check_ptr->client_id == cid){
                        //         if(release_idx <= granted_lockNum){
                        //             release_succ = 1;
                        //             break;
                        //         }
                        //     }
                        //     check_ptr = check_ptr->next;
                        //     release_idx++;                            
                        // }
                        if (check_ptr != NULL) {
                            if(check_ptr->mode == mode && check_ptr->client_id == cid){
                                    release_succ = 1;
                            }
                            else if(check_ptr->mode == mode && mode == SHARED_LOCK){
                                    release_succ = 1;
                            }                            
                        }
                        if(release_succ == 0){
                            // if(check_queue_lock_flag && (lock_id == hotID[0] || lock_id == hotID[1] || lock_id == hotID[2] || lock_id == hotID[3] || lock_id == hotID[4] || lock_id == hotID[5] || lock_id == hotID[6] || lock_id == hotID[7])){
                            //     printf("828: ERROR! core%d OP_RELEASE_FAILED, lockID = %d\n", lcore_id, lock_id);
                            // }
                            //表示release失败，发送OP_RELEASE_FAILED返回�??
                            mbuf = rte_pktmbuf_alloc(pktmbuf_pool);
                            generate_write_request_pkt_server(rx_queue_id, header_template_local, lcore_id, mbuf, OP_RELEASE_FAILED, mode, cid, lock_id, txn_id, dstIP, srcIP, ntohs(udp->src_port), message_header->timestamp);
                            //generate_write_request_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, lockInfo, txnInfo, ip_src_pton, ip_dst_pton, cur_tsc);
                            //enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                            enqueue_pkt_with_thres(lcore_id, mbuf, NC_MAX_BURST_SIZE, 0);   
                            tput_stat[lcore_id].tx_resend += 1;                            
                        }                           
                        else if (mode == SHARED_LOCK) {//当前锁的类型是SHARED_LOCK
                            // if(check_queue_lock_flag && (lock_id == hotID[0] || lock_id == hotID[1] || lock_id == hotID[2] || lock_id == hotID[3] || lock_id == hotID[4] || lock_id == hotID[5] || lock_id == hotID[6] || lock_id == hotID[7])){
                            //     printf("828: core%d queue_size_of_%d = %d\n", lcore_id, lock_id, get_queue_size(&(lockqueues[lock_id])));
                            // }
                            dequeue(&(lockqueues[lock_id]), &head_mode, &head_cid, &head_srcPort, &head_txnID, &head_srcIP);
                            // if(check_queue_lock_flag && (lock_id == hotID[0] || lock_id == hotID[1] || lock_id == hotID[2] || lock_id == hotID[3] || lock_id == hotID[4] || lock_id == hotID[5] || lock_id == hotID[6] || lock_id == hotID[7])){
                            //     printf("828: OP_RELEASE_SUCC, lockID = %d\n", lock_id);
                            //     printf("828: core%d queue_size_of_%d = %d\n", lcore_id, lock_id, get_queue_size(&(lockqueues[lock_id])));
                            // }
                            if(get_queue_size(&(lockqueues[lock_id])) == 0){
                                queueNotEmptyFlag[lock_id] = 0;
                            }
                            // 从新的队列头部得到�?�应锁的指针（即下一�??待�?�理锁）
                            queue_node *ptr = (&(lockqueues[lock_id]))->head;                        
                            num_sh[lock_id] --;
                            num_sh_granted[lock_id] --;
                            //表示release成功，发送OP_RELEASE_SUCCESS返回�??
                            mbuf = rte_pktmbuf_alloc(pktmbuf_pool);
                            generate_write_request_pkt_server(rx_queue_id, header_template_local, lcore_id, mbuf, OP_RELEASE_SUCCESS, mode, cid, lock_id, txn_id, dstIP, srcIP, ntohs(udp->src_port), message_header->timestamp);
                            //generate_write_request_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, lockInfo, txnInfo, ip_src_pton, ip_dst_pton, cur_tsc);
                            //enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                            enqueue_pkt_with_thres(lcore_id, mbuf, NC_MAX_BURST_SIZE, 0);
                            tput_stat[lcore_id].tx_normal += 1;

                            if ((ptr != NULL) && (ptr->mode == EXCLUSIVE_LOCK)) {//下一待�?�理锁是EXCLUSIVE_LOCK
                                // notify the client
                                //DEBUG_PRINT("RCV&SND: lock_id:%d, txn_id:%d, mac_src_addr_part:%d, ip_src_addr:%d, cid:%d\n", lock_id, ntohl(ptr->txnID), ptr->mac_src_addr_part , ptr->ip_src_addr, ptr->client_id);

                                //表示授予成功，发送OP_GRANT_SUCCESS返回�??
                                mbuf = rte_pktmbuf_alloc(pktmbuf_pool);
                                generate_write_request_pkt_server(rx_queue_id, header_template_local, lcore_id, mbuf, OP_GRANT_SUCCESS, ptr->mode, ptr->client_id, lock_id, ptr->txnID, dstIP, ptr->srcIP, ptr->srcPort, message_header->timestamp);
                                //generate_write_request_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, lockInfo, txnInfo, ip_src_pton, ip_dst_pton, cur_tsc);
                                //enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                                enqueue_pkt_with_thres(lcore_id, mbuf, NC_MAX_BURST_SIZE, 0);
                                /*
                                * Here, rx_read marks the rate server grants the lock,
                                * rx_write marks the rate server pushs locks back to switch
                                */ 
                                tput_stat[lcore_id].tx_normal += 1;
                                num_ex_granted[lock_id] ++;
                            }
                        } else if (mode == EXCLUSIVE_LOCK) {
                            dequeue(&(lockqueues[lock_id]), &head_mode, &head_cid, &head_srcPort, &head_txnID, &head_srcIP);
                            // 从新的队列头部得到�?�应锁的指针（即下一�??待�?�理锁）
                            if(get_queue_size(&(lockqueues[lock_id])) == 0){
                                queueNotEmptyFlag[lock_id] = 0;
                            }
                            queue_node *ptr = (&(lockqueues[lock_id]))->head;
                            num_ex[lock_id] --;
                            num_ex_granted[lock_id] --;
                            //表示release成功，发送OP_RELEASE_SUCCESS返回�??
                            mbuf = rte_pktmbuf_alloc(pktmbuf_pool);
                            generate_write_request_pkt_server(rx_queue_id, header_template_local, lcore_id, mbuf, OP_RELEASE_SUCCESS, mode, cid, lock_id, txn_id, dstIP, srcIP, ntohs(udp->src_port), message_header->timestamp);
                            //generate_write_request_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, lockInfo, txnInfo, ip_src_pton, ip_dst_pton, cur_tsc);
                            //enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                            enqueue_pkt_with_thres(lcore_id, mbuf, NC_MAX_BURST_SIZE, 0);
                            tput_stat[lcore_id].tx_normal += 1;

                            if ((ptr != NULL) && (ptr->mode == EXCLUSIVE_LOCK)) {
                                // notify the client
                                //DEBUG_PRINT("RCV&SND: lock_id:%d, txn_id:%d, mac_src_addr_part:%d, ip_src_addr:%d, cid:%d\n", lock_id, ntohl(ptr->txnID), ptr->mac_src_addr_part , ptr->ip_src_addr, ptr->client_id);

                                //表示授予成功，发送OP_GRANT_SUCCESS返回�??
                                mbuf = rte_pktmbuf_alloc(pktmbuf_pool);
                                generate_write_request_pkt_server(rx_queue_id, header_template_local, lcore_id, mbuf, OP_GRANT_SUCCESS, ptr->mode, ptr->client_id, lock_id, ptr->txnID, dstIP, ptr->srcIP, ptr->srcPort, message_header->timestamp);
                                //generate_write_request_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, lockInfo, txnInfo, ip_src_pton, ip_dst_pton, cur_tsc);
                                //enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                                enqueue_pkt_with_thres(lcore_id, mbuf, NC_MAX_BURST_SIZE, 0);
                                /*
                                * Here, rx_read marks the rate server grants the lock,
                                * rx_write marks the rate server pushs locks back to switch
                                */
                                tput_stat[lcore_id].tx_normal += 1;
                                num_ex_granted[lock_id] ++;
                            }
                            else {
                                while ((ptr != NULL) && (ptr->mode == SHARED_LOCK)) {
                                    //DEBUG_PRINT("lock_id:%d, txn_id:%d, ip_src_addr:%d, cid:%d\n", lock_id, ntohl(ptr->txnID), ptr->ip_src_addr, ptr->client_id);

                                    //表示授予成功，发送OP_GRANT_SUCCESS返回�??
                                    mbuf = rte_pktmbuf_alloc(pktmbuf_pool);
                                    generate_write_request_pkt_server(rx_queue_id, header_template_local, lcore_id, mbuf, OP_GRANT_SUCCESS, ptr->mode, ptr->client_id, lock_id, ptr->txnID, dstIP, ptr->srcIP, ptr->srcPort, message_header->timestamp);
                                    //generate_write_request_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, lockInfo, txnInfo, ip_src_pton, ip_dst_pton, cur_tsc);
                                    //enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                                    enqueue_pkt_with_thres(lcore_id, mbuf, NC_MAX_BURST_SIZE, 0);
                                    ptr = ptr->next;
                                    /*
                                    * Here, rx_read marks the rate server grants the lock,
                                    * rx_write marks the rate server pushs locks back to switch
                                    */
                                    tput_stat[lcore_id].tx_normal += 1;
                                    num_sh_granted[lock_id] ++;
                                }
                            }
                        }



                    }
                }

                rte_pktmbuf_free(mbuf_rcv);
            }

            enqueue_pkt_with_thres(lcore_id, mbuf, 1, 1);
        }

 
    }
    
    for (int i=0; i<MAX_LOCK_NUM; i++) {
        queue_clear(&(lockqueues[i]));
    }
    return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// static int32_t np_client_statistic_loop(uint32_t lcore_id, uint16_t rx_queue_id) {
//     printf("%lld entering RX loop (master loop) on lcore %u, numa_id_of_lcore = %d\n", (long long)time(NULL), lcore_id, rte_socket_id());
//     // initiate header_template_local_ctrl
//     uint8_t header_template_local_ctrl[HEADER_TEMPLATE_SIZE_CTRL];
//     init_header_template_local_ctrl(header_template_local_ctrl);
//     // initiate lconf
//     struct lcore_configuration *lconf = &lcore_conf[lcore_id];
//     // memory related points
//     struct rte_mbuf *mbuf;
//     struct rte_mbuf *mbuf_burst[NC_MAX_BURST_SIZE];
//     struct rte_mbuf *mbuf_rcv;
//     // time related configurations
//     uint64_t cur_tsc = rte_rdtsc();
//     uint64_t ms_tsc = rte_get_tsc_hz() / 1000;
//     uint64_t next_ms_tsc = cur_tsc + ms_tsc;
//     uint64_t update_tsc = rte_get_tsc_hz(); // in second
//     uint64_t next_update_tsc = cur_tsc + update_tsc;
//     uint64_t average_start_tsc = cur_tsc + update_tsc * average_interval;
//     uint64_t average_end_tsc = cur_tsc + update_tsc * average_interval * 2;
//     uint64_t exit_tsc = cur_tsc + update_tsc * average_interval * 20;
//     // stop flags
//     uint32_t pkts_send_ms = 0;
//     int stop_statis = 0;

//     int flag = 0;
//     int temp_idx = 0;
 
//     while (1) {
// //-----------------------------------------------------------process for ctrl-----------------------------------------------------------
//         switch(stats_stage){
//             case STATS_LOCK_STAT: { 
//                     // int temp_size = get_queue_size(&(lockqueues[0]));
//                     // temp_idx++;
//                     // if(temp_size == 0){
//                     //     printf("828: %d-queue_size of lock0 = %d\n",temp_idx,temp_size);
//                     // }
//                     for (int i = 0; i < lconf->n_rx_queue; i++) {
//                         uint32_t nb_rx = rte_eth_rx_burst(lconf->port, lconf->rx_queue_list[i], mbuf_burst, NC_MAX_BURST_SIZE);
//                         for (int j = 0; j < nb_rx; j++) {
//                             //printf("826: STATS_LOCK_STAT starts\n");
//                             mbuf_rcv = mbuf_burst[j];
//                             rte_prefetch0(rte_pktmbuf_mtod(mbuf_rcv, void *));     
//                             struct rte_ether_hdr* eth = rte_pktmbuf_mtod(mbuf_rcv, struct rte_ether_hdr *);
//                             // printf("829: stage = %d, ether_type = %d\n", stats_stage, ntohs(eth->ether_type));
//                             if(ntohs(eth->ether_type) == ether_type_fpga){
//                                 CtrlHeader_switch* message_header = (CtrlHeader_switch*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE_CTRL);
//                                 if(message_header->opType == OP_LOCK_STAT){
//                                     lock_stat_flag = 1;
//                                     //lock stat response
//                                     mbuf = rte_pktmbuf_alloc(pktmbuf_pool);
//                                     generate_ctrl_pkt(rx_queue_id, header_template_local_ctrl, lcore_id, mbuf, ether_type_fpga, OP_LOCK_STAT);
//                                     enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
//                                     stats_stage = STATS_GET_HOT;
//                                     //printf("826: send OP_LOCK_STAT\n");
//                                     //printf("826: OP_LOCK_STAT ok!\n");
//                                 }
//                             }
//                             else{
//                                 printf("---------------ERROR!!!!!!: error ether type = %x!---------------\n", ntohs(eth->ether_type));
//                                 //return 1;
//                             }
//                             rte_pktmbuf_free(mbuf_rcv);
//                         }
//                     }
//                 break;
//             }

//             case STATS_GET_HOT: { 
//                     for (int i = 0; i < lconf->n_rx_queue; i++) {
//                         uint32_t nb_rx = rte_eth_rx_burst(lconf->port, lconf->rx_queue_list[i], mbuf_burst, NC_MAX_BURST_SIZE);
//                         for (int j = 0; j < nb_rx; j++) {
//                             //printf("826: STATS_GET_HOT starts\n");
//                             mbuf_rcv = mbuf_burst[j];
//                             rte_prefetch0(rte_pktmbuf_mtod(mbuf_rcv, void *));     
//                             struct rte_ether_hdr* eth = rte_pktmbuf_mtod(mbuf_rcv, struct rte_ether_hdr *);
//                             // printf("829: stage = %d, ether_type = %d\n", stats_stage, ntohs(eth->ether_type));
//                             if(ntohs(eth->ether_type) == ether_type_fpga){
//                                 CtrlHeader_switch* message_header = (CtrlHeader_switch*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE_CTRL);
//                                 if(message_header->opType == OP_GET_HOT){
//                                     if(clear_TOPK_flag){
//                                         for(int m=0; m<NUM_SERVER_CORE; m++){
//                                             for(int n=0; n<TOPK; n++){
//                                                 merge_TopKNode[m*TOPK + n] = TopKNode[1][m][n+1];
//                                                 TopKNode[0][m][n+1].lockCounter = 0;
//                                                 TopKNode[0][m][n+1].lockId = 4294967295;
//                                             }                                   
//                                         }
//                                     }
//                                     else{
//                                         for(int m=0; m<NUM_SERVER_CORE; m++){
//                                             for(int n=0; n<TOPK; n++){
//                                                 merge_TopKNode[m*TOPK + n] = TopKNode[0][m][n+1];
//                                                 TopKNode[1][m][n+1].lockCounter = 0;
//                                                 TopKNode[1][m][n+1].lockId = 4294967295;
//                                             }                                   
//                                         }                                        
//                                     }
//                                     //clear_TOPK_flag = !clear_TOPK_flag;
//                                     qsort(merge_TopKNode, NUM_SERVER_CORE * TOPK, sizeof(heapNode), inc);
//                                     mbuf = rte_pktmbuf_alloc(pktmbuf_pool);
//                                     generate_ctrl_pkt_hotReport(rx_queue_id, header_template_local_ctrl, lcore_id, mbuf, ether_type_fpga, OP_GET_HOT, merge_TopKNode);
//                                     enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
//                                     //printf("826: send OP_GET_HOT\n");
//                                     //DONE: get hotInfo, send to controller
//                                     stats_stage = STATS_HOT_REPORT;
//                                 }
//                                 else if(message_header->opType == OP_CLR_COLD_COUNTER){
//                                     if(clear_flag){
//                                         for(int k = 0; k < MAX_LOCK_NUM; k++){
//                                             hotCounter[0][k] = 0;
//                                         }
//                                     }
//                                     else{
//                                         for(int k = 0; k < MAX_LOCK_NUM; k++){
//                                             hotCounter[1][k] = 0;
//                                         }
//                                     }
//                                     clear_flag = !clear_flag;
//                                     //send clear done response
//                                     mbuf = rte_pktmbuf_alloc(pktmbuf_pool);
//                                     generate_ctrl_pkt(rx_queue_id, header_template_local_ctrl, lcore_id, mbuf, ether_type_fpga, OP_CLR_COLD_COUNTER);
//                                     enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
//                                     //printf("826: send OP_CLR_COLD_COUNTER\n");   
//                                     stats_stage = STATS_GET_HOT;                                 
//                                 }
//                                 else if(message_header->opType == OP_UNLOCK_STAT){
//                                     lock_stat_flag = 0;
//                                     //unlock stat response
//                                     mbuf = rte_pktmbuf_alloc(pktmbuf_pool);
//                                     generate_ctrl_pkt(rx_queue_id, header_template_local_ctrl, lcore_id, mbuf, ether_type_fpga, OP_UNLOCK_STAT);
//                                     enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
//                                     stats_stage = STATS_LOCK_STAT;
//                                     //printf("826: send OP_UNLOCK_STAT\n");                                  
//                                 }
//                             }
//                             else{
//                                 printf("---------------ERROR!!!!!!: error ether type = %x!---------------\n", ntohs(eth->ether_type));
//                                 //return 1;
//                             }
//                             rte_pktmbuf_free(mbuf_rcv);
//                         }
//                     }
//                 break;
//             }

//             case STATS_HOT_REPORT: { 
//                     for (int i = 0; i < lconf->n_rx_queue; i++) {
//                         uint32_t nb_rx = rte_eth_rx_burst(lconf->port, lconf->rx_queue_list[i], mbuf_burst, NC_MAX_BURST_SIZE);
//                         for (int j = 0; j < nb_rx; j++) {
//                             //printf("826: STATS_HOT_REPORT starts\n");
//                             mbuf_rcv = mbuf_burst[j];
//                             rte_prefetch0(rte_pktmbuf_mtod(mbuf_rcv, void *));     
//                             struct rte_ether_hdr* eth = rte_pktmbuf_mtod(mbuf_rcv, struct rte_ether_hdr *);
//                             // printf("829: stage = %d, ether_type = %d\n", stats_stage, ntohs(eth->ether_type));
//                             if(ntohs(eth->ether_type) == ether_type_fpga){
//                                 CtrlHeader_switch* message_header = (CtrlHeader_switch*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE_CTRL);
//                                 if(message_header->opType == OP_LOCK_HOT_QUEUE){
//                                     hotNum = ntohl(message_header->lockIdx);
//                                     for(int k=0; k<hotNum; k++){
//                                         hotID[k] = ntohl(message_header->lockCounter[k]);
//                                     }
//                                     if(hotNum == 0){
//                                         stats_stage = STATS_GET_HOT;
//                                     }
//                                     else{
//                                         stats_stage = STATS_WAIT_EMPTY;
//                                     }
//                                     //DONE: lock hot queue
//                                     mbuf = rte_pktmbuf_alloc(pktmbuf_pool);
//                                     generate_ctrl_pkt(rx_queue_id, header_template_local_ctrl, lcore_id, mbuf, ether_type_fpga, OP_LOCK_HOT_QUEUE);
//                                     enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
//                                 }
//                             }
//                             else{
//                                 printf("---------------ERROR!!!!!!: error ether type = %x!---------------\n", ntohs(eth->ether_type));
//                                 //return 1;
//                             }
//                             rte_pktmbuf_free(mbuf_rcv);
//                         }
//                     }
//                 break;
//             }

//             case STATS_WAIT_EMPTY: { 
//                     printf("826: STATS_WAIT_EMPTY starts\n");
//                     printf("826: hotNum = %d\n", hotNum);
//                     for(int k=0; k<hotNum; k++){
//                         printf("826: hotID = %d\n", hotID[k]);
//                         queueLockFlag[hotID[k]] = 1;                        
//                     }
//                     check_queue_lock_flag = 1;
//                     bool temp_queueEmptyFlag = 0;
//                     while (!temp_queueEmptyFlag){
//                         for(int k=0; k<hotNum; k++){
//                             //int temp_size = get_queue_size(&(lockqueues[hotID[k]]));
//                             //int temp_size = num_sh[hotID[k]] + num_ex[hotID[k]];
//                             //printf("828: queue len of hotID %d is %d\n", hotID[k], temp_size);
//                             // printf("828: num_sh of hotID %d = %d\n", hotID[k], num_sh[hotID[k]]);
//                             // printf("828: num_ex of hotID %d = %d\n", hotID[k], num_ex[hotID[k]]);
//                             if(num_sh[hotID[k]] + num_ex[hotID[k]] != 0){
//                             //if(queueNotEmptyFlag[hotID[k]] != 0){
//                                 //printf("828: hotID %d is not empty\n", hotID[k]);
//                                 temp_queueEmptyFlag = 0;
//                                 break;
//                             }
//                             else{
//                                 //printf("828: queue len of hotID %d is %d\n", hotID[k], temp_size);
//                                 temp_queueEmptyFlag = 1;
//                             }                        
//                         } 
//                         //usleep(1000);                       
//                     }
//                     //DONE: wait until hot queue empty
//                     mbuf = rte_pktmbuf_alloc(pktmbuf_pool);
//                     generate_ctrl_pkt(rx_queue_id, header_template_local_ctrl, lcore_id, mbuf, ether_type_fpga, OP_CALLBACK);
//                     enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
//                     stats_stage = STATS_WAIT_COMPLETE;                  
//                 break;
//             }

//             case STATS_WAIT_COMPLETE: { 
//                     for (int i = 0; i < lconf->n_rx_queue; i++) {
//                         uint32_t nb_rx = rte_eth_rx_burst(lconf->port, lconf->rx_queue_list[i], mbuf_burst, NC_MAX_BURST_SIZE);
//                         for (int j = 0; j < nb_rx; j++) {
//                             printf("826: STATS_WAIT_COMPLETE starts\n");
//                             mbuf_rcv = mbuf_burst[j];
//                             rte_prefetch0(rte_pktmbuf_mtod(mbuf_rcv, void *));     
//                             struct rte_ether_hdr* eth = rte_pktmbuf_mtod(mbuf_rcv, struct rte_ether_hdr *);
//                             // printf("829: stage = %d, ether_type = %d\n", stats_stage, ntohs(eth->ether_type));
//                             if(ntohs(eth->ether_type) == ether_type_fpga){
//                                 CtrlHeader_switch* message_header = (CtrlHeader_switch*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE_CTRL);
//                                 if(message_header->opType == OP_REPLACE_SUCCESS){                                
//                                     //DONE: unlock hot queue
//                                     //check_queue_lock_flag = 0;
//                                     clear_TOPK_flag = !clear_TOPK_flag;
//                                     for(int k=0; k<last_hotNum; k++){
//                                         queueLockFlag[last_hotID[k]] = 0;
//                                         //queueLockFlag[hotID[k]] = 0;                        
//                                     }
//                                     for(int k=0; k<hotNum; k++){
//                                         last_hotNum = hotNum;
//                                         last_hotID[k] = hotID[k];
//                                     }                                     

//                                     mbuf = rte_pktmbuf_alloc(pktmbuf_pool);
//                                     generate_ctrl_pkt(rx_queue_id, header_template_local_ctrl, lcore_id, mbuf, ether_type_fpga, OP_REPLACE_SUCCESS);
//                                     enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
//                                     stats_stage = STATS_GET_HOT;
//                                 }
//                             }
//                             else{
//                                 printf("---------------ERROR!!!!!!: error ether type = %x!---------------\n", ntohs(eth->ether_type));
//                                 //return 1;
//                             }
//                             rte_pktmbuf_free(mbuf_rcv);
//                         }
//                     }
//                 break;
//             }
//         }
//     }
//     return 0;
// }  
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------


static int32_t np_client_loop(__attribute__((unused)) void *arg) {
    uint32_t lcore_id = rte_lcore_id();
    if ((lcore_id < 1)) {
        //------------------------------------------------------------------------------
        //------------------------------------------------------------------------------
        struct rte_ether_addr src_addr = { 
            .addr_bytes = MAC_LOCAL}; 
        int ifvalue = 1;
        struct rte_mempool* pktmbuf_pool_tmp = pktmbuf_pool_ctrl;
        np_client_statistic_loop(lcore_id, 0, src_addr, ifvalue, pktmbuf_pool_tmp, n_lcores - 1, lockqueues);
        // np_client_statistic_loop(lcore_id, 0);
        //------------------------------------------------------------------------------
        //------------------------------------------------------------------------------
    }
    else {
        np_client_txrx_loop(lcore_id, lcore_id, lcore_id_2_rx_queue_id[lcore_id]);
    }
    return 0; 
}

int main(int argc, char **argv) {
    printf("im here!!!!!!\n");
    //configurations
    machine_id = 4;
    // parse default arguments
    int ret;
    ret = rte_eal_init(argc, argv);
    if (ret < 0) {
        rte_exit(EXIT_FAILURE, "invalid EAL arguments\n");
    }
    argc -= ret;
    argv += ret;
    // init
    nc_init();
    //define lock queues
    lockqueues = (queue_list *) malloc(sizeof(queue_list) * (MAX_LOCK_NUM + 1));
    // lockqueues = (queue_list *)rte_malloc
    for (int i=0; i<MAX_LOCK_NUM; i++) {
        queue_init(&(lockqueues[i]));
        num_ex[i] = 0;
        num_sh[i] = 0;
        num_ex_granted[i] = 0;
        num_sh_granted[i] = 0;        
    }

    statistic_ini();
    // for(int i=0; i<TOPK; i++){
    //     hotID[i] = 4294967295;
    //     last_hotID[i] = 4294967295;
    // }

    // for(int i=0; i<=1; i++){
    //     for(int j=0; j<NUM_SERVER_CORE; j++){
    //         for(int k=0; k<=TOPK; k++){
    //             TopKNode[i][j][k].lockId = 4294967295;
    //             TopKNode[i][j][k].lockCounter = 0;
    //         }
    //     }
    // }

    inet_pton(AF_INET, ip_src, &(ip_src_pton));   
    inet_pton(AF_INET, ip_dst, &(ip_dst_pton)); 

    int numa_id_of_port = rte_eth_dev_socket_id(0);
    printf("numa_id_of_port = %d\n", numa_id_of_port);

    for(int i = 1; i <= 11; i++){
        lcore_id_2_rx_queue_id[i] = i;
    }
    for(int i = 24; i <= 35; i++){
        lcore_id_2_rx_queue_id[i] = i - 12;
    }    

    //--------------------------------------------------------------
    for(int i = 0; i < NUM_CLIENT; i++){
        inet_pton(AF_INET, ip_client_arr[i], ip_client_pton_arr + i);
    }   

    // launch main loop in every lcore
    uint32_t lcore_id;
    rte_eal_mp_remote_launch(np_client_loop, NULL, CALL_MASTER);
    RTE_LCORE_FOREACH_SLAVE(lcore_id) {
        if (rte_eal_wait_lcore(lcore_id) < 0) {
            ret = -1;
            break;
        }
    }
    return 0;
}
