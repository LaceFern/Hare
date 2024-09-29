#include <stdio.h>
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

uint32_t num_ex[MAX_LOCK_NUM] = {0};
uint32_t num_sh[MAX_LOCK_NUM] = {0};
char busy[MAX_CLIENT_NUM][MAX_LOCK_NUM] = {0};
uint32_t ip_dst_pton, ip_src_pton;
int occupying_flag[MAX_LOCK_NUM] = {0};

struct latency_statistics {
    uint64_t max;
    uint64_t num;
    uint64_t total;
    uint64_t max_grantSucc;
    uint64_t num_grantSucc;
    uint64_t total_grantSucc;
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

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define LOCKID_NOTFIT 0x0 //must be larger than the range
#define MAX_LOCK_IN_SWITCH 32768//16384//16384//16382  MAX_LOCK_IN_SWITCH must larger than MAX_LOCK_IN_TXN
#define MAX_LOCK_IN_TXN 8//1//8!!!!!!!!!!!!!!!!!!!!!!!!!!!
#define MAX_CLIENT_IN_LCORE 512//512//512//1//512//1024//512//512//64
#define RESEND_CHECK_INTERVAL_US 5000//2000//200//50// 5000000 // consider adjusting it dynamically, fpga drops packets!!!!!
// #define RESEND_MAX_COUNT 100
// #define DEADLOCK_CHECK_INTERVAL_US 1000//1000//6000//1000
#define RESEND_CHECK_PADDING_US 1000


#define HC_US 50000000//50000000//5000000//50000//5000000//6000//1000
#define HC_UPD_US 30000000
#define SENDEND_US 140000000//5000000//50000//5000000//6000//1000
#define END_US 141000000//200000000//5000000//50000//5000000//6000//1000

#define CHECK_INTERVAL_US 1000000//100000//1000000//100000
#define CHECK_START_US 0//48000000//0//45000000
#define CHECK_END_US 140000000//200000000//65000000 


// #define HC_US 50000000//50000000//5000000//50000//5000000//6000//1000
// #define HC_UPD_US 30000000
// #define SENDEND_US 85000000//5000000//50000//5000000//6000//1000
// #define END_US 90000000//200000000//5000000//50000//5000000//6000//1000

// #define CHECK_INTERVAL_US 1000000//100000//1000000//100000
// #define CHECK_START_US 0//48000000//0//45000000
// #define CHECK_END_US 85000000//200000000//65000000 

// #define HC_US 50000000//50000000//5000000//50000//5000000//6000//1000
// #define HC_UPD_US 5000000
// #define SENDEND_US 65000000//5000000//50000//5000000//6000//1000
// #define END_US 70000000//200000000//5000000//50000//5000000//6000//1000

// #define CHECK_INTERVAL_US 100000//1000000//100000
// #define CHECK_START_US 48000000//0//45000000
// #define CHECK_END_US 65000000//200000000//65000000 

#define SERVER_CORE_NUM 23 // 23
 
#define RESPFLAG_IDLE 0 
#define RESPFLAG_ACQUIRE_WAIT 1
#define RESPFLAG_ACQUIRE_HAVEGOTTEN 2 
#define RESPFLAG_RELEASE_WAIT 3 
#define RESPFLAG_RELEASE_HAVEGOTTEN 4
#define RESPFLAG_ACQUIRE_FAIL 5
#define RESPFLAG_RELEASE_FAIL 6
#define RESPFLAG_ACQUIRE_INQUEUE 7 
//
#define RESPFLAG_ACQUIRE_MODFAIL 8
#define RESPFLAG_RELEASE_MODFAIL 9
//

// #define MAX_LOCK_IN_SWITCH_VALID 4096 //need to match LOCKID_MASK
#define LOCKID_MASK 0xfff//0x7fff//0x3ffffff
#define LOCKID_BIAS 0
#define LOCKID_CAT  0x0
#define QUEUE_DEPTH 8


// #define NOT_RECIRCULATE         0x00
// #define ACQUIRE_LOCK            0x01
// #define RELEASE_LOCK            0x02
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

#define LOCK_QUEUE_SUCC 0x07
#define RE_LOCK_QUEUE_SUCC 0x87
//
// #define LOCK_GRANT_MODFAIL 0x08
// #define LOCK_RELEA_MODFAIL 0x09

#define LOCK_GRANT_MODFAIL_NFS 0x12
#define LOCK_GRANT_MODFAIL_HIT 0x13
#define LOCK_RELEA_MODFAIL_NFS 0x14
#define LOCK_RELEA_MODFAIL_HIT 0x15
//
#define LOCK_GRANT_BW_FAILED 0x16
#define LOCK_RELEA_BW_FAILED 0x17

#define PADDING_FPGA 0xffff

//lock mode 
#define EXCLU_LOCK 1
#define SHARE_LOCK 2

#define LOCKID_OVERFLOW  0xffffffff



uint32_t wpkts_send_limit_ms = 8192;//8192;//8192;
uint32_t average_interval = 5;
uint32_t endFlag[NC_MAX_LCORES];

char ip_src[32] = IP_LOCAL;
char ip_dst[32] = IP_FPGA;

typedef struct mine_lockInfo{
    uint32_t lock_id;
    uint8_t action_type;
    uint8_t lock_type;
    uint8_t resend_count; 
    uint64_t last_send_time;
}struct_lockInfo;

typedef struct mine_txnInfo{
    uint32_t txn_id;
    uint32_t lock_num;
    uint16_t cid;
}struct_txnInfo;

typedef struct mine_zipfPara{
    double a; 
    uint32_t n;
    uint32_t lockNumInTxn;
    uint32_t txnNum;
    uint32_t txnNumReadIn;
    uint32_t lfsrSeed;
    uint64_t deadlock_check_interval_us;
    uint32_t hotChangedNum;
    uint32_t distribution_type;
    uint32_t fpga_wait_ms;
    uint32_t fpga_cacheSetNum;
    uint32_t reqNum;
    uint32_t reqNumReadIn;
}struct_zipfPara;

struct_zipfPara gb_zipfPara={
    0.99,
    1048576,
    1,
    1048576, 
    1048576,
    1,
    10000,//5000000, //5000000,
    500,
    0,
    100,
    64,
    1048576,
    1048576
};
pthread_barrier_t barrier;
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

// op code
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

typedef struct mine_kvReqInfo{
    uint32_t key;
    uint8_t op_type;
    uint8_t value_len;
}struct_kvReqInfo;

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
    double average_latency_grantSucc = 0;

    if (latency_stat[1].num > 0) {
        average_latency = latency_stat[1].total / (double)latency_stat[1].num;
    }
    if (latency_stat[1].num_grantSucc > 0) {
        average_latency_grantSucc = latency_stat[1].total_grantSucc / (double)latency_stat[1].num_grantSucc;
    }
    printf("\tcount: %"PRIu64"\t"
        "average latency: %.4f ms\t"
        "max latency: %.4f ms\t"
        "overflow: %"PRIu64"\n",
        latency_stat[1].num, average_latency / 1000.0, latency_stat[1].max / 1000.0, overflow);
    printf("\tcount_grantSucc: %"PRIu64"\t"
        "average latency_grantSucc: %.4f ms\t"
        "max latency_grantSucc: %.4f ms\n",
        latency_stat[1].num_grantSucc, average_latency_grantSucc / 1000.0, latency_stat[1].max_grantSucc / 1000.0);
}

