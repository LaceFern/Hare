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

#include "./include/util.h"
#include "./include/cache_upd_ctrl.h"

/*
 * constants
 */
#define MIN_LOSS_RATE           0.01
#define MAX_LOSS_RATE           0.05
#define PKTS_SEND_LIMIT_MIN_MS  300
#define PKTS_SEND_LIMIT_MAX_MS  2500
#define PKTS_SEND_RESTART_MS    300
#define NUM_LCORES              32

// uint32_t wpkts_send_limit_ms = 530;//8192;//8192;

/* 
 * custom types 
 */
//-------------------------------------------------------- KVS --------------------------------------------------------
uint8_t valueLenTable[MAX_SERVICE_SIZE] = {0};
char valueTable[MAX_SERVICE_SIZE][VALUE_BYTES] = {0};
char keyTable[MAX_SERVICE_SIZE][KEY_BYTES] = {0};
//-------------------------------------------------------- ctrl --------------------------------------------------------


/* 
 * global variables  
 */
static uint32_t second = 0;
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
uint32_t wpkts_send_limit_ms = 435;//435; //435; //435;//8000;//435;//8000;//435;
uint32_t wpkts_send_limit_s = 435000;//435; //435; //435;//8000;//435;//8000;//435;
uint32_t average_interval = 5;
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------



// generate write request packet for server
static void generate_write_request_pkt_server(uint16_t rx_queue_id, uint8_t header_template[HEADER_TEMPLATE_SIZE], uint32_t lcore_id, struct rte_mbuf *mbuf, \
            uint8_t opType, char key[], uint8_t valueLen, char value[], uint32_t ip_src_addr, uint32_t ip_dst_addr, uint16_t dst_port, struct rte_ether_hdr* eth_client, uint16_t cid) {
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

    eth->s_addr = eth_client->d_addr;
    eth->d_addr = eth_client->s_addr;
    ip->src_addr = ip_src_addr;
    ip->dst_addr = ip_dst_addr;

    if(valueLen == 0){
        ip->total_length = rte_cpu_to_be_16(HEADER_TEMPLATE_SIZE + sizeof(MessageHeader_S) - sizeof(struct rte_ether_hdr));
        udp->dgram_len = rte_cpu_to_be_16(HEADER_TEMPLATE_SIZE + sizeof(MessageHeader_S)
            - sizeof(struct rte_ether_hdr)
            - sizeof(struct rte_ipv4_hdr));
    }

    // printf("ip->src_addr = %x\n", ip->src_addr);
    // printf("ip->dst_addr = %x\n", ip->dst_addr);

    udp->src_port = htons(CLIENT_PORT + rx_queue_id - 1);
    udp->dst_port = htons(dst_port);

    MessageHeader* message_header = (MessageHeader*) ((uint8_t*)eth + HEADER_TEMPLATE_SIZE);
    message_header->opType = opType;
    // message_header->clientID = htons(cid);
    for(int i=0; i<KEY_BYTES; i++){
        message_header->key[i] = key[15-i];     //if affect performance, try delete this big-little transfer
    }
    if(valueLen != 0){
        message_header->valueLen = valueLen;
        for(int i=0; i<VALUE_BYTES; i++){
            message_header->value[i] = value[127-i];
        }
        mbuf->data_len += sizeof(MessageHeader);
        mbuf->pkt_len += sizeof(MessageHeader);
    }   
    else{
        // for(int i=0; i<47; i++){
        //     message_header->padding[i] = 48;    //ascii of "0"
        // }
        mbuf->data_len += sizeof(MessageHeader_S);
        mbuf->pkt_len += sizeof(MessageHeader_S); 
    }
}

int mapping_func(var_obj obj){
    return obj; //��̨backend nodeʱ��ʹ�ã���̨����Ҫ����
}

// KVS: TX loop for test, fixed write rate
static int32_t np_client_txrx_loop(uint32_t lcore_id, uint32_t client_id, uint16_t rx_queue_id) {
    printf("%lld entering TX loop for write on lcore %u, numa_id_of_lcore = %d\n", (long long)time(NULL), lcore_id, rte_socket_id());

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

    uint64_t s_tsc = rte_get_tsc_hz(); 
    uint64_t next_s_tsc = cur_tsc + s_tsc;    
    uint64_t pkts_send_s = 0;

    // mine
    uint32_t cid = client_id;
    int stepFlag = 0;

    struct rte_mbuf *mbuf_burst[NC_MAX_BURST_SIZE];
    struct rte_mbuf *mbuf_rcv;
    int x;
    
    while (1) {
        // read current time
        cur_tsc = rte_rdtsc();
        //usleep(1);

        if (unlikely(cur_tsc > next_ms_tsc)) {
            pkts_send_ms = 0;
            next_ms_tsc += ms_tsc;
        } 

        if (unlikely(cur_tsc > next_s_tsc)) {
            pkts_send_s = 0;
            next_s_tsc += s_tsc;
        } 

        // RX
        for (int i = 0; i < lconf->n_rx_queue; i++) {
            uint32_t nb_rx = rte_eth_rx_burst(lconf->port, lconf->rx_queue_list[i], mbuf_burst, NC_MAX_BURST_SIZE);
            tput_stat[lcore_id].rx += nb_rx;
            for (int j = 0; j < nb_rx; j++) {
            
                mbuf_rcv = mbuf_burst[j];
                //rte_prefetch0(rte_pktmbuf_mtod(mbuf_rcv, void *)); 
                struct rte_ether_hdr* eth = rte_pktmbuf_mtod(mbuf_rcv, struct rte_ether_hdr *);
                struct rte_ipv4_hdr *ip = (struct rte_ipv4_hdr *)((uint8_t*) eth + sizeof(struct rte_ether_hdr));
                struct rte_udp_hdr *udp = (struct rte_udp_hdr *)((uint8_t*) ip + sizeof(struct rte_ipv4_hdr));
                uint32_t srcIP = ip->src_addr;
                uint32_t dstIP = ip->dst_addr;
                // char str_srcIP[INET_ADDRSTRLEN];
                // char *ptr = inet_ntop(AF_INET,&foo.sin_addr, str, sizeof(str)); 

                if(ntohs(eth->ether_type) != ether_type_IPV4){
                    CtrlHeader_fpga* message_header_ctrl = (CtrlHeader_fpga*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE_CTRL);
                    printf("---------------data rcv ERROR!!!!!!: error ether type = %x, op = %d!---------------\n", ntohs(eth->ether_type), message_header_ctrl->opType);
                    rte_pktmbuf_free(mbuf_rcv);
                    continue;
                    //return 1;                    
                }

                // printf("rx_queue_id = %d, dst_port = %d, src_port = %d\n",lconf->rx_queue_list[i], ntohs(udp->dst_port), ntohs(udp->src_port));

                // parse NetLock header,could be changed
                MessageHeader* message_header = (MessageHeader*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE);
                uint8_t opt = (uint8_t) message_header->opType;
                unsigned char key[KEY_BYTES];
                for(int i=0; i<KEY_BYTES; i++){
                    key[i] = message_header->key[KEY_BYTES-1-i];
                }
                uint8_t valueLen = 0;
                unsigned char value[VALUE_BYTES];
                if(opt == OP_PUT_SWITCHHIT || opt == OP_PUT_SWITCHMISS){
                    valueLen = (uint8_t) message_header->valueLen;
                    for(int i=0; i<VALUE_BYTES; i++){
                        value[i] = message_header->value[127-i];
                    }
                }

                // uint32_t key_id = BKDRHash(key, KEY_BYTES);
                uint32_t key_id = (((uint32_t)key[3]) << 24) | (((uint32_t)key[2]) << 16) | (((uint32_t)key[1]) << 8) | (uint32_t)key[0];
                uint16_t cid = 0;//ntohs(message_header->clientID);
                // printf("rcv_cid = %d, rcv_key = %d\n", cid, key_id);
                
                
                if(key_id >= MAX_SERVICE_SIZE){
                    printf("ERROR!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
                    rte_pktmbuf_free(mbuf_rcv);
                    continue;
                }
                // printf("key[3]=%u, key[2]=%u, key[1]=%u, key[0]=%u, key_id = %u\n", (uint32_t)(key[3]), (uint32_t)(key[2]), (uint32_t)(key[1]), (uint32_t)(key[0]), key_id);
                uint32_t lock_id = key_id;
                
                // if (pkts_send_ms < wpkts_send_limit_ms){// || cid > 20000) {
                //     pkts_send_ms++;
                if (pkts_send_s < wpkts_send_limit_s){
                    pkts_send_s++;
                    //CTRL!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!: count lock, manage topk hot
                    service_statistic_update(lock_id, rx_queue_id - 1, mapping_func);
                    // if(lock_id == 79) printf("obj = %d, src_port=%d, dst_port = %d, rx_queue_id = %d, opt=%d\n", lock_id, ntohs(udp->src_port), ntohs(udp->dst_port), rx_queue_id, opt);

                    //KVS OP_GET
                    if (opt == OP_GET || opt == OP_GET_RESEND) {  
                        // if(valueLenTable[lock_id] == 0 || strncmp(keyTable[lock_id], key, KEY_BYTES) != 0){
                        if(valueLenTable[lock_id] == 0){
                            mbuf = rte_pktmbuf_alloc(pktmbuf_pool);
                            generate_write_request_pkt_server(rx_queue_id, header_template_local, lcore_id, mbuf, OP_GETFAIL, key, 0, value, dstIP, srcIP, ntohs(udp->src_port), eth, cid);
                            enqueue_pkt_with_thres(lcore_id, mbuf, NC_MAX_BURST_SIZE, 0);     
                            tput_stat[lcore_id].tx_resend += 1;                        
                        }
                        else{
                            mbuf = rte_pktmbuf_alloc(pktmbuf_pool);
                            generate_write_request_pkt_server(rx_queue_id, header_template_local, lcore_id, mbuf, OP_GETSUCC, key, valueLenTable[lock_id], valueTable[lock_id], dstIP, srcIP, ntohs(udp->src_port), eth, cid);
                            enqueue_pkt_with_thres(lcore_id, mbuf, NC_MAX_BURST_SIZE, 0);                        
                            tput_stat[lcore_id].tx_normal += 1;
                        }    
                    }     
                    //KVS OP_DEL           
                    else if (opt == OP_DEL) {  
                        // if(service_object_state_get(lock_id) == 1 || valueLenTable[lock_id] == 0 || strncmp(keyTable[lock_id], key, KEY_BYTES) != 0){
                        if(service_object_state_get(lock_id) == 1 || valueLenTable[lock_id] == 0){
                            mbuf = rte_pktmbuf_alloc(pktmbuf_pool);
                            generate_write_request_pkt_server(rx_queue_id, header_template_local, lcore_id, mbuf, OP_DELFAIL, key, 0, value, dstIP, srcIP, ntohs(udp->src_port), eth, cid);
                            enqueue_pkt_with_thres(lcore_id, mbuf, NC_MAX_BURST_SIZE, 0);     
                            tput_stat[lcore_id].tx_resend += 1;                            
                        }  
                        else{
                            mbuf = rte_pktmbuf_alloc(pktmbuf_pool);
                            generate_write_request_pkt_server(rx_queue_id, header_template_local, lcore_id, mbuf, OP_DELSUCC, key, 0, value, dstIP, srcIP, ntohs(udp->src_port), eth, cid);
                            enqueue_pkt_with_thres(lcore_id, mbuf, NC_MAX_BURST_SIZE, 0);   
                            valueLenTable[lock_id] = 0;  
                            tput_stat[lcore_id].tx_normal += 1;
                        }                  
                    }
                    //KVS OP_PUT
                    else if (opt == OP_PUT_SWITCHHIT || opt == OP_PUT_SWITCHMISS) {  
                        // if(service_object_state_get(lock_id) == 1 || valueLenTable[lock_id] != 0 && strncmp(keyTable[lock_id], key, KEY_BYTES) != 0){
                        if(service_object_state_get(lock_id) == 1 || valueLenTable[lock_id] != 0){
                            mbuf = rte_pktmbuf_alloc(pktmbuf_pool);
                            generate_write_request_pkt_server(rx_queue_id, header_template_local, lcore_id, mbuf, OP_PUTFAIL, key, 0, value, dstIP, srcIP, ntohs(udp->src_port), eth, cid);
                            enqueue_pkt_with_thres(lcore_id, mbuf, NC_MAX_BURST_SIZE, 0);     
                            tput_stat[lcore_id].tx_resend += 1;                            
                        }         
                        else if(opt == OP_PUT_SWITCHHIT){
                            mbuf = rte_pktmbuf_alloc(pktmbuf_pool);
                            generate_write_request_pkt_server(rx_queue_id, header_template_local, lcore_id, mbuf, OP_PUTSUCC_SWITCHHIT, key, valueLen, value, dstIP, srcIP, ntohs(udp->src_port), eth, cid);
                            enqueue_pkt_with_thres(lcore_id, mbuf, NC_MAX_BURST_SIZE, 0);     
                            valueLenTable[lock_id] = valueLen;
                            memcpy(valueTable[lock_id], value, VALUE_BYTES);
                            if(valueLenTable[lock_id] == 0){
                                memcpy(keyTable[lock_id], key, KEY_BYTES);
                            }
                            tput_stat[lcore_id].tx_normal += 1;
                        }   
                        else if(opt == OP_PUT_SWITCHMISS){
                            mbuf = rte_pktmbuf_alloc(pktmbuf_pool);
                            generate_write_request_pkt_server(rx_queue_id, header_template_local, lcore_id, mbuf, OP_PUTSUCC_SWITCHMISS, key, 0, value, dstIP, srcIP, ntohs(udp->src_port), eth, cid);
                            enqueue_pkt_with_thres(lcore_id, mbuf, NC_MAX_BURST_SIZE, 0);     
                            valueLenTable[lock_id] = valueLen;
                            memcpy(valueTable[lock_id], value, VALUE_BYTES);
                            if(valueLenTable[lock_id] == 0){
                                memcpy(keyTable[lock_id], key, KEY_BYTES);
                            }
                            tput_stat[lcore_id].tx_normal += 1;
                        }         
                    }
                }

                rte_pktmbuf_free(mbuf_rcv);
            }

            enqueue_pkt_with_thres(lcore_id, mbuf, 1, 1);
        }

 
    }
    return 0;
}

//--------------------------------------------


static int32_t np_client_loop(__attribute__((unused)) void *arg) {
    uint32_t lcore_id = rte_lcore_id();
    if ((lcore_id < 1)) {
        struct rte_ether_addr src_addr = { 
            .addr_bytes = MAC_LOCAL}; 
        int ifvalue = 1;
        struct rte_mempool* pktmbuf_pool_tmp = pktmbuf_pool_ctrl;
        np_client_statistic_loop(lcore_id, 0, src_addr, ifvalue, pktmbuf_pool_tmp, n_lcores - 1);
    }
    else {
        np_client_txrx_loop(lcore_id, lcore_id, lcore_id_2_rx_queue_id[lcore_id]);
    }
    return 0; 
}


int main(int argc, char **argv) {
    int ret;
    ret = rte_eal_init(argc, argv);
    if (ret < 0) {
        rte_exit(EXIT_FAILURE, "invalid EAL arguments\n");
    }
    argc -= ret;
    argv += ret;
    nc_init();
    statistic_ini();

    for(int i=0; i<MAX_SERVICE_SIZE; i++){
        valueLenTable[i] = 128; 
    }

    int numa_id_of_port = rte_eth_dev_socket_id(0);
    printf("numa_id_of_port = %d\n", numa_id_of_port);
    printf("im here !");

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