static void generate_lock_request_pkt(uint16_t rx_queue_id, uint8_t header_template[HEADER_TEMPLATE_SIZE], uint32_t lcore_id, struct rte_mbuf *mbuf, \
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
    
    // if (ip_src_addr != 0) 
    //     ip->src_addr = ip_src_addr;
    // else 
    //     inet_pton(AF_INET, ip_src, &(ip->src_addr));

    // if (ip_dst_addr != 0)
    //     ip->dst_addr = ip_dst_addr;
    // else
    //     inet_pton(AF_INET, ip_dst, &(ip->dst_addr));
    uint16_t bn_idx = lockInfo.lock_id % BN_NUM;
    char* ip_src_hc = IP_LOCAL;
    char* ip_dst_hc = ip_dst_arr[bn_idx];
    struct rte_ether_addr mac_src = {.addr_bytes = MAC_LOCAL};
    struct rte_ether_addr mac_dst = mac_dst_arr[bn_idx];
    inet_pton(AF_INET, ip_src_hc, &(ip->src_addr));
    inet_pton(AF_INET, ip_dst_hc, &(ip->dst_addr));
    rte_ether_addr_copy(&mac_src, &eth->s_addr);
    rte_ether_addr_copy(&mac_dst, &eth->d_addr);

    // printf("ip->src_addr = %x\n", ip->src_addr);
    // printf("ip->dst_addr = %x\n", ip->dst_addr);

    
    udp->src_port = htons(CLIENT_PORT + rx_queue_id);//htons(port_write + (uint32_t)(txnInfo.txn_id) % 128);
    // udp->src_port = htons(port_write); 
    // if(lcore_id == 2) printf("src_port = %d\n", CLIENT_PORT + rx_queue_id);
    // udp->dst_port = htons(SERVICE_PORT);
    
    // DEBUG!!
    uint16_t temp_port = 5002 + lockInfo.lock_id % SERVER_CORE_NUM;
    // uint16_t temp_port = 5002;
    if(temp_port < 5002 || temp_port > 5024){
        printf("ERROR!!: dst_port = %d\n", temp_port);
    }
    if(lockInfo.lock_id < 0 || lockInfo.lock_id > 10009999){
        printf("ERROR!!: lockid = %d\n", lockInfo.lock_id);
    }
    udp->dst_port = htons(temp_port);
    //udp->dst_port = htons(5002 + lockInfo.lock_id % 23); //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    
    MessageHeader* message_header = (MessageHeader*) ((uint8_t*)eth + HEADER_TEMPLATE_SIZE);
    message_header->padding = 0;
    message_header->opType = lockInfo.action_type;
    message_header->lockID = htonl(lockInfo.lock_id);
    message_header->lockMode = lockInfo.lock_type;
    message_header->txnID = htonl(txnInfo.txn_id);
    message_header->clientID = htons(txnInfo.cid);

    // if(txnInfo.cid == 3){
    //     printf("error: txnInfo.cid == 3\n");
    // }

    if (timestamp == 0) {
        message_header->timestamp = rte_rdtsc();
    } else {
        message_header->timestamp = timestamp;
    }
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


static void generate_kv_request_pkt(uint16_t rx_queue_id, uint8_t header_template[HEADER_TEMPLATE_SIZE], uint32_t lcore_id, struct rte_mbuf *mbuf, \
            struct_kvReqInfo reqInfo, uint32_t ip_src_addr, uint32_t ip_dst_addr, uint64_t timestamp) {
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
    
    // if (ip_src_addr != 0) 
    //     ip->src_addr = ip_src_addr;
    // else 
    //     inet_pton(AF_INET, ip_src, &(ip->src_addr));

    // if (ip_dst_addr != 0)
    //     ip->dst_addr = ip_dst_addr;
    // else
    //     inet_pton(AF_INET, ip_dst, &(ip->dst_addr));

    uint16_t bn_idx = reqInfo.key % BN_NUM;
    char* ip_src_hc = IP_LOCAL;
    char* ip_dst_hc = ip_dst_arr[bn_idx];
    struct rte_ether_addr mac_src = {.addr_bytes = MAC_LOCAL};
    struct rte_ether_addr mac_dst = mac_dst_arr[bn_idx];
    inet_pton(AF_INET, ip_src_hc, &(ip->src_addr));
    inet_pton(AF_INET, ip_dst_hc, &(ip->dst_addr));
    rte_ether_addr_copy(&mac_src, &eth->s_addr);
    rte_ether_addr_copy(&mac_dst, &eth->d_addr);

    // debug
    // printf("ip->src_addr = %x\n", ip->src_addr);
    // printf("ip->dst_addr = %x\n", ip->dst_addr);

    
    udp->src_port = htons(CLIENT_PORT + rx_queue_id);//htons(port_write + (uint32_t)(txnInfo.txn_id) % 128);
    // udp->src_port = htons(port_write); 
    // if(lcore_id == 2) printf("src_port = %d\n", CLIENT_PORT + rx_queue_id);
    // udp->dst_port = htons(SERVICE_PORT);
    
    // DEBUG!!
    uint16_t temp_port = 5002 + reqInfo.key % SERVER_CORE_NUM;//uint16_t temp_port = 5002 + reqInfo.key % 47;//uint16_t temp_port = 5002 + reqInfo.key % 23;
    // uint16_t temp_port = 5002;
    // if(temp_port < 5002 || temp_port > 5024){
    //     printf("ERROR!!: dst_port = %d\n", temp_port);
    // }
    if(reqInfo.key < 0 || reqInfo.key > 10009999){
        printf("ERROR!!: key = %d\n", reqInfo.key);
    }
    udp->dst_port = htons(temp_port);
    // if(reqInfo.key == 79) printf("obj = %d, udp_port = %d, rx_queue_id = %d\n", reqInfo.key, ntohs(udp->dst_port), rx_queue_id);
    //udp->dst_port = htons(5002 + lockInfo.lock_id % 23); //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    if(reqInfo.op_type == OP_GET || reqInfo.op_type == OP_DEL){
        MessageHeader_KVSS* message_header = (MessageHeader_KVSS*) ((uint8_t*)eth + HEADER_TEMPLATE_SIZE);
        message_header->opType = reqInfo.op_type;

        // message_header->key[0] = reqInfo.key;
        unsigned char key_tmp[KEY_BYTES];
        key_tmp[0] = reqInfo.key & 0xff;
        key_tmp[1] = (reqInfo.key >> 8) & 0xff;
        key_tmp[2] = (reqInfo.key >> 16) & 0xff;
        key_tmp[3] = (reqInfo.key >> 24) & 0xff;
        for(int i=0; i<KEY_BYTES; i++){
            message_header->key[KEY_BYTES - 1 - i] = key_tmp[i];
        }

        // message_header->valueLen = reqInfo.value_len;
        // if (timestamp == 0) {
        //     message_header->timestamp = rte_rdtsc();
        // } else {
        //     message_header->timestamp = timestamp;
        // }
        mbuf->data_len += sizeof(MessageHeader);
        mbuf->pkt_len += sizeof(MessageHeader);
    }
    else{
        MessageHeader_KVSL* message_header = (MessageHeader_KVSL*) ((uint8_t*)eth + HEADER_TEMPLATE_SIZE);
        message_header->opType = reqInfo.op_type;

        // message_header->key[0] = reqInfo.key;
        unsigned char key_tmp[KEY_BYTES];
        key_tmp[0] = reqInfo.key & 0xff;
        key_tmp[1] = (reqInfo.key >> 8) & 0xff;
        key_tmp[2] = (reqInfo.key >> 16) & 0xff;
        key_tmp[3] = (reqInfo.key >> 24) & 0xff;
        // printf("key[3]=%u, key[2]=%u, key[1]=%u, key[0]=%u, key_id = %u\n", (uint32_t)(key_tmp[3]), (uint32_t)(key_tmp[2]), (uint32_t)(key_tmp[1]), (uint32_t)(key_tmp[0]), reqInfo.key);
        for(int i=0; i<KEY_BYTES; i++){
            message_header->key[KEY_BYTES - 1 - i] = key_tmp[i];
        }

        message_header->valueLen = reqInfo.value_len;
        message_header->value[0] = 0;
        // if (timestamp == 0) {
        //     message_header->timestamp = rte_rdtsc();
        // } else {
        //     message_header->timestamp = timestamp;
        // }
        mbuf->data_len += sizeof(MessageHeader);
        mbuf->pkt_len += sizeof(MessageHeader);
    }
}


static void generate_hc_pkt(uint16_t rx_queue_id, uint8_t header_template[HEADER_TEMPLATE_SIZE], uint32_t lcore_id, struct rte_mbuf *mbuf, \
                        char ip_src_hc[], char ip_dst_hc[], struct rte_ether_addr mac_src_hc, struct rte_ether_addr mac_dst_hc) {
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
    
    inet_pton(AF_INET, ip_src_hc, &(ip->src_addr));
    inet_pton(AF_INET, ip_dst_hc, &(ip->dst_addr));

    udp->src_port = htons(HC_PORT);
    udp->dst_port = htons(HC_PORT);

    rte_ether_addr_copy(&mac_src_hc, &eth->s_addr);
    rte_ether_addr_copy(&mac_dst_hc, &eth->d_addr);

    MessageHeader* message_header = (MessageHeader*) ((uint8_t*)eth + HEADER_TEMPLATE_SIZE);
    message_header->opType = OP_HC;

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

static void compute_latency_grantSucc(struct latency_statistics *latency_stat,
    uint64_t latency) {
    latency_stat->num_grantSucc++;
    latency_stat->total_grantSucc += latency;
    if(latency_stat->max_grantSucc < latency) {
        latency_stat->max_grantSucc = latency; 
    }
} 

// TX loop for test, fixed write rate
static int32_t np_client_txrx_loop_lock(uint32_t lcore_id, uint32_t client_id, uint16_t rx_queue_id, struct_zipfPara zipfPara, FILE* fout, FILE* fout_fpgaCheck) {
    printf("%lld entering TX loop for write on lcore %u, numa_id_of_lcore = %d\n", (long long)time(NULL), lcore_id, rte_socket_id());

    //trace
    // printf("check point 0\n");
    uint32_t *lockId_arr = (uint32_t *)malloc(sizeof(uint32_t *) * zipfPara.txnNumReadIn * zipfPara.lockNumInTxn);
    uint32_t *lockId_hc_arr = (uint32_t *)malloc(sizeof(uint32_t *) * zipfPara.txnNumReadIn * zipfPara.lockNumInTxn);
    char lockFile[128];
    char lockFile_hc[128];
    if(zipfPara.distribution_type == 0){
        sprintf(lockFile, "./trace/res_zipf/a%0.2f_n%u_lNIT%u_tN%d_sd%u.txt", zipfPara.a, zipfPara.n, zipfPara.lockNumInTxn, zipfPara.txnNum, zipfPara.lfsrSeed);
        sprintf(lockFile_hc, "./trace/res_zipf_hc/a%0.2f_n%u_lNIT%u_tN%d_sd%u_hot-in_%u.txt", zipfPara.a, zipfPara.n, zipfPara.lockNumInTxn, zipfPara.txnNum, zipfPara.lfsrSeed, zipfPara.hotChangedNum);    
    }
    else{
        sprintf(lockFile, "./trace/res_rect/a%0.2f_n%u_lNIT%u_tN%d_sd%u.txt", zipfPara.a, zipfPara.n, zipfPara.lockNumInTxn, zipfPara.txnNum, zipfPara.lfsrSeed);
        sprintf(lockFile_hc, "./trace/res_rect_hc/a%0.2f_n%u_lNIT%u_tN%d_sd%u_hot-in_%u.txt", zipfPara.a, zipfPara.n, zipfPara.lockNumInTxn, zipfPara.txnNum, zipfPara.lfsrSeed, zipfPara.hotChangedNum);    
    }
 

    printf("hotChangedNum: %u\n", zipfPara.hotChangedNum);
    printf("traceFile: %s\n", lockFile);
    FILE *lockFp=fopen(lockFile,"r");
    FILE *lockFp_hc=fopen(lockFile_hc,"r");
    if(!lockFp){
       printf("error when open %s\n", lockFile);
       endFlag[lcore_id] = 1;
       return -1;
    }
    if(!lockFp_hc){
       printf("error when open %s\n", lockFile_hc);
       endFlag[lcore_id] = 1;
       return -1;
    } 
    // printf("check point 1\n");
    for(uint32_t i = 0; i < zipfPara.txnNumReadIn * zipfPara.lockNumInTxn; i++){
        // printf("check point 1.5: %d\n", i);
        fscanf(lockFp,"%u", lockId_arr+i);
        fscanf(lockFp_hc,"%u", lockId_hc_arr+i);
        if(i == 0) printf("idx=0, lockid-d = %d, lockid-u = %u\n", lockId_arr[i], lockId_arr[i]);
    }
    uint32_t txnIdx = 0;
    uint32_t lockIdxInTxn = 0; 
    // printf("check point 2\n");
    //current txn info
    struct_lockInfo ** lockInTxn_arr = (struct_lockInfo **)malloc(sizeof(struct_lockInfo *) * MAX_CLIENT_IN_LCORE);
    int ** getRespFlag_arr = (int **)malloc(sizeof(int *) * MAX_CLIENT_IN_LCORE);
    for(int i = 0; i < MAX_CLIENT_IN_LCORE; i++){
        lockInTxn_arr[i] = (struct_lockInfo *)malloc(sizeof(struct_lockInfo) * MAX_LOCK_IN_TXN);
        getRespFlag_arr[i] = (int *)malloc(sizeof(int) * MAX_LOCK_IN_TXN); 
    }
    // printf("check point 3\n");
    //---------------------------------------------------
    struct_txnInfo txn_arr[MAX_CLIENT_IN_LCORE];
    int stepFlag[MAX_CLIENT_IN_LCORE] = {0};
    uint64_t next_deadlock_check_tsc[MAX_CLIENT_IN_LCORE] = {0};
    uint32_t cid[MAX_CLIENT_IN_LCORE];
    uint32_t lockID[MAX_CLIENT_IN_LCORE];
    for(uint32_t i = 0; i < MAX_CLIENT_IN_LCORE; i++){
        cid[i] = (client_id - 1) * MAX_CLIENT_IN_LCORE + i + CLIENT_ID_BIAS;
        lockID[i] = cid[i];//rand();//cid[i];
    }
    for(int i = 0; i < MAX_CLIENT_IN_LCORE; i++){
        for(int j = 0; j < MAX_LOCK_IN_TXN; j++){
            getRespFlag_arr[i][j] = RESPFLAG_IDLE;
        }
    }
    // printf("check point 4\n");
    uint16_t randop[MAX_CLIENT_IN_LCORE];
    for(int i = 0; i < MAX_CLIENT_IN_LCORE; i++){
        randop[i] = rand();
    }
    //---------------------------------------------------

    // initiate header_template_local
    uint8_t header_template_local[HEADER_TEMPLATE_SIZE];
    init_header_template_local(header_template_local);
    // initiate lconf
    struct lcore_configuration *lconf = &lcore_conf[lcore_id];
    // memory related points
    struct rte_mbuf *mbuf;
    
    // time related configurations 
    uint64_t cur_tsc = rte_rdtsc();
    uint64_t ms_tsc = rte_get_tsc_hz() / 1000; 
    uint64_t next_ms_tsc = cur_tsc + ms_tsc;
    uint64_t pkts_send_ms = 0;
    uint64_t pkts_send_counter = 0;

    // mine
    uint64_t resend_check_interval_us = rte_get_tsc_hz() / 1000000 * RESEND_CHECK_INTERVAL_US;
    uint64_t deadlock_check_interval_us = rte_get_tsc_hz() / 1000000 * zipfPara.deadlock_check_interval_us;
    uint64_t padding_us = rte_get_tsc_hz() / 1000000 * RESEND_CHECK_PADDING_US;
    uint64_t end_us = cur_tsc + rte_get_tsc_hz() / 1000000 * END_US;
    uint64_t hc_us = cur_tsc + rte_get_tsc_hz() / 1000000 * HC_US;
    uint64_t sendEnd_us = cur_tsc + rte_get_tsc_hz() / 1000000 * SENDEND_US;
 
    struct rte_mbuf *mbuf_burst[NC_MAX_BURST_SIZE];
    struct rte_mbuf *mbuf_rcv;
    // printf("check point 1.\n");

    //debug
    uint64_t update_tsc = rte_get_tsc_hz(); // in second
    uint64_t next_update_tsc = cur_tsc + update_tsc;
 

    int sendEnd_flag[MAX_CLIENT_IN_LCORE] = {0};

    int count[MAX_CLIENT_IN_LCORE] = {0};
    for(int i = 0; i < MAX_CLIENT_IN_LCORE; i++){
        count[i] = 0;
    }
    
    while (1) {
    //     // sleep(0.5);
    //     // read current time
    //     // printf("pkts_send_ms = %d\n", pkts_send_ms);
        cur_tsc = rte_rdtsc();
        // clean packet counters for each ms
        if(unlikely(cur_tsc > end_us)){
            for(int c = 0; c < MAX_CLIENT_IN_LCORE; c++){
                if(getRespFlag_arr[c][0] != RESPFLAG_IDLE){
                    fprintf(fout, "812 endStatus: client_id:%d, lock_id:%d, getRespFlag:%d, \n", cid[c], lockInTxn_arr[c][0].lock_id, getRespFlag_arr[c][0]);
                }
            }
            break;
        }
        if (unlikely(cur_tsc > next_ms_tsc)) {
            pkts_send_ms = 0;
            next_ms_tsc += ms_tsc;
        } 

        // RX
        ////fprintf(fout, "613 check point: rx start.\n");
        int loop = MAX_CLIENT_IN_LCORE * zipfPara.lockNumInTxn / NC_MAX_BURST_SIZE;
        if(loop == 0) loop = 1;
        for(int c = 0; c < loop; c++){ 
            for (int i = 0; i < lconf->n_rx_queue; i++) {
                uint32_t nb_rx = rte_eth_rx_burst(lconf->port, lconf->rx_queue_list[i], mbuf_burst, NC_MAX_BURST_SIZE);
                tput_stat[lcore_id].rx += nb_rx;
                for (int j = 0; j < nb_rx; j++) {
                
                    mbuf_rcv = mbuf_burst[j];
                    rte_prefetch0(rte_pktmbuf_mtod(mbuf_rcv, void *)); 
                    struct rte_ether_hdr* eth = rte_pktmbuf_mtod(mbuf_rcv, struct rte_ether_hdr *);
                    struct rte_ipv4_hdr *ip = (struct rte_ipv4_hdr *)((uint8_t*) eth + sizeof(struct rte_ether_hdr));
                    struct rte_udp_hdr *udp = (struct rte_udp_hdr *)((uint8_t*) ip + sizeof(struct rte_ipv4_hdr));
                    // printf("dst_port = %d, src_port = %d\n", ntohs(udp->dst_port), ntohs(udp->src_port));
                    // parse NetLock header,could be changed
                    MessageHeader* message_header = (MessageHeader*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE);
                    uint8_t  rcv_mode = (uint8_t) message_header->lockMode;
                    uint8_t  rcv_opt = (uint8_t) message_header->opType;
                    uint32_t rcv_txn_id = ntohl(message_header->txnID);
                    uint32_t rcv_lock_id = ntohl(message_header->lockID); 
                    uint16_t rcv_cid = ntohs(message_header->clientID);
                    uint16_t rcv_padding = ntohs(message_header->padding);
                    float latency = timediff_in_us(rte_rdtsc(), message_header->timestamp);

                    // printf("latency = %f\n", latency)
                    
                    //fprintf(fout, "613 rcv lock_id:%d, txn_id:%d, cid:%d, opType:%d, mode:%d\n", rcv_lock_id, rcv_txn_id, rcv_cid, rcv_opt, rcv_mode);
                    // printf("(checkpoint 0) 907 rcv lock_id:%d, txn_id:%d, cid:%d, opType:%d, mode:%d\n", rcv_lock_id, rcv_txn_id, rcv_cid, rcv_opt, rcv_mode);
                    compute_latency(&latency_stat_c[lcore_id], latency);
                    if (latency_sample_start == 1) {
                        if (latency_stat_c[lcore_id].num % latency_sample_interval == 0) {
                            // printf("latency_stat_c[lcore_id].num = %d\n", latency_stat_c[lcore_id].num);
                            latency_samples[lcore_id][latency_sample_num[lcore_id]] = latency;
                            latency_sample_num[lcore_id]++;
                        }
                    }
 
                    // if(rcv_cid != LOCKID_NOTFIT){
                        //set flag
                        int fitFlag = 0; 
                        int rcv_cid_idx = rcv_cid - (client_id - 1) * MAX_CLIENT_IN_LCORE - CLIENT_ID_BIAS;
                        // printf("(checkpoint 1) dst_port = %u, src_port = %u, rcv_mode = %u, rcv_opt = %u, rcv_txn_id = %u, rcv_cid = %u, rcv_padding = %u, rcv_lockId = %u\n", ntohs(udp->dst_port), ntohs(udp->src_port), rcv_mode, rcv_opt, rcv_txn_id, rcv_cid, rcv_padding, rcv_lock_id);
                        // if(rcv_cid_idx >= 0 && udp->src_port == htons(SERVICE_PORT)){   
                        if(rcv_cid_idx >= 0 && rcv_cid_idx < MAX_CLIENT_IN_LCORE){   
                            // printf("907 lockInfo.lock_id == rcv_lock_id\n");
                            for(int k = 0; k < txn_arr[rcv_cid_idx].lock_num; k++){
                                struct_lockInfo lockInfo = lockInTxn_arr[rcv_cid_idx][k];
                                if(lockInfo.lock_id == rcv_lock_id){
                                    // printf("907 lockInfo.lock_id == rcv_lock_id\n");
                                    //statistics
                                    // printf("(checkpoint 2) rcv lock_id:%d, txn_id:%d, cid:%d, op_type:%d, latency:%f\n", rcv_lock_id, rcv_txn_id, rcv_cid, rcv_opt, latency);
                                    // printf("rcv message_header->payload = %lu, current = %lu\n", message_header->payload, rte_rdtsc());
                                    if((rcv_opt == LOCK_GRANT_SUCC || rcv_opt == RE_LOCK_GRANT_SUCC) && (getRespFlag_arr[rcv_cid_idx][k] == RESPFLAG_ACQUIRE_WAIT || getRespFlag_arr[rcv_cid_idx][k] == RESPFLAG_ACQUIRE_INQUEUE)){
                                        // printf("907 rcv_opt == LOCK_GRANT_SUCC, getRespFlag_arr[rcv_cid_idx][k] == RESPFLAG_ACQUIRE_WAIT\n");
                                        fitFlag = 1;
                                        getRespFlag_arr[rcv_cid_idx][k] = RESPFLAG_ACQUIRE_HAVEGOTTEN;
                                        tput_stat[lcore_id].rx_grantSucc += 1;
                                        if(rcv_padding == PADDING_FPGA){
                                            tput_stat[lcore_id].rx_fpga += 1;
                                            tput_stat[lcore_id].rx_grantSucc_fpga += 1;
                                        }
                                        else{
                                            tput_stat[lcore_id].rx_p4 += 1;
                                            tput_stat[lcore_id].rx_grantSucc_p4 += 1;
                                            if(rcv_padding == 1){
                                                tput_stat[lcore_id].rx_mirror_p4 += 1;
                                            }
                                        }
                                        compute_latency_grantSucc(&latency_stat_c[lcore_id], latency);
                                    }
                                    else if((rcv_opt == LOCK_QUEUE_SUCC || rcv_opt == RE_LOCK_QUEUE_SUCC) && getRespFlag_arr[rcv_cid_idx][k] == RESPFLAG_ACQUIRE_WAIT){
                                        fitFlag = 1;
                                        getRespFlag_arr[rcv_cid_idx][k] = RESPFLAG_ACQUIRE_INQUEUE;
                                        tput_stat[lcore_id].rx_queueSucc += 1;
                                        if(rcv_padding == PADDING_FPGA){
                                            tput_stat[lcore_id].rx_fpga += 1;
                                            tput_stat[lcore_id].rx_queueSucc_fpga += 1;
                                        }
                                        else{   
                                            tput_stat[lcore_id].rx_p4 += 1;
                                            tput_stat[lcore_id].rx_queueSucc_p4 += 1;
                                        }
                                    }  
                                    else if((rcv_opt == LOCK_GRANT_FAIL || rcv_opt == RE_LOCK_GRANT_FAIL || rcv_opt == LOCK_GRANT_MODFAIL_NFS || rcv_opt == LOCK_GRANT_MODFAIL_HIT || rcv_opt == LOCK_GRANT_BW_FAILED) && getRespFlag_arr[rcv_cid_idx][k] == RESPFLAG_ACQUIRE_WAIT){
                                        fitFlag = 1;
                                        if(rcv_opt == LOCK_GRANT_FAIL || rcv_opt == RE_LOCK_GRANT_FAIL){
                                            getRespFlag_arr[rcv_cid_idx][k] = RESPFLAG_ACQUIRE_FAIL;
                                            tput_stat[lcore_id].rx_grantFail += 1;
                                            if(rcv_padding == PADDING_FPGA){
                                                tput_stat[lcore_id].rx_fpga += 1;
                                                tput_stat[lcore_id].rx_grantFail_fpga += 1;
                                            }
                                            else{
                                                tput_stat[lcore_id].rx_p4 += 1;
                                                tput_stat[lcore_id].rx_grantFail_p4 += 1;
                                            }
                                        }
                                        else{
                                            getRespFlag_arr[rcv_cid_idx][k] = RESPFLAG_ACQUIRE_MODFAIL;
                                            if(rcv_opt == LOCK_GRANT_MODFAIL_HIT){
                                                tput_stat[lcore_id].rx_grantModHitFail_fpga += 1;
                                                tput_stat[lcore_id].rx_grantModFail_fpga += 1;
                                                tput_stat[lcore_id].rx_grantModFail += 1;
                                                tput_stat[lcore_id].rx_fpga += 1;
                                            }
                                            else if(rcv_opt == LOCK_GRANT_MODFAIL_NFS){
                                                tput_stat[lcore_id].rx_grantModNfsFail_fpga += 1;
                                                tput_stat[lcore_id].rx_grantModFail_fpga += 1;
                                                tput_stat[lcore_id].rx_grantModFail += 1;
                                                tput_stat[lcore_id].rx_fpga += 1;
                                            }
                                            else{
                                                tput_stat[lcore_id].rx_bdFail_p4 += 1;
                                                tput_stat[lcore_id].rx_p4 += 1;
                                            }
                                        }
                                        // printf("lcore = %d, rcv_opt = %d, rcv_cid_idx = %d, getRespFlag = %d \n", lcore_id, rcv_opt, rcv_cid_idx, getRespFlag_arr[rcv_cid_idx][k]);
                                    }          
                                    else if((rcv_opt == LOCK_RELEA_SUCC || rcv_opt == RE_LOCK_RELEA_SUCC) && getRespFlag_arr[rcv_cid_idx][k] == RESPFLAG_RELEASE_WAIT){
                                        fitFlag = 1;
                                        getRespFlag_arr[rcv_cid_idx][k] = RESPFLAG_RELEASE_HAVEGOTTEN;
                                        tput_stat[lcore_id].rx_releaseSucc += 1;
                                        if(rcv_padding == PADDING_FPGA){
                                            tput_stat[lcore_id].rx_fpga += 1;
                                            tput_stat[lcore_id].rx_releaseSucc_fpga += 1;
                                        }
                                        else{
                                            tput_stat[lcore_id].rx_p4 += 1;
                                            tput_stat[lcore_id].rx_releaseSucc_p4 += 1;
                                        }
                                        fitFlag = 1;
                                    }
                                    else if((rcv_opt == LOCK_RELEA_FAIL || rcv_opt == RE_LOCK_RELEA_FAIL || rcv_opt == LOCK_RELEA_MODFAIL_NFS || rcv_opt == LOCK_RELEA_MODFAIL_HIT || rcv_opt == LOCK_RELEA_BW_FAILED) && getRespFlag_arr[rcv_cid_idx][k] == RESPFLAG_RELEASE_WAIT){
                                        fitFlag = 1;
                                        if(rcv_opt == LOCK_RELEA_FAIL || rcv_opt == RE_LOCK_RELEA_FAIL){
                                            getRespFlag_arr[rcv_cid_idx][k] = RESPFLAG_RELEASE_FAIL;
                                            tput_stat[lcore_id].rx_releaseFail += 1;
                                            if(rcv_padding == PADDING_FPGA){
                                                tput_stat[lcore_id].rx_fpga += 1;
                                                tput_stat[lcore_id].rx_releaseFail_fpga += 1;
                                            }
                                            else{
                                                tput_stat[lcore_id].rx_p4 += 1;
                                                tput_stat[lcore_id].rx_releaseFail_p4 += 1;
                                            }
                                        }
                                        else{
                                            getRespFlag_arr[rcv_cid_idx][k] = RESPFLAG_RELEASE_MODFAIL;

                                            if(rcv_opt == LOCK_RELEA_MODFAIL_HIT){
                                                tput_stat[lcore_id].rx_releaseModHitFail_fpga += 1;
                                                tput_stat[lcore_id].rx_releaseModFail_fpga += 1;
                                                tput_stat[lcore_id].rx_releaseModFail += 1;
                                                tput_stat[lcore_id].rx_fpga += 1;
                                            }
                                            else if(rcv_opt == LOCK_RELEA_MODFAIL_NFS){
                                                tput_stat[lcore_id].rx_releaseModNfsFail_fpga += 1;
                                                tput_stat[lcore_id].rx_releaseModFail_fpga += 1;
                                                tput_stat[lcore_id].rx_releaseModFail += 1;
                                                tput_stat[lcore_id].rx_fpga += 1;
                                            }
                                            else{
                                                tput_stat[lcore_id].rx_bdFail_p4 += 1;
                                                tput_stat[lcore_id].rx_p4 += 1;
                                            }
                                        }
                                        //fprintf(fout, "613 rcv lock_id:%d, txn_id:%d, cid:%d, opType:%d, mode:%d\n", rcv_lock_id, rcv_txn_id, rcv_cid, rcv_opt, rcv_mode);
                                        // printf("lcore = %d, rcv_opt = %d, rcv_cid_idx = %d, getRespFlag = %d \n", lcore_id, rcv_opt, rcv_cid_idx, getRespFlag_arr[rcv_cid_idx][k]);
                                    }   
                                    // printf("lcore = %d, rcv_opt = %d, rcv_cid_idx = %d, getRespFlag = %d \n", lcore_id, rcv_opt, rcv_cid_idx, getRespFlag_arr[rcv_cid_idx][k]);
                                    break;
                                }    
                            } 
                        }
                    //     if(fitFlag == 0 && (rcv_opt == LOCK_GRANT_SUCC || rcv_opt == RE_LOCK_GRANT_SUCC)){
                    //         struct_lockInfo lockInfo;  
                    //         struct_txnInfo txnInfo;
                    //         txnInfo.cid = LOCKID_NOTFIT;
                    //         lockInfo.lock_id = rcv_lock_id;
                    //         lockInfo.action_type = RELEA_LOCK;
                    //         lockInfo.lock_type = rcv_mode;
                    //         mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]); 
                    //         generate_lock_request_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, lockInfo, txnInfo, ip_src_pton, ip_dst_pton, 0);
                    //         enqueue_pkt_with_thres(lcore_id, mbuf, 32, 0);
                    //         // //fprintf(fout, "613 rcv but send (4rls) lock_id:%d, txn_id:%d, cid:%d, opType:%d, mode:%d\n", rcv_lock_id, rcv_txn_id, txnInfo.cid, RELEA_LOCK, rcv_mode);
                    //     }
                    // }
                    rte_pktmbuf_free(mbuf_rcv);
                }
            }
        }

        // TX: generate packet, put in TX queue
        ////fprintf(fout, "613 check point: tx start.\n");
        if (pkts_send_ms < wpkts_send_limit_ms) {
            for(int c = 0; c < MAX_CLIENT_IN_LCORE; c++){
                // stage:send batch--require
                if(stepFlag[c] == 0 && sendEnd_flag[c] == 0){
                    tput_stat[lcore_id].txn_total += 1;

                    struct_txnInfo txnInfo;
                    txnInfo.lock_num = zipfPara.lockNumInTxn;//MAX_LOCK_IN_TXN;//rand() % MAX_LOCK_IN_TXN + 1;
                    txnInfo.txn_id = cid[c];// rand();
                    txnInfo.cid = cid[c];
                    txn_arr[c] = txnInfo;

                    lockIdxInTxn = txnIdx * zipfPara.lockNumInTxn;
                    if(txnIdx == zipfPara.txnNumReadIn - 1){
                        txnIdx = 0;
                    }
                    else{
                        txnIdx++;
                    }
                    for(int i = 0; i < txnInfo.lock_num; i++){
                        struct_lockInfo lockInfo;  

                        // lockInfo.lock_id = lockID[c]; 
                        // lockID[c] ^= lockID[c] >> 7;
                        // lockID[c] ^= lockID[c] << 20;
                        // lockID[c] ^= lockID[c] >> 13;
                        // lockInfo.lock_id &= LOCKID_MASK; //0x7fff;
                        // lockInfo.lock_id = 0;
                        // lockInfo.lock_id <<= LOCKID_BIAS;
                        // lockInfo.lock_id += LOCKID_CAT;

                        // if(hc_flag == 1){
                        //     lockInfo.lock_id = lockId_hc_arr[lockIdxInTxn];
                        // } 
                        // else{
                        //     lockInfo.lock_id = lockId_arr[lockIdxInTxn];
                        // }            
                        // if(((int)(lockId_arr[lockIdxInTxn]) - hc_flag * 10000) < 0){ //��Ȼ����ѭ�������⵫��Ŀǰ���滻����������ѭ��
                        //     lockInfo.lock_id = zipfPara.txnNumReadIn * zipfPara.lockNumInTxn + lockId_arr[lockIdxInTxn] - hc_flag * 10000;
                        // } 
                        // else{
                        //     lockInfo.lock_id = lockId_arr[lockIdxInTxn] - hc_flag * 10000;
                        // }
                        //debug
                        if(((int)(lockId_arr[lockIdxInTxn]) - (int)(hc_flag) * (int)(zipfPara.hotChangedNum)) < 0){ //��Ȼ����ѭ�������⵫��Ŀǰ���滻����������ѭ��
                            lockInfo.lock_id = zipfPara.reqNumReadIn + lockId_arr[lockIdxInTxn] - hc_flag * zipfPara.hotChangedNum;
                        } 
                        else{
                            lockInfo.lock_id = lockId_arr[lockIdxInTxn] - hc_flag * zipfPara.hotChangedNum;
                        }

                        // lockInfo.lock_id = rx_queue_id;
                        // usleep(1000);
                        // lockInfo.lock_id &= 0x1;//0xfff; //0x7fff;
                        // lockInfo.lock_id += 20000;

                        lockInfo.action_type = GRANT_LOCK;

                        // uint16_t randop_tmp = randop[c];
                        // if((randop_tmp & 0x1) == 1){
                        //     lockInfo.lock_type = EXCLU_LOCK;
                        // }
                        // else{
                        //     lockInfo.lock_type = SHARE_LOCK;
                        // }
                        lockInfo.lock_type = SHARE_LOCK;//SHARE_LOCK;//SHARE_LOCK;//SHARE_LOCK;//EXCLU_LOCK;
                        randop[c] ^= randop[c] >> 7;
                        randop[c] ^= randop[c] << 9;
                        randop[c] ^= randop[c] >> 13;
                        
                        
                        lockInfo.resend_count = 0;
                        lockInfo.last_send_time = cur_tsc;
                        lockInTxn_arr[c][i] = lockInfo;
                        //fprintf(fout, "613 step 0: send lock_id:%d, txn_id:%d, cid:%d\n", lockInfo.lock_id, txnInfo.txn_id, txnInfo.cid);
 
                        getRespFlag_arr[c][i] = RESPFLAG_ACQUIRE_WAIT;
                        mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]); 
                        generate_lock_request_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, lockInfo, txnInfo, ip_src_pton, ip_dst_pton, 0);
                        enqueue_pkt_with_thres(lcore_id, mbuf, 32, 0); // enqueue_pkt_with_thres(lcore_id, mbuf, txnInfo.lock_num, 0);
                        pkts_send_ms ++; 
                        pkts_send_counter ++;
                        // printf("wait input:");
                        // getchar();

                        tput_stat[lcore_id].tx += 1;
                        tput_stat[lcore_id].tx_grant += 1;
                    }
                    stepFlag[c] = 1;//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                    next_deadlock_check_tsc[c] = cur_tsc + deadlock_check_interval_us;
                }

                // stage: check rcv 
                else if(stepFlag[c] == 1){ 

                    struct_txnInfo txnInfo = txn_arr[c];
                    int resendFlag = 0;
                    int deadLockFlag = 0;
                    int allSuccFlag = 1; 

                    if(cur_tsc > next_deadlock_check_tsc[c]){
                        deadLockFlag = 1;
                        tput_stat[lcore_id].txn_fail += 1;
                        // printf("731:txnFail: getRespFlag_arr[c][i] = %d, lock_id = %d\n", getRespFlag_arr[0][0], lockInTxn_arr[0][0].lock_id);
                        fprintf(fout, "731 step 1(txnFail) getRespFlag_arr[%d][%d] = %d, lock_id = %d\n", c, 0, getRespFlag_arr[c][0], lockInTxn_arr[c][0].lock_id);
                    }

                    if(deadLockFlag == 0){
                        // printf("check point 0\n");
                        for(int i = 0; i < txnInfo.lock_num; i++){
                            struct_lockInfo lockInfo = lockInTxn_arr[c][i];
                            if(getRespFlag_arr[c][i] == RESPFLAG_ACQUIRE_MODFAIL){
                                // // printf("check point 1\n");
                                // allSuccFlag = 0;
                                // if(likely(cur_tsc < (next_deadlock_check_tsc[c] - padding_us))){
                                //     resendFlag = 1;
                                //     lockInfo.action_type = GRANT_LOCK;//RE_GRANT_LOCK;
                                //     mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]);
                                //     generate_lock_request_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, lockInfo, txnInfo, ip_src_pton, ip_dst_pton, 0);
                                //     enqueue_pkt_with_thres(lcore_id, mbuf, 32, 0); // enqueue_pkt_with_thres(lcore_id, mbuf, txnInfo.lock_num, 0);//
                                //     pkts_send_ms ++;
                                //     getRespFlag_arr[c][i] = RESPFLAG_ACQUIRE_WAIT;
                                //     lockInfo.last_send_time = cur_tsc;
                                //     lockInTxn_arr[c][i] = lockInfo;
                                //     tput_stat[lcore_id].tx_resend += 1;
                                // }
                            }
                            else if(getRespFlag_arr[c][i] == RESPFLAG_ACQUIRE_FAIL){
                                // // printf("check point 1\n");
                                // allSuccFlag = 0;
                                // if(likely(cur_tsc < (next_deadlock_check_tsc[c] - padding_us))){
                                //     resendFlag = 1;
                                //     lockInfo.action_type = GRANT_LOCK;//RE_GRANT_LOCK;
                                //     mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]);
                                //     generate_lock_request_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, lockInfo, txnInfo, ip_src_pton, ip_dst_pton, 0);
                                //     enqueue_pkt_with_thres(lcore_id, mbuf, 32, 0); // enqueue_pkt_with_thres(lcore_id, mbuf, txnInfo.lock_num, 0);//
                                //     pkts_send_ms ++;
                                //     getRespFlag_arr[c][i] = RESPFLAG_ACQUIRE_WAIT;
                                //     lockInfo.last_send_time = cur_tsc;
                                //     lockInTxn_arr[c][i] = lockInfo;
                                //     tput_stat[lcore_id].tx_resend += 1;
                                // }
                            }
                            else if(getRespFlag_arr[c][i] == RESPFLAG_ACQUIRE_WAIT){
                                // printf("check point 2\n");
                                allSuccFlag = 0;
                                if(unlikely(cur_tsc >= resend_check_interval_us + lockInfo.last_send_time && cur_tsc < next_deadlock_check_tsc[c] - padding_us)){
                                    // printf("check point 3\n");
                                    resendFlag = 1;
                                    lockInfo.action_type = GRANT_LOCK;//RE_GRANT_LOCK;
                                    mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]);
                                    //fprintf(fout, "613 step 1(acquireWait) send lock_id:%d, txn_id:%d, cid:%d\n", lockInfo.lock_id, txnInfo.txn_id, txnInfo.cid);
                                    generate_lock_request_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, lockInfo, txnInfo, ip_src_pton, ip_dst_pton, 0);
                                    enqueue_pkt_with_thres(lcore_id, mbuf, 32, 0); // enqueue_pkt_with_thres(lcore_id, mbuf, txnInfo.lock_num, 0);//
                                    pkts_send_ms ++;
                                    pkts_send_counter ++;
                                    getRespFlag_arr[c][i] = RESPFLAG_ACQUIRE_WAIT;
                                    lockInfo.last_send_time = cur_tsc;
                                    lockInTxn_arr[c][i] = lockInfo;

                                    tput_stat[lcore_id].tx += 1;
                                    tput_stat[lcore_id].tx_grant_re4wait += 1;
                                }
                            }
                            else if(getRespFlag_arr[c][i] == RESPFLAG_ACQUIRE_INQUEUE){
                                allSuccFlag = 0;
                            }
                        } 
                    } 

                    if(deadLockFlag == 1 || allSuccFlag == 1){
                        for(int i = 0; i < txnInfo.lock_num; i++){
                            if(getRespFlag_arr[c][i] == RESPFLAG_ACQUIRE_HAVEGOTTEN){
                                getRespFlag_arr[c][i] = RESPFLAG_RELEASE_WAIT;
                            }
                            else{
                                getRespFlag_arr[c][i] = RESPFLAG_IDLE;
                            }
                        }
                        stepFlag[c] = 2;// stepFlag[c] = 2;
                    }
                }  

                // stage:send batch--release
                else if(stepFlag[c] == 2){
                    struct_txnInfo txnInfo = txn_arr[c];

                    for(int i = 0; i < txnInfo.lock_num; i++){
                        struct_lockInfo lockInfo = lockInTxn_arr[c][i];

                        if(getRespFlag_arr[c][i] == RESPFLAG_RELEASE_WAIT){
                            //fprintf(fout, "613 step 2: send lock_id:%d, txn_id:%d, cid:%d\n", lockInfo.lock_id, txnInfo.txn_id, txnInfo.cid);
                            lockInfo.action_type = RELEA_LOCK;
                            lockInfo.resend_count = 0;
                            lockInfo.last_send_time = cur_tsc;
                            lockInTxn_arr[c][i] = lockInfo;
                            mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]);
                            generate_lock_request_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, lockInfo, txnInfo, ip_src_pton, ip_dst_pton, 0);
                            enqueue_pkt_with_thres(lcore_id, mbuf, 32, 0); // enqueue_pkt_with_thres(lcore_id, mbuf, txnInfo.lock_num, 0);
                            pkts_send_ms ++;
                            pkts_send_counter ++;
                            tput_stat[lcore_id].tx += 1;
                            tput_stat[lcore_id].tx_release += 1;
                        }
                    }
                    stepFlag[c] = 3;
                    next_deadlock_check_tsc[c] = cur_tsc + deadlock_check_interval_us;
                } 

                // stage: check rcv
                else if(stepFlag[c] == 3){

                    struct_txnInfo txnInfo = txn_arr[c];
                    int resendFlag = 0;
                    int deadLockFlag = 0;
                    int allSuccFlag = 1;

                    if(cur_tsc > next_deadlock_check_tsc[c]){
                        deadLockFlag = 1;
                        fprintf(fout, "731 step 3(releaseFail) getRespFlag_arr[%d][%d] = %d, lock_id = %d\n", c, 0, getRespFlag_arr[c][0], lockInTxn_arr[c][0].lock_id);
                    }

                    if(deadLockFlag == 0){
                        for(int i = 0; i < txnInfo.lock_num; i++){
                            struct_lockInfo lockInfo = lockInTxn_arr[c][i];
                            if(getRespFlag_arr[c][i] == RESPFLAG_RELEASE_MODFAIL){     
                                allSuccFlag = 0;
                                if(unlikely(cur_tsc < (next_deadlock_check_tsc[c] - padding_us))){
                                    resendFlag = 1;
                                    lockInfo.action_type = RELEA_LOCK;//RE_RELEA_LOCK;
                                    mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]);
                                    generate_lock_request_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, lockInfo, txnInfo, ip_src_pton, ip_dst_pton, 0);
                                    enqueue_pkt_with_thres(lcore_id, mbuf, 32, 0); // enqueue_pkt_with_thres(lcore_id, mbuf, txnInfo.lock_num, 0);
                                    pkts_send_ms ++;
                                    pkts_send_counter ++;
                                    getRespFlag_arr[c][i] = RESPFLAG_RELEASE_WAIT;
                                    lockInfo.last_send_time = cur_tsc;
                                    lockInTxn_arr[c][i] = lockInfo;

                                    tput_stat[lcore_id].tx += 1;
                                    tput_stat[lcore_id].tx_release_re4modfail += 1;
                                }
                            }
                            else if(getRespFlag_arr[c][i] == RESPFLAG_RELEASE_FAIL){
                                // no corresponding lock is acquired, maybe be released by the server
                                // allSuccFlag = 0;
                                // if(unlikely(cur_tsc < (next_deadlock_check_tsc[c] - padding_us))){
                                //     resendFlag = 1;
                                //     lockInfo.action_type = RELEA_LOCK;//RE_RELEA_LOCK;
                                //     mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]);
                                //     generate_lock_request_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, lockInfo, txnInfo, ip_src_pton, ip_dst_pton, 0);
                                //     enqueue_pkt_with_thres(lcore_id, mbuf, 32, 0); // enqueue_pkt_with_thres(lcore_id, mbuf, txnInfo.lock_num, 0);
                                //     pkts_send_ms ++;
                                //     getRespFlag_arr[c][i] = RESPFLAG_RELEASE_WAIT;
                                //     lockInfo.last_send_time = cur_tsc;
                                //     lockInTxn_arr[c][i] = lockInfo;
                                //     tput_stat[lcore_id].tx_resend += 1;
                                // }
                            }
                            else if(getRespFlag_arr[c][i] == RESPFLAG_RELEASE_WAIT){
                                allSuccFlag = 0;
                                if (unlikely(cur_tsc >= resend_check_interval_us + lockInfo.last_send_time && cur_tsc < next_deadlock_check_tsc[c] - padding_us)) {
                                    resendFlag = 1;
                                    lockInfo.action_type = RELEA_LOCK;//RE_RELEA_LOCK;
                                    mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]);
                                    //fprintf(fout, "613 step 3: send lock_id:%d, txn_id:%d, cid:%d\n", lockInfo.lock_id, txnInfo.txn_id, txnInfo.cid);
                                    generate_lock_request_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, lockInfo, txnInfo, ip_src_pton, ip_dst_pton, 0);
                                    enqueue_pkt_with_thres(lcore_id, mbuf, 32, 0); // enqueue_pkt_with_thres(lcore_id, mbuf, txnInfo.lock_num, 0);
                                    pkts_send_ms ++;
                                    pkts_send_counter ++;
                                    getRespFlag_arr[c][i] = RESPFLAG_RELEASE_WAIT;
                                    lockInfo.last_send_time = cur_tsc;
                                    lockInTxn_arr[c][i] = lockInfo;

                                    tput_stat[lcore_id].tx += 1;
                                    tput_stat[lcore_id].tx_release_re4wait += 1;
                                }
                            }
                        }
                    }
                    if(deadLockFlag == 1 || allSuccFlag == 1){
                        for(int i = 0; i < txnInfo.lock_num; i++){
                            getRespFlag_arr[c][i] = RESPFLAG_IDLE;
                        }
                        stepFlag[c] = 0;

                        if(unlikely(cur_tsc > sendEnd_us)){
                            sendEnd_flag[c] = 1;
                        }
                    }
                } 
            } 
        }
        enqueue_pkt_with_thres(lcore_id, NULL, 1, 1);
    }
    endFlag[lcore_id] = 1;
    pthread_barrier_wait(&barrier);
    printf("checkpoint: %d\n", lcore_id);
    sleep(2);
    return 0; 
}


static int32_t np_client_txrx_loop_kvs(uint32_t lcore_id, uint32_t client_id, uint16_t rx_queue_id, struct_zipfPara zipfPara, FILE* fout, FILE* fout_fpgaCheck) {
    printf("%lld entering TX loop for write on lcore %u, numa_id_of_lcore = %d\n", (long long)time(NULL), lcore_id, rte_socket_id());

    //trace
    // printf("check point 0\n");
    uint32_t *req_arr = (uint32_t *)malloc(sizeof(uint32_t *) * zipfPara.reqNumReadIn);
    uint32_t *req_hc_arr = (uint32_t *)malloc(sizeof(uint32_t *) * zipfPara.reqNumReadIn);
    char reqFile[128];
    char reqFile_hc[128];
    if(zipfPara.distribution_type == 0){
        sprintf(reqFile, "./trace/res_zipf/a%0.2f_n%u_rN%d_sd%u.txt", zipfPara.a, zipfPara.n, zipfPara.reqNum, zipfPara.lfsrSeed);
        sprintf(reqFile_hc, "./trace/res_zipf_hc/a%0.2f_n%u_rN%d_sd%u_hcT1_hcN%u.txt", zipfPara.a, zipfPara.n, zipfPara.reqNum, zipfPara.lfsrSeed, zipfPara.hotChangedNum);    
    }
    else{
        sprintf(reqFile, "./trace/res_rect/a%0.2f_n%u_rN%d_sd%u.txt", zipfPara.a, zipfPara.n, zipfPara.reqNum, zipfPara.lfsrSeed);
        sprintf(reqFile_hc, "./trace/res_rect_hc/a%0.2f_n%u_rN%d_sd%u_hcT1_hcN%u.txt", zipfPara.a, zipfPara.n, zipfPara.reqNum, zipfPara.lfsrSeed, zipfPara.hotChangedNum);    
    }
 

    printf("hotChangedNum: %u\n", zipfPara.hotChangedNum);
    printf("traceFile: %s\n", reqFile);
    FILE *reqFp=fopen(reqFile,"r");
    FILE *reqFp_hc=fopen(reqFile_hc,"r");
    if(!reqFp){
       printf("error when open %s\n", reqFile);
       endFlag[lcore_id] = 1;
       return -1;
    }
    if(!reqFp_hc){
       printf("error when open %s\n", reqFile_hc);
       endFlag[lcore_id] = 1;
       return -1;
    } 
    // printf("check point 1\n");
    for(uint32_t i = 0; i < zipfPara.reqNumReadIn; i++){
        fscanf(reqFp,"%u", req_arr+i);
        fscanf(reqFp_hc,"%u", req_hc_arr+i);
        if(i == 0) printf("idx=0, lockid-d = %d, lockid-u = %u\n", req_arr[i], req_arr[i]);
    }
    uint32_t reqIdx = 0;
    //---------------------------------------------------
    uint32_t reqK[MAX_CLIENT_IN_LCORE];
    for(uint32_t i = 0; i < MAX_CLIENT_IN_LCORE; i++){
        reqK[i] = i;//rand();//cid[i];
    }
    uint16_t randop[MAX_CLIENT_IN_LCORE];
    for(int i = 0; i < MAX_CLIENT_IN_LCORE; i++){
        randop[i] = rand();
    }
    //---------------------------------------------------

    // initiate header_template_local
    uint8_t header_template_local[HEADER_TEMPLATE_SIZE];
    init_header_template_local(header_template_local);
    // initiate lconf
    struct lcore_configuration *lconf = &lcore_conf[lcore_id];
    // memory related points
    struct rte_mbuf *mbuf;
    
    // time related configurations 
    uint64_t cur_tsc = rte_rdtsc();
    uint64_t ms_tsc = rte_get_tsc_hz() / 1000; 
    uint64_t next_ms_tsc = cur_tsc + ms_tsc;
    uint64_t pkts_send_ms = 0;
    uint64_t pkts_send_counter = 0;

    // mine
    uint64_t resend_check_interval_us = rte_get_tsc_hz() / 1000000 * RESEND_CHECK_INTERVAL_US;
    uint64_t deadlock_check_interval_us = rte_get_tsc_hz() / 1000000 * zipfPara.deadlock_check_interval_us;
    uint64_t padding_us = rte_get_tsc_hz() / 1000000 * RESEND_CHECK_PADDING_US;
    uint64_t end_us = cur_tsc + rte_get_tsc_hz() / 1000000 * END_US;
    uint64_t hc_us = cur_tsc + rte_get_tsc_hz() / 1000000 * HC_US;
    uint64_t sendEnd_us = cur_tsc + rte_get_tsc_hz() / 1000000 * SENDEND_US;
 
    struct rte_mbuf *mbuf_burst[NC_MAX_BURST_SIZE];
    struct rte_mbuf *mbuf_rcv;
    // printf("check point 1.\n");

    //debug
    uint64_t update_tsc = rte_get_tsc_hz(); // in second
    uint64_t next_update_tsc = cur_tsc + update_tsc;

    int sendEnd_flag[MAX_CLIENT_IN_LCORE] = {0};

    int count[MAX_CLIENT_IN_LCORE] = {0};
    for(int i = 0; i < MAX_CLIENT_IN_LCORE; i++){
        count[i] = 0;
    }
    
    while (1) {
    //     // sleep(0.5);
    //     // read current time
    //     // printf("pkts_send_ms = %d\n", pkts_send_ms);
        cur_tsc = rte_rdtsc();
        // clean packet counters for each ms
        if(unlikely(cur_tsc > end_us)){
            break;
        }
        if (unlikely(cur_tsc > next_ms_tsc)) {
            pkts_send_ms = 0;
            next_ms_tsc += ms_tsc;
        } 

        // RX
        ////fprintf(fout, "613 check point: rx start.\n");
        int loop = MAX_CLIENT_IN_LCORE / NC_MAX_BURST_SIZE;
        if(loop == 0) loop = 1;
        for(int c = 0; c < loop; c++){ 
            for (int i = 0; i < lconf->n_rx_queue; i++) {
                uint32_t nb_rx = rte_eth_rx_burst(lconf->port, lconf->rx_queue_list[i], mbuf_burst, NC_MAX_BURST_SIZE);
                tput_stat[lcore_id].rx += nb_rx;
                for (int j = 0; j < nb_rx; j++) {
                
                    mbuf_rcv = mbuf_burst[j];
                    rte_prefetch0(rte_pktmbuf_mtod(mbuf_rcv, void *)); 
                    struct rte_ether_hdr* eth = rte_pktmbuf_mtod(mbuf_rcv, struct rte_ether_hdr *);
                    struct rte_ipv4_hdr *ip = (struct rte_ipv4_hdr *)((uint8_t*) eth + sizeof(struct rte_ether_hdr));
                    struct rte_udp_hdr *udp = (struct rte_udp_hdr *)((uint8_t*) ip + sizeof(struct rte_ipv4_hdr));
                    // printf("dst_port = %d, src_port = %d\n", ntohs(udp->dst_port), ntohs(udp->src_port));
                    // parse NetLock header,could be changed
                    MessageHeader* message_header = (MessageHeader*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE);
                    uint8_t  rcv_opt = (uint8_t) message_header->opType;
                    float latency = timediff_in_us(rte_rdtsc(), message_header->timestamp);

                    // printf("latency = %f\n", latency)
                    
                    //fprintf(fout, "613 rcv lock_id:%d, txn_id:%d, cid:%d, opType:%d, mode:%d\n", rcv_lock_id, rcv_txn_id, rcv_cid, rcv_opt, rcv_mode);
                    // printf("907 rcv lock_id:%d, txn_id:%d, cid:%d, opType:%d, mode:%d\n", rcv_lock_id, rcv_txn_id, rcv_cid, rcv_opt, rcv_mode);
                    compute_latency(&latency_stat_c[lcore_id], latency);

                    // tput_stat[lcore_id].rx += 1;
                    switch(rcv_opt){
                        case OP_GETSUCC:{tput_stat[lcore_id].rx_getSucc += 1; break;}
                        // case OP_PUTSUCC:{tput_stat[lcore_id].rx_putSucc += 1; break;}
                        case OP_PUTSUCC_SWITCHHIT:{tput_stat[lcore_id].rx_putSucc += 1; break;}
                        case OP_PUTSUCC_SWITCHMISS:{tput_stat[lcore_id].rx_putSucc += 1; break;}
                        case OP_DELSUCC:{tput_stat[lcore_id].rx_delSucc += 1; break;}
                        case OP_GETFAIL:{tput_stat[lcore_id].rx_getFail += 1; break;}
                        case OP_PUTFAIL:{tput_stat[lcore_id].rx_putFail += 1; break;}
                        // case OP_PUTFAIL_SWITCHHIT:{tput_stat[lcore_id].rx_putFail += 1; break;}
                        // case OP_PUTFAIL_SWITCHMISS:{tput_stat[lcore_id].rx_putFail += 1; break;}
                        case OP_DELFAIL:{tput_stat[lcore_id].rx_delFail += 1; break;}
                        default:{
                            // printf("other_opt = %d\n", rcv_opt);
                            break;
                        }
                    }
                    rte_pktmbuf_free(mbuf_rcv);
                }
            }
        }

        // TX: generate packet, put in TX queue
        ////fprintf(fout, "613 check point: tx start.\n");
        if (pkts_send_ms < wpkts_send_limit_ms) {
            for(int c = 0; c < MAX_CLIENT_IN_LCORE; c++){
                // stage:send batch--require
                if(sendEnd_flag[c] == 0){
                    // tput_stat[lcore_id].tx += 1;
                    if(reqIdx == zipfPara.reqNumReadIn - 1){
                        reqIdx = 0;
                    }
                    else{
                        reqIdx++;
                    }

                    struct_kvReqInfo reqInfo;  
                    if(((int)(req_arr[reqIdx]) - hc_flag * 10000) < 0){ //��Ȼ����ѭ�������⵫��Ŀǰ���滻����������ѭ��
                        reqInfo.key = zipfPara.reqNumReadIn + req_arr[reqIdx] - hc_flag * 10000;
                    } 
                    else{
                        reqInfo.key = req_arr[reqIdx] - hc_flag * 10000;
                    }
                    // if(hc_flag == 1){
                    //     reqInfo.key = req_hc_arr[reqIdx];
                    // } 
                    // else{
                    //     reqInfo.key = req_arr[reqIdx];
                    // }                      
                    // reqInfo.key = 1;
                    reqInfo.op_type = OP_GET;//OP_PUT_SWITCHMISS;//OP_GET;
                    reqInfo.value_len = 128;
                    
                    //fprintf(fout, "613 step 0: send lock_id:%d, txn_id:%d, cid:%d\n", lockInfo.lock_id, txnInfo.txn_id, txnInfo.cid);
                    mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]); 
                    generate_kv_request_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, reqInfo, ip_src_pton, ip_dst_pton, 0);
                    enqueue_pkt_with_thres(lcore_id, mbuf, 32, 0); // enqueue_pkt_with_thres(lcore_id, mbuf, txnInfo.lock_num, 0);
                    pkts_send_ms ++; 
                    pkts_send_counter ++;
                    // printf("wait input:");
                    // getchar();
                    tput_stat[lcore_id].tx += 1;
                    tput_stat[lcore_id].tx_grant += 1;

                    if(unlikely(cur_tsc > sendEnd_us)){
                        sendEnd_flag[c] = 1;
                    }
                } 
            } 
        }
        enqueue_pkt_with_thres(lcore_id, NULL, 1, 1);
    }
    endFlag[lcore_id] = 1;
    pthread_barrier_wait(&barrier);
    printf("checkpoint: %d\n", lcore_id);
    sleep(2);
    return 0; 
}

//--------------------------------------------------------
static int32_t np_client_statistic_loop(uint32_t lcore_id, uint32_t client_id, uint16_t rx_queue_id) {
    printf("%lld entering RX loop (master loop) on lcore %u, numa_id_of_lcore = %d\n", (long long)time(NULL), lcore_id, rte_socket_id());
    // initiate header_template_local
    uint8_t header_template_local[HEADER_TEMPLATE_SIZE];
    init_header_template_local(header_template_local);
    // initiate lconf  
    struct lcore_configuration *lconf = &lcore_conf[lcore_id];
    // memory related points
    struct rte_mbuf *mbuf;
    struct rte_mbuf *mbuf_rcv;  
    struct rte_mbuf *mbuf_burst[NC_MAX_BURST_SIZE];
    // time related configurations 
    uint64_t cur_tsc = rte_rdtsc();
    uint64_t update_tsc = rte_get_tsc_hz() / 1000000 * CHECK_INTERVAL_US; // in second
    uint64_t next_update_tsc = cur_tsc + update_tsc;
    uint64_t average_start_tsc = cur_tsc + update_tsc * average_interval;
    uint64_t average_end_tsc = cur_tsc + update_tsc * average_interval * 2;
    uint64_t exit_tsc = cur_tsc + update_tsc * average_interval * 20;
    // stop flags
    uint32_t pkts_send_ms = 0;
    int stop_statis = 0;

    uint64_t check_start_tsc = cur_tsc + rte_get_tsc_hz() / 1000000 * CHECK_START_US;
    uint64_t check_end_tsc = cur_tsc + rte_get_tsc_hz() / 1000000 * CHECK_END_US;

    int flag = 0;
    int times = 0;

    uint64_t last_tsc = cur_tsc;
    uint64_t hc_us = cur_tsc + rte_get_tsc_hz() / 1000000 * HC_US;
    uint64_t hc_update_us = rte_get_tsc_hz() / 1000000 * HC_UPD_US; // in second

    while (1) { 
        uint32_t endCount = 0;
        for(int i = 0; i < NC_MAX_LCORES; i++){
            endCount += endFlag[i];
        }
        if(endCount == n_lcores - 1){ //lcore 0 just for statistics
            break;
        } 
        // read current time
        cur_tsc = rte_rdtsc();
        if(HC_MASTER == 1){
            if(last_tsc <= hc_us && cur_tsc > hc_us){
                hc_us = hc_us + hc_update_us;

                // printf("hot-in flag!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
                for(int i = 0; i < HC_SLAVE_NUM; i++){
                    char* ip_src_hc = IP_LOCAL;
                    char* ip_dst_hc = ip_dst_hc_arr[i];
                    struct rte_ether_addr mac_src_hc = {.addr_bytes = MAC_LOCAL};
                    struct rte_ether_addr mac_dst_hc = mac_dst_hc_arr[i];
                    mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]);
                    generate_hc_pkt(rx_queue_id, header_template_local, lcore_id, mbuf,\
                                    ip_src_hc, ip_dst_hc, mac_src_hc, mac_dst_hc);
                    enqueue_pkt_with_thres(lcore_id, mbuf, 32, 0);
                }
                enqueue_pkt_with_thres(lcore_id, mbuf, 1, 1);
                printf("send hot-in flag!\n");

                hc_flag++;
                // if(hc_flag == 1){
                //     hc_flag = 0;
                // }
                // else{
                //     hc_flag = 1;
                // }
            } 
            last_tsc = cur_tsc;
        }
        else{
            for (int i = 0; i < lconf->n_rx_queue; i++) {
                uint32_t nb_rx = rte_eth_rx_burst(lconf->port, lconf->rx_queue_list[i], mbuf_burst, NC_MAX_BURST_SIZE);
                tput_stat[lcore_id].rx += nb_rx;
                for (int j = 0; j < nb_rx; j++) {
                    mbuf_rcv = mbuf_burst[j];
                    rte_prefetch0(rte_pktmbuf_mtod(mbuf_rcv, void *)); 
                    struct rte_ether_hdr* eth = rte_pktmbuf_mtod(mbuf_rcv, struct rte_ether_hdr *);
                    struct rte_ipv4_hdr *ip = (struct rte_ipv4_hdr *)((uint8_t*) eth + sizeof(struct rte_ether_hdr));
                    struct rte_udp_hdr *udp = (struct rte_udp_hdr *)((uint8_t*) ip + sizeof(struct rte_ipv4_hdr));
                    // printf("dst_port = %d, src_port = %d\n", ntohs(udp->dst_port), ntohs(udp->src_port));
                    // parse NetLock header,could be changed
                    MessageHeader* message_header = (MessageHeader*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE);
                    uint8_t  rcv_opt = (uint8_t) message_header->opType;
                    if(rcv_opt == OP_HC){
                        printf("receive hot-in flag!\n");
                        hc_flag++;
                        // if(hc_flag == 1){
                        //     hc_flag = 0;
                        // }
                        // else{
                        //     hc_flag = 1;
                        // }
                    }
                    rte_pktmbuf_free(mbuf_rcv);
                }
            }
        }  

        // cur_tsc = rte_rdtsc();
 
        // if(last_tsc <= hc_us && cur_tsc > hc_us){
        //     hc_us = hc_us + hc_update_us;
        //     if(HC_MASTER == 1){
        //         // printf("hot-in flag!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        //         for(int i = 0; i < HC_SLAVE_NUM; i++){
        //             char* ip_src_hc = IP_LOCAL;
        //             char* ip_dst_hc = ip_dst_hc_arr[i];
        //             struct rte_ether_addr mac_src_hc = {.addr_bytes = MAC_LOCAL};
        //             struct rte_ether_addr mac_dst_hc = mac_dst_hc_arr[i];
        //             mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]);
        //             generate_hc_pkt(rx_queue_id, header_template_local, lcore_id, mbuf,\
        //                             ip_src_hc, ip_dst_hc, mac_src_hc, mac_dst_hc);
        //             enqueue_pkt_with_thres(lcore_id, mbuf, 32, 0);
        //         }
        //         enqueue_pkt_with_thres(lcore_id, mbuf, 1, 1);
        //         printf("send hot-in flag!\n");
        //     }
        //     else{
        //         for (int i = 0; i < lconf->n_rx_queue; i++) {
        //             uint32_t nb_rx = rte_eth_rx_burst(lconf->port, lconf->rx_queue_list[i], mbuf_burst, NC_MAX_BURST_SIZE);
        //             tput_stat[lcore_id].rx += nb_rx;
        //             for (int j = 0; j < nb_rx; j++) {
        //                 mbuf_rcv = mbuf_burst[j];
        //                 rte_prefetch0(rte_pktmbuf_mtod(mbuf_rcv, void *)); 
        //                 struct rte_ether_hdr* eth = rte_pktmbuf_mtod(mbuf_rcv, struct rte_ether_hdr *);
        //                 struct rte_ipv4_hdr *ip = (struct rte_ipv4_hdr *)((uint8_t*) eth + sizeof(struct rte_ether_hdr));
        //                 struct rte_udp_hdr *udp = (struct rte_udp_hdr *)((uint8_t*) ip + sizeof(struct rte_ipv4_hdr));
        //                 // printf("dst_port = %d, src_port = %d\n", ntohs(udp->dst_port), ntohs(udp->src_port));
        //                 // parse NetLock header,could be changed
        //                 MessageHeader* message_header = (MessageHeader*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE);
        //                 uint8_t  rcv_opt = (uint8_t) message_header->opType;
        //                 if(rcv_opt == OP_HC){
        //                     printf("receive hot-in flag!\n");
        //                 }
        //                 rte_pktmbuf_free(mbuf_rcv);
        //             }
        //         }
        //     }
        //     if(hc_flag == 1){
        //         hc_flag = 0;
        //     }
        //     else{
        //         hc_flag = 1;
        //     }
        // }
        // last_tsc = cur_tsc;

        // if(HC_MASTER == 1){
        //     if(cur_tsc > hc_us && hc_flag == 0){
        //         // printf("hot-in flag!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        //         hc_flag = 1; 
        //         for(int i = 0; i < HC_SLAVE_NUM; i++){
        //             char* ip_src_hc = IP_LOCAL;
        //             char* ip_dst_hc = ip_dst_hc_arr[i];
        //             struct rte_ether_addr mac_src_hc = {.addr_bytes = MAC_LOCAL};
        //             struct rte_ether_addr mac_dst_hc = mac_dst_hc_arr[i];
        //             mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]);
        //             generate_hc_pkt(rx_queue_id, header_template_local, lcore_id, mbuf,\
        //                             ip_src_hc, ip_dst_hc, mac_src_hc, mac_dst_hc);
        //             enqueue_pkt_with_thres(lcore_id, mbuf, 32, 0);
        //         }
        //         enqueue_pkt_with_thres(lcore_id, mbuf, 1, 1);
        //         printf("send hot-in flag!\n");
        //     }
        // }
        // else if(hc_flag == 0){
        //     for (int i = 0; i < lconf->n_rx_queue; i++) {
        //         uint32_t nb_rx = rte_eth_rx_burst(lconf->port, lconf->rx_queue_list[i], mbuf_burst, NC_MAX_BURST_SIZE);
        //         tput_stat[lcore_id].rx += nb_rx;
        //         for (int j = 0; j < nb_rx; j++) {
        //             mbuf_rcv = mbuf_burst[j];
        //             rte_prefetch0(rte_pktmbuf_mtod(mbuf_rcv, void *)); 
        //             struct rte_ether_hdr* eth = rte_pktmbuf_mtod(mbuf_rcv, struct rte_ether_hdr *);
        //             struct rte_ipv4_hdr *ip = (struct rte_ipv4_hdr *)((uint8_t*) eth + sizeof(struct rte_ether_hdr));
        //             struct rte_udp_hdr *udp = (struct rte_udp_hdr *)((uint8_t*) ip + sizeof(struct rte_ipv4_hdr));
        //             // printf("dst_port = %d, src_port = %d\n", ntohs(udp->dst_port), ntohs(udp->src_port));
        //             // parse NetLock header,could be changed
        //             MessageHeader* message_header = (MessageHeader*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE);
        //             uint8_t  rcv_opt = (uint8_t) message_header->opType;
        //             if(rcv_opt == OP_HC){
        //                 hc_flag = 1;
        //                 printf("receive hot-in flag!\n");
        //             }
        //             rte_pktmbuf_free(mbuf_rcv);
        //         }
        //     }
        // }

    //-----------------------------------------------------------process for statistics-----------------------------------------------------------
    //-----------------------------------------------------------begin-----------------------------------------------------------

        // print stats at master lcore 
        if ((lcore_id == 0) && (update_tsc > 0)) {        
            if (unlikely(cur_tsc > check_start_tsc && cur_tsc < check_end_tsc && cur_tsc > next_update_tsc)) {
                times += 1;
#ifdef APP_LOCK
                print_per_core_throughput_lock();
#elif APP_KVS 
                print_per_core_throughput_kvs();
#endif
                // printf("latency\n");
                // print_latency(latency_stat_c); 
                next_update_tsc = cur_tsc + update_tsc;
                // printf("cur_tsc = %lf, average_end_tsc = %lf, leftover interval time in seconds = %lf\n", (double)cur_tsc, (double)average_end_tsc, (double)(((average_end_tsc - cur_tsc)) / rte_get_tsc_hz()));
            }
        } 
    } 
    return 0;
}  

//----------------------------------------------------------------------------------------------

FILE* fout;
FILE* fout_fpgaCheck;
FILE* fout_stats;
static int32_t np_client_loop(__attribute__((unused)) void *arg) {
    uint32_t lcore_id = rte_lcore_id();
    if ((lcore_id < 1)) {
        np_client_statistic_loop(lcore_id, lcore_id, lcore_id_2_rx_queue_id[lcore_id]);
    }
    else {
#ifdef APP_LOCK
        np_client_txrx_loop_lock(lcore_id, lcore_id, lcore_id_2_rx_queue_id[lcore_id], gb_zipfPara, fout, fout_fpgaCheck);
#elif APP_KVS
        np_client_txrx_loop_kvs(lcore_id, lcore_id, lcore_id_2_rx_queue_id[lcore_id], gb_zipfPara, fout, fout_fpgaCheck);
#endif
        // gb_zipfPara.lfsrSeed++;
    }
    return 0; 
}

static void nc_parse_args_help(void) {
#ifdef APP_LOCK
    printf("simple_socket [EAL options] --\n"
           "  -n size of lock id range\n"
           "  -a zipf/rect parameter alpha * 100\n"
           "  -k number of locks in 1 txn\n"
           "  -t number of txns in a trace file\n"
           "  -s lfsr seed\n"
           "  -d deadlock check time interval (us) \n"
           "  -h number of hot-in locks\n"
           "  -z 0:zipf, 1:rect\n"
           "  -w fgpa wait time (ms) for stats\n"
           "  -e number of fgpa cache set\n");
#elif APP_KVS
    printf("simple_socket [EAL options] --\n"
           "  -n size of key range\n"
           "  -a zipf/rect parameter alpha * 100\n"
           "  -r number of req in a trace file\n"
           "  -s lfsr seed\n"
           "  -h number of hot-in keys\n"
           "  -z 0:zipf, 1:rect\n"
           "  -w fgpa wait time (ms) for stats\n"
           "  -e number of fgpa cache set\n");
#endif
}

static int nc_parse_args(int argc, char **argv) {
    int opt;
#ifdef APP_LOCK
    while ((opt = getopt(argc, argv, "n:a:k:t:s:d:h:z:w:e:")) != -1) {
        switch (opt) {
        case 'n':
            gb_zipfPara.n = atoi(optarg);
            break;
        case 'a':
            gb_zipfPara.a = atoi(optarg) / 100.0;
            break; 
        case 'k':
            gb_zipfPara.lockNumInTxn = atoi(optarg); 
            break;
        case 't':
            gb_zipfPara.txnNum = atoi(optarg);
            gb_zipfPara.txnNumReadIn = gb_zipfPara.txnNum;
            break;
        case 's':
            gb_zipfPara.lfsrSeed = atoi(optarg);
            break;
        case 'd':
            gb_zipfPara.deadlock_check_interval_us = atoi(optarg);
            break;
        case 'h':
            gb_zipfPara.hotChangedNum = atoi(optarg);
            break;
        case 'z':
            gb_zipfPara.distribution_type = atoi(optarg);
            break;
        case 'w':
            gb_zipfPara.fpga_wait_ms = atoi(optarg);
            break;
        case 'e':
            gb_zipfPara.fpga_cacheSetNum = atoi(optarg);
            break;
        default:
            nc_parse_args_help();
            return -1;
        }
    }
#elif APP_KVS
    while ((opt = getopt(argc, argv, "n:a:r:s:h:z:w:e:")) != -1) {
        switch (opt) {
        case 'n':
            gb_zipfPara.n = atoi(optarg);
            break;
        case 'a':
            gb_zipfPara.a = atoi(optarg) / 100.0;
            break; 
        case 'r':
            gb_zipfPara.reqNum = atoi(optarg);
            gb_zipfPara.reqNumReadIn = gb_zipfPara.reqNum;
            break;
        case 's':
            gb_zipfPara.lfsrSeed = atoi(optarg);
            break;
        case 'h':
            gb_zipfPara.hotChangedNum = atoi(optarg);
            break;
        case 'z':
            gb_zipfPara.distribution_type = atoi(optarg);
            break;
        case 'w':
            gb_zipfPara.fpga_wait_ms = atoi(optarg);
            break;
        case 'e':
            gb_zipfPara.fpga_cacheSetNum = atoi(optarg);
            break;
        default:
            nc_parse_args_help();
            return -1;
        }
    }
#endif
    return 1;
}

int main(int argc, char **argv) {
    // fout = fopen("./log/log.txt", "a");
    // parse default arguments
    int ret;
    ret = rte_eal_init(argc, argv);
    if (ret < 0) {
        rte_exit(EXIT_FAILURE, "invalid EAL arguments\n");
    }
    argc -= ret;
    argv += ret;
    //user arguments
    ret = nc_parse_args(argc, argv);
    if (ret < 0) {
        rte_exit(EXIT_FAILURE, "invalid user arguments\n");
    }

    // gb_zipfPara.hotChangedNum = 256;
    nc_init();
    pthread_barrier_init(&barrier, NULL, n_lcores - 1);

    inet_pton(AF_INET, ip_src, &(ip_src_pton));   
    inet_pton(AF_INET, ip_dst, &(ip_dst_pton)); 

    int numa_id_of_port = rte_eth_dev_socket_id(0);
    printf("numa_id_of_port = %d\n", numa_id_of_port);

    // for(int i = 1; i <= 11; i++){
    //     lcore_id_2_rx_queue_id[i] = i - 1;
    // }
    // for(int i = 24; i <= 35; i++){
    //     lcore_id_2_rx_queue_id[i] = i - 13;
    // }     

#ifdef APP_LOCK
    lockInTxnNum = gb_zipfPara.lockNumInTxn;
    clientInLcoreNum = MAX_CLIENT_IN_LCORE; 
    lockInSwitchValidNum = gb_zipfPara.n;
    // log file and statistics file
    char logFile[128];
    sprintf(logFile, "./log/n%u_a%0.2f_k%u_t%u_s%u_d%lu_h%u_z%u_w%u_e%u.txt", \
    gb_zipfPara.n, gb_zipfPara.a, gb_zipfPara.lockNumInTxn, gb_zipfPara.txnNum, \
    gb_zipfPara.lfsrSeed, gb_zipfPara.deadlock_check_interval_us, \
    gb_zipfPara.hotChangedNum, gb_zipfPara.distribution_type, gb_zipfPara.fpga_wait_ms, gb_zipfPara.fpga_cacheSetNum);
    fout = fopen(logFile, "w");

    char statsFile[128];
    sprintf(statsFile, "./stats/n%u_a%0.2f_k%u_t%u_s%u_d%lu_h%u_z%u_w%u_e%u.txt", \
    gb_zipfPara.n, gb_zipfPara.a, gb_zipfPara.lockNumInTxn, gb_zipfPara.txnNum, \
    gb_zipfPara.lfsrSeed, gb_zipfPara.deadlock_check_interval_us, \
    gb_zipfPara.hotChangedNum, gb_zipfPara.distribution_type, gb_zipfPara.fpga_wait_ms, gb_zipfPara.fpga_cacheSetNum);
    fout_stats = fopen(statsFile, "w");
#elif APP_KVS
    char logFile[128];
    sprintf(logFile, "./log/n%u_a%0.2f_r%u_s%u_h%u_z%u_w%u_e%u.txt", \
    gb_zipfPara.n, gb_zipfPara.a, gb_zipfPara.reqNum, \
    gb_zipfPara.lfsrSeed, \
    gb_zipfPara.hotChangedNum, gb_zipfPara.distribution_type, gb_zipfPara.fpga_wait_ms, gb_zipfPara.fpga_cacheSetNum);
    fout = fopen(logFile, "w");

    char statsFile[128];
    sprintf(statsFile, "./stats/n%u_a%0.2f_r%u_s%u_h%u_z%u_w%u_e%u.txt", \
    gb_zipfPara.n, gb_zipfPara.a, gb_zipfPara.reqNum, \
    gb_zipfPara.lfsrSeed, \
    gb_zipfPara.hotChangedNum, gb_zipfPara.distribution_type, gb_zipfPara.fpga_wait_ms, gb_zipfPara.fpga_cacheSetNum);
    fout_stats = fopen(statsFile, "w");
#endif


    fout_fpgaCheck = fopen("./log/log_fpgaCheck.txt", "a");
    if(fout == NULL){
        printf("open fout fail\n");
    } 
    if(fout_fpgaCheck == NULL){
        printf("open fout_fpgaCheck fail\n");
    }
    if(fout_stats == NULL){
        printf("open fout_stats fail\n");
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
    
    //print statistics
#ifdef APP_LOCK
    print_time_trace_lock((CHECK_END_US - CHECK_START_US) / CHECK_INTERVAL_US, fout_stats);
#elif APP_KVS
    print_time_trace_kvs((CHECK_END_US - CHECK_START_US) / CHECK_INTERVAL_US, fout_stats);
#endif

    fclose(fout);
    fclose(fout_fpgaCheck);
    fclose(fout_stats);

    return 0;
}