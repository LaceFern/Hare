#define STATS_LOCK_STAT 0
#define STATS_GET_HOT 1
#define STATS_HOT_REPORT 2
#define STATS_WAIT 3 
#define STATS_READY 4
#define STATS_REPLACE_SUCCESS 5

#define OBJSTATE_IDLE 0
#define OBJSTATE_OCCU 1
#define OBJSTATE_WAIT 2
#define OBJSTATE_READY 3 

#define HEADER_TEMPLATE_SIZE_CTRL    14

#define OP_GET_COLD_COUNTER   8		//FPGA->switch：获取冷锁精??计数
#define OP_CLR_COLD_COUNTER   9		//FPGA->switch：数??面精??计数清零
#define OP_LOCK_COLD_QUEUE    10		//FPGA->switch：通知交换机锁住??????替换的冷锁队??
#define OP_UNLOCK_COLD_QUEUE  11		//（交换机专用??
#define OP_LOCK_STAT 		  12		//FPGA->switch：锁住???数（精??和CMS??
#define OP_UNLOCK_STAT 		  13		//FPGA->switch：解锁???数（精??和CMS??
#define OP_HOT_REPORT 	      14		//FPGA->switch：通知交换机进行热锁替??
#define OP_REPLACE_SUCCESS 	  15		//switch->FPGA：交换机替换完成后返回响??
#define OP_CALLBACK 	      17		//switch->FPGA：通知FPGA交换机上的冷锁队列已??
#define OP_GET_HOT 			  18
#define OP_LOCK_HOT_QUEUE 	  19
#define OP_WAIT_QUEUE_TIMEOUT 20
#define OP_HOT_REPORT_FPGA    21
#define OP_HOT_REPORT_END_FPGA    22


//-----------------------developer-related-para starts-------------------
// #define IP_CTRL               "10.0.0.1"
// #define MAC_CTRL               {0xb8, 0x59, 0x9f, 0xe9, 0x6b, 0x1c} //amax 1 
#define IP_CTRL               "10.0.0.233"
#define MAC_CTRL               {0x11, 0x22, 0x33, 0x44, 0x55, 0x66} //amax 2

#define MAX_SERVICE_SIZE            10000000 
#define MAX_CACHE_SIZE  60000
#define TOPK 8
#define FETCHM 8
#define NUM_SERVICE_CORE 23

#define NUM_BACKENDNODE 2
char* ip_backendNode_arr[NUM_BACKENDNODE] = {
    "10.0.0.7",
    "10.0.0.8"
};
struct rte_ether_addr mac_backendNode_arr[NUM_BACKENDNODE] = {
    {.addr_bytes = {0x98, 0x03, 0x9b, 0xca, 0x40, 0x18}},
    {.addr_bytes = {0x0c, 0x42, 0xa1, 0x2b, 0x0d, 0x70}}
};

//-----------------------developer-related-para ends-------------------

uint16_t ether_type_fpga = 0x2332;
uint16_t ether_type_response = 0x2330;
uint16_t ether_type_IPV4 = 0x0800;
uint32_t ether_type_switch = 0x2333;
uint32_t ether_type_switch_s = 0x2334;

typedef uint32_t var_obj;

typedef struct  CtrlHeader_switch_ {
    uint8_t      opType;
    uint32_t     objIdx;
    uint32_t     counter[TOPK];
    // uint8_t      ack;
    // uint8_t      padding[12];
} __attribute__((__packed__)) CtrlHeader_switch;

//obj和objidx是两种东西，例如两个backend node的时候key不能直接当作其在service中的索引

typedef struct  CtrlHeader_fpga_ {
    uint8_t     opType;
    uint32_t    replaceNum;
    var_obj    obj[TOPK];
    // uint8_t     padding[13];
} __attribute__((__packed__)) CtrlHeader_fpga;

typedef struct  CtrlHeader_fpga_s_ {
    uint8_t     opType;
    uint8_t     replaceNum; 
    var_obj    obj[TOPK];
    uint32_t    counter[TOPK];
} __attribute__((__packed__)) CtrlHeader_fpga_big;

typedef struct  UpdHeader_ {
    uint8_t     opType;
    uint8_t     replaceNum;
    uint32_t    objIdx[TOPK];
    uint32_t    obj[TOPK];
} __attribute__((__packed__)) UpdHeader;

typedef struct  heapNode_ {
    var_obj    obj;
    uint32_t    counter;
} heapNode;

// init header template for ctrl pkt
static void forService_init_header_template_local_ctrl(uint8_t header_template_local[HEADER_TEMPLATE_SIZE_CTRL], struct rte_ether_addr src_addr) {
    memset(header_template_local, 0, HEADER_TEMPLATE_SIZE_CTRL);
    struct rte_ether_hdr *eth = (struct rte_ether_hdr *)header_template_local;
    eth->ether_type = htons(ether_type_fpga);
    struct rte_ether_addr dst_addr = {
        .addr_bytes = MAC_CTRL}; 
    rte_ether_addr_copy(&src_addr, &eth->s_addr);
    rte_ether_addr_copy(&dst_addr, &eth->d_addr);
}
 
static void forCtrlNode_init_header_template_local_ctrl(uint8_t header_template_local[HEADER_TEMPLATE_SIZE_CTRL]) {
    memset(header_template_local, 0, HEADER_TEMPLATE_SIZE_CTRL);
    struct rte_ether_hdr *eth = (struct rte_ether_hdr *)header_template_local;
    eth->ether_type = rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4);
    struct rte_ether_addr src_addr = {
         .addr_bytes = MAC_CTRL}; 
    struct rte_ether_addr dst_addr = {
        .addr_bytes = MAC_CTRL};
    rte_ether_addr_copy(&src_addr, &eth->s_addr);
    rte_ether_addr_copy(&dst_addr, &eth->d_addr);
}


void minheap_downAdjust(heapNode heap[], int low, int high){
    int i = low, j = i * 2;
    while(j <= high){
        if(j+1 <= high && heap[j+1].counter < heap[j].counter){ //TOPKCOLD的话小于号需要改成大于号
            j = j + 1;
        }
        if(heap[j].counter < heap[i].counter){  //TOPKCOLD的话需要改成大于号
            heapNode temp = heap[i];
            heap[i] = heap[j];
            heap[j] = temp;
            i = j;
            j = i * 2;
        }
        else{
            break;
        }  
    }
}

uint32_t hotCounter[2][MAX_SERVICE_SIZE] = {0};
heapNode TopKNode[2][NUM_SERVICE_CORE][TOPK + 1];
heapNode merge_TopKNode[NUM_SERVICE_CORE * TOPK];
uint32_t backendNode_stats_stage = STATS_LOCK_STAT;
volatile bool lock_stat_flag = 0;
volatile bool clear_flag = 0;
volatile bool clear_TOPK_flag = 0;
volatile bool check_queue_lock_flag = 0;
volatile uint8_t obj_states[MAX_SERVICE_SIZE] = {0};
uint32_t hotNum = 0;
uint32_t last_hotNum = 0;
uint32_t hotID[TOPK] = {0};
uint32_t last_hotID[TOPK] = {0};

volatile uint32_t TopKNode_state[NUM_SERVICE_CORE];

void statistic_ini(){
    for(int i=0; i<TOPK; i++){
        hotID[i] = 4294967295;
        last_hotID[i] = 4294967295;
    }
    for(int i=0; i<=1; i++){
        for(int j=0; j<NUM_SERVICE_CORE; j++){
            for(int k=0; k<=TOPK; k++){
                TopKNode[i][j][k].obj = 4294967295;
                TopKNode[i][j][k].counter = 0;
            }
        }
    }
    for(int i=0; i<MAX_SERVICE_SIZE; i++){
        obj_states[i] = OBJSTATE_IDLE;
    }
}

//Descending
int inc(const void *a, const void *b)
{
	return ( * (heapNode * )b).counter > ( * (heapNode * )a).counter ? 1 : -1;
}

// generate write request packet for server
static void forService_generate_ctrl_pkt(uint16_t rx_queue_id, uint8_t header_template[HEADER_TEMPLATE_SIZE_CTRL], uint32_t lcore_id, struct rte_mbuf *mbuf, \
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
    eth->ether_type = htons(ether_type);;
    mbuf->data_len += HEADER_TEMPLATE_SIZE_CTRL;
    mbuf->pkt_len += HEADER_TEMPLATE_SIZE_CTRL;

    mbuf->data_len += sizeof(CtrlHeader_fpga);
    mbuf->pkt_len += sizeof(CtrlHeader_fpga);
    CtrlHeader_fpga* message_header = (CtrlHeader_fpga*) ((uint8_t*)eth + HEADER_TEMPLATE_SIZE_CTRL);
    message_header->opType = opType;
}

// generate write request packet for server
static void forService_generate_ctrl_pkt_hotReport(uint16_t rx_queue_id, uint8_t header_template[HEADER_TEMPLATE_SIZE_CTRL], uint32_t lcore_id, struct rte_mbuf *mbuf, \
            uint16_t ether_type, uint8_t opType, heapNode TopKhotNode[]) {
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

    mbuf->data_len += sizeof(CtrlHeader_fpga_big);
    mbuf->pkt_len += sizeof(CtrlHeader_fpga_big);
    CtrlHeader_fpga_big* message_header = (CtrlHeader_fpga_big*) ((uint8_t*)eth + HEADER_TEMPLATE_SIZE_CTRL);
    message_header->opType = opType;
    message_header->replaceNum = 0;
    for(int i=0; i<TOPK; i++){
        message_header->obj[i] = htonl(TopKhotNode[i].obj);
        if(message_header->obj[i] == 4294967295){
            message_header->counter[i] = 0;
        }
        else{
            message_header->counter[i] = htonl(TopKhotNode[i].counter);
        }
    }
}

void service_statistic_update(var_obj obj, uint16_t process_id, int (*mapping_func)(var_obj)){
    int obj_idx = (*mapping_func)(obj);
    // printf("obj_idx = %d\n", obj_idx);

    if(!lock_stat_flag){
        //DEBUG! 4500000 NOT COUNT
        if(obj != 4500000){
            if(clear_flag){
                hotCounter[1][obj_idx] ++;
            }
            else{
                hotCounter[0][obj_idx] ++;
            }
        }
    }
    else{

        if(TopKNode_state[process_id] == 0){
            TopKNode_state[process_id] = 1;

            heapNode tempNode;
            tempNode.obj = obj;
            if(clear_flag){
                tempNode.counter = hotCounter[1][obj_idx];   
            }
            else{
                tempNode.counter = hotCounter[0][obj_idx]; 
            }
            // printf("obj = %d, counter = %d, process_id=%d, clear_TOPK_flag=%d\n", tempNode.obj, tempNode.counter, process_id, clear_TOPK_flag);
            bool jump_flag = 0;
            if(clear_TOPK_flag){
                for(int k=1; k<=TOPK; k++){
                    if(TopKNode[1][process_id][k].obj == tempNode.obj){
                        jump_flag = 1;
                        break;
                    }
                }
                for(int m=0; m<last_hotNum; m++){
                    if(last_hotID[m] == tempNode.obj){
                        jump_flag = 1;
                        break;                                
                    }
                }
                if(!jump_flag && tempNode.counter > TopKNode[1][process_id][1].counter){ //TOPKCOLD??????????????С???
                    TopKNode[1][process_id][1] = tempNode;
                    minheap_downAdjust(TopKNode[1][process_id], 1, TOPK);
                    // printf("minheap_downAdjust(TopKNode[1][%d], 1, TOPK)\n", process_id);
                }
            }
            else{
                for(int k=1; k<=TOPK; k++){
                    if(TopKNode[0][process_id][k].obj == tempNode.obj){
                        jump_flag = 1;
                        break;
                    }
                }
                for(int m=0; m<last_hotNum; m++){
                    if(last_hotID[m] == tempNode.obj){
                        jump_flag = 1;
                        break;                                
                    }
                }
                // printf("jump_flag=%d, tempNode.counter=%d, TopKNode[0][%d][1].counter=%d\n", jump_flag, tempNode.counter, process_id, TopKNode[0][process_id][1].counter);
                if(!jump_flag && tempNode.counter > TopKNode[0][process_id][1].counter){ //TOPKCOLD??????????????С???
                    TopKNode[0][process_id][1] = tempNode;
                    minheap_downAdjust(TopKNode[0][process_id], 1, TOPK);
                    // printf("minheap_downAdjust(TopKNode[0][%d], 1, TOPK)\n", process_id);
                }
            }

            if(TopKNode_state[process_id] == 2){
                TopKNode_state[process_id] = 3;
            }
        }
        if(TopKNode_state[process_id] == 2){
            TopKNode_state[process_id] = 3;
        }
        // printf("stats: TopKNode_state[%d] = %d\n", process_id, TopKNode_state[process_id]);
    }
}


// service_object_state
uint8_t service_object_state_get(int id){
    return obj_states[id];
}
void service_object_state_set(int id, uint8_t obj_state){
    obj_states[id] = obj_state;  
}

//--------------------------------------------------------
static int32_t np_client_statistic_loop(uint32_t lcore_id, uint16_t rx_queue_id, struct rte_ether_addr src_addr, int ifvalue, struct rte_mempool * pktmbuf_pool_ctrl, int statistic_lcore_num) {
    printf("%lld entering RX loop (master loop) on lcore %u, numa_id_of_lcore = %d\n", (long long)time(NULL), lcore_id, rte_socket_id());
    // initiate header_template_local_ctrl
    uint8_t header_template_local_ctrl[HEADER_TEMPLATE_SIZE_CTRL];
    forService_init_header_template_local_ctrl(header_template_local_ctrl, src_addr);
    // initiate lconf
    struct lcore_configuration *lconf = &lcore_conf[lcore_id];
    // memory related points
    struct rte_mbuf *mbuf;
    struct rte_mbuf *mbuf_burst[NC_MAX_BURST_SIZE];
    struct rte_mbuf *mbuf_rcv;
    // time related configurations
    uint64_t cur_tsc = rte_rdtsc();
    uint64_t ms_tsc = rte_get_tsc_hz() / 1000;
    uint64_t next_ms_tsc = cur_tsc + ms_tsc;
    uint64_t update_tsc = rte_get_tsc_hz(); // in second
    uint64_t next_update_tsc = cur_tsc + update_tsc;
    // stop flags
    uint32_t pkts_send_ms = 0;
    int stop_statis = 0;

    int flag = 0;
    int temp_idx = 0;
 
    while (1) {
        // sleep(1);
        // mbuf = rte_pktmbuf_alloc(pktmbuf_pool_ctrl);
        // forService_generate_ctrl_pkt(rx_queue_id, header_template_local_ctrl, lcore_id, mbuf, ether_type_switch, OP_CALLBACK);
        // enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0); 
        // printf("826: send OP_CALLBACK\n");

        for (int i = 0; i < lconf->n_rx_queue; i++) {
            uint32_t nb_rx = rte_eth_rx_burst(lconf->port, lconf->rx_queue_list[i], mbuf_burst, NC_MAX_BURST_SIZE);
            for (int j = 0; j < nb_rx; j++) {
                mbuf_rcv = mbuf_burst[j];
                //rte_prefetch0(rte_pktmbuf_mtod(mbuf_rcv, void *));     
                struct rte_ether_hdr* eth = rte_pktmbuf_mtod(mbuf_rcv, struct rte_ether_hdr *);
                // printf("829: stage = %d, ether_type = %d\n", backendNode_stats_stage, ntohs(eth->ether_type));
                if(ntohs(eth->ether_type) == ether_type_switch){
                    CtrlHeader_switch* message_header = (CtrlHeader_fpga*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE_CTRL);
                    if(message_header->opType == OP_LOCK_HOT_QUEUE){
                        lock_stat_flag = 1;
                        //lock stat response
                        mbuf = rte_pktmbuf_alloc(pktmbuf_pool_ctrl);
                        forService_generate_ctrl_pkt(rx_queue_id, header_template_local_ctrl, lcore_id, mbuf, ether_type_switch, OP_CALLBACK);
                        enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                        // printf("826: send OP_CALLBACK\n");
                    }
                    else if(message_header->opType == OP_REPLACE_SUCCESS){
                        // printf("826: recv OP_REPLACE_SUCCESS\n");
                    }
                }
                else{
                    // struct rte_ipv4_hdr *ip = (struct rte_ipv4_hdr *)((uint8_t*) eth + sizeof(struct rte_ether_hdr));
                    // struct rte_udp_hdr *udp = (struct rte_udp_hdr *)((uint8_t*) ip + sizeof(struct rte_ipv4_hdr));
                    // printf("---------------ERROR!!!!!!: error ether type = %x, udp_port = %d!---------------\n", ntohs(eth->ether_type), ntohs(udp->dst_port));
                    struct rte_ipv4_hdr *ip = (struct rte_ipv4_hdr *)((uint8_t*) eth + sizeof(struct rte_ether_hdr));
                    struct rte_udp_hdr *udp = (struct rte_udp_hdr *)((uint8_t*) ip + sizeof(struct rte_ipv4_hdr));
                    MessageHeader* message_header = (MessageHeader*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE);
                    unsigned char tmp_key[KEY_BYTES];
                    for(int i=0; i<KEY_BYTES; i++){
                        tmp_key[i] = message_header->key[KEY_BYTES-1-i];
                    }
                    uint32_t tmp_key_id = (((uint32_t)tmp_key[3]) << 24) | (((uint32_t)tmp_key[2]) << 16) | (((uint32_t)tmp_key[1]) << 8) | (uint32_t)tmp_key[0];
                    uint8_t tmp_opt = (uint8_t) message_header->opType;
                    char tmp_src_ip[16];
                    char tmp_dst_ip[16];
                    inet_ntop(AF_INET, &(ip->src_addr), tmp_src_ip, sizeof(tmp_src_ip));
                    inet_ntop(AF_INET, &(ip->dst_addr), tmp_dst_ip, sizeof(tmp_dst_ip));
                    printf("---------------ERROR!!!!!!: error ether type = %x, udp_port = %d!---------------\n", ntohs(eth->ether_type), ntohs(udp->dst_port));
                    printf("src_ip = %s, dst_ip = %s, op = %d, key = %d\n", tmp_src_ip, tmp_dst_ip, tmp_opt, tmp_key_id);
                    //return 1;
                }
                rte_pktmbuf_free(mbuf_rcv);
            }
        }

        // print stats at master lcore 
        cur_tsc = rte_rdtsc();
        if ((lcore_id == 0) && (cur_tsc > next_update_tsc)) {        
                print_per_core_throughput();
                next_update_tsc = cur_tsc + update_tsc;
        } 

    }
    return 0;
}  


//----------------------------------------------------------------
//----------------------------------------------------------------
//----------------------------------------------------------------
//----------------------------------------------------------------
//----------------------------------------------------------------
struct rte_ether_addr mac_zero = {.addr_bytes = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
typedef struct hotobj{
    var_obj obj;
    uint32_t hot_count;
    uint32_t obj_idx;
}struct_hotobj;
typedef struct coldobj{
    uint32_t obj_idx;
    uint32_t cold_count;
    uint32_t empty_flag;
}struct_coldobj;

#define STATS_COLLECT 0
#define STATS_ANALYZE 1
#define STATS_UPDATE 2
#define STATS_CLEAN 3
#define STATS_WAIT_MS 100
#define END_S 200

#define OP_LOCK_STAT_SWITCH 0x0C
#define OP_LOCK_STAT_FPGA 12
#define OP_GET_COLD_COUNTER_SWITCH 0x08
#define OP_GET_HOT_COUNTER_FPGA 18
#define OP_QUEUE_EMPTY_SWITCH 0x11
#define OP_QUEUE_EMPTY_FPGA 0x11
#define OP_HOT_REPORT_SWITCH 0x0E
#define OP_LOCK_COLD_QUEUE_SWITCH 0x0A
#define OP_LOCK_HOT_QUEUE_FPGA 19 
#define OP_CLN_COLD_COUNTER_SWITCH 0x09
#define OP_CLN_HOT_COUNTER_FPGA 9
#define OP_UNLOCK_STAT_SWITCH 0x0D
#define OP_UNLOCK_STAT_FPGA 13
#define OP_UNLOCK_HOT_QUEUE_FPGA 15
// #define OP_REPLACE_SUCCESS 0x0F
// #define OP_CALLBACK 0x11

#define WINSIZE 128

uint32_t *objstats;
// struct_hotobj tmp_topk_hot_obj[NUM_BACKENDNODE][TOPK];
struct_hotobj topk_hot_obj[TOPK];
struct_hotobj topk_hot_obj_upd[TOPK];
struct_coldobj topk_cold_obj[TOPK];
struct_coldobj topk_cold_obj_heap[TOPK + 1];
struct_coldobj topk_cold_obj_heap_tmp[32][TOPK + 1];
struct_coldobj topk_cold_obj_upd[TOPK];
int pkt_count_4findCold[32] = {0};
uint32_t tmp_topk_hot_obj_num[NUM_BACKENDNODE];
uint32_t topk_hot_obj_num;
uint32_t topk_cold_obj_num;
uint32_t topk_upd_obj_num;
volatile uint32_t ctrlNode_stats_stage = STATS_COLLECT;//STATS_COLLECT;//STATS_COLLECT;//STATS_CLEAN;
uint32_t find_topk_hot_ready_flag;
volatile uint32_t find_topk_cold_ready_flag[32];
volatile uint32_t p4_clean_flag = 1;
uint32_t runEnd_flag = 0;
volatile uint32_t epoch_idx = 0;
volatile int slidingWin[8][4096] = {0};
volatile int pktSendCount[32] = {0};
volatile int pktRecvCount[32] = {0};
volatile int minheap_downAdjustCount[32] = {0};

typedef struct mine_ctrlPara{
    uint32_t waitTime;
    uint32_t cacheSize;
    uint32_t globalFlag;
    uint32_t updEpochs;
    uint32_t cacheSetSize;
}struct_ctrlPara;

struct_ctrlPara gb_ctrlPara = {
    100,
    32768,
    1,
    64,
    64
};

bool mac_compare(struct rte_ether_addr mac1, struct rte_ether_addr mac2){
    bool flag = 1;
    for(int i = 0; i < RTE_ETHER_ADDR_LEN; i++){
        if(mac1.addr_bytes[i] != mac2.addr_bytes[i]){
            flag = 0;
        }
    }
    return flag;
}

void maxheap_downAdjust(struct_coldobj heap[], int low, int high){
    int i = low, j = i * 2;
    while(j <= high){
        if(j+1 <= high && heap[j+1].cold_count > heap[j].cold_count){ //TOPKCOLD锟侥伙拷小锟节猴拷锟斤拷要锟侥成达拷锟节猴拷
            j = j + 1;
        }
        if(heap[j].cold_count > heap[i].cold_count){  //TOPKCOLD锟侥伙拷锟斤拷要锟侥成达拷锟节猴拷
            struct_coldobj temp = heap[i];
            heap[i] = heap[j];
            heap[j] = temp;
            i = j;
            j = i * 2;
        }
        else{
            break;
        }
    }
}

static void forCtrlNode_generate_ctrl_pkt(uint16_t rx_queue_id, uint8_t header_template[], uint32_t lcore_id, struct rte_mbuf *mbuf, \
            uint16_t ether_type, uint8_t opType, uint32_t objIdx_or_objNum, struct_hotobj hotobj[], struct rte_ether_addr mac_dst_bn) {
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

    eth->ether_type = htons(ether_type);
    if(ether_type == ether_type_fpga){
        rte_ether_addr_copy(&mac_dst_bn, &eth->d_addr);
        mbuf->data_len += sizeof(CtrlHeader_fpga);
        mbuf->pkt_len += sizeof(CtrlHeader_fpga);
        CtrlHeader_fpga* message_header = (CtrlHeader_fpga*) ((uint8_t*)eth + HEADER_TEMPLATE_SIZE_CTRL);
        message_header->opType = opType;
        switch(opType){
            case OP_LOCK_HOT_QUEUE_FPGA:{
                message_header->replaceNum = htonl(objIdx_or_objNum); //=objnum here.
                for(int i = 0; i < objIdx_or_objNum; i++){
                    message_header->obj[i] = htonl(hotobj[i].obj);
                }
                for(int i = objIdx_or_objNum; i < TOPK; i++){
                    message_header->obj[i] = htonl(0xffffffff);
                }
                break;
            }
        }
    }
    else{
        if(opType == OP_HOT_REPORT_SWITCH){ 
            mbuf->data_len += sizeof(UpdHeader);
            mbuf->pkt_len += sizeof(UpdHeader); 
        }
        else{
            mbuf->data_len += sizeof(CtrlHeader_switch);
            mbuf->pkt_len += sizeof(CtrlHeader_switch);
        }

        CtrlHeader_switch* message_header = (CtrlHeader_switch*) ((uint8_t*)eth + HEADER_TEMPLATE_SIZE_CTRL);
        message_header->opType = opType;
        switch(opType){
            case OP_GET_COLD_COUNTER_SWITCH:{
                message_header->objIdx = htonl(objIdx_or_objNum);
                break;
            }
            case OP_LOCK_COLD_QUEUE_SWITCH:{
                message_header->objIdx = htonl(objIdx_or_objNum);
                break;
            }
            case OP_HOT_REPORT_SWITCH:{
                UpdHeader* message_header_s = (UpdHeader*) ((uint8_t*)eth + HEADER_TEMPLATE_SIZE_CTRL);
                message_header_s->replaceNum = objIdx_or_objNum; //=objnum here.
                for(int i = 0; i < objIdx_or_objNum; i++){
                    message_header_s->obj[i] = htonl(hotobj[i].obj);
                    message_header_s->objIdx[i] = htonl(hotobj[i].obj_idx);
                }
                break;
            }
            case OP_CLN_COLD_COUNTER_SWITCH:{
                message_header->objIdx = htonl(objIdx_or_objNum);
                break;
            }
            default:{ 
                message_header->objIdx = htonl(objIdx_or_objNum);
                break;
            }
        }
    }
}

static void forCtrlNode_generate_ctrl_pkt_fetchCold(uint16_t rx_queue_id, uint8_t header_template[], uint32_t lcore_id, struct rte_mbuf *mbuf, \
            uint16_t ether_type, uint8_t opType, uint32_t objIdx_or_objNum, int dst_mac) {
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
  
    eth->ether_type = htons(ether_type);
    struct rte_ether_addr dst_addr = {.addr_bytes = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};
    dst_addr.addr_bytes[5] = dst_mac;
    rte_ether_addr_copy(&dst_addr, &eth->d_addr);
    // printf("829: send: ether_src = %d, ether_dst  = %d, ether_type = %d\n", eth->s_addr, eth->d_addr, ntohs(eth->ether_type));

    mbuf->data_len += sizeof(CtrlHeader_switch);
    mbuf->pkt_len += sizeof(CtrlHeader_switch);

    CtrlHeader_switch* message_header = (CtrlHeader_switch*) ((uint8_t*)eth + HEADER_TEMPLATE_SIZE_CTRL);
    message_header->opType = opType;
    message_header->objIdx = htonl(objIdx_or_objNum);
}

//--------------------------------------------------------------
//--------------------------------------------------------------
//--------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>

//单个记录的结构体
typedef struct {
    var_obj obj;
    int count;
    int backendNode_idx;

}SqNote;
//记录表的结构体
typedef struct {
    SqNote r[1 + NUM_BACKENDNODE * TOPK];
    int length;
}SqList;
//此方法中，存储记录的数组中，下标为 0 的位置时空着的，不放任何记录，记录从下标为 1 处开始依次存放
int Partition(SqList *L,int low,int high){
    L->r[0]=L->r[low];
    int pivotcount=L->r[low].count;
    //直到两指针相遇，程序结束
    while (low<high) {
        //high指针左移，直至遇到比pivotcount值小的记录，指针停止移动
        while (low<high && L->r[high].count>=pivotcount) {
            high--;
        }
        //直接将high指向的小于支点的记录移动到low指针的位置。
        L->r[low]=L->r[high];
        //low 指针右移，直至遇到比pivotcount值大的记录，指针停止移动
        while (low<high && L->r[low].count<=pivotcount) {
            low++;
        }
        //直接将low指向的大于支点的记录移动到high指针的位置
        L->r[high]=L->r[low];
    }
    //将支点添加到准确的位置
    L->r[low]=L->r[0];
    return low;
}
void QSort(SqList *L,int low,int high){
    if (low<high) {
        //找到支点的位置
        int pivotloc=Partition(L, low, high);
        //对支点左侧的子表进行排序
        QSort(L, low, pivotloc-1);
        //对支点右侧的子表进行排序
        QSort(L, pivotloc+1, high);
    }
}
void QuickSort(SqList *L){
    QSort(L, 1,L->length);
}
// int main() {
//     SqList * L=(SqList*)malloc(sizeof(SqList));
//     L->length=8; 
//     L->r[1].count=49;
//     L->r[2].count=38;
//     L->r[3].count=65;
//     L->r[4].count=97;
//     L->r[5].count=76;
//     L->r[6].count=13;
//     L->r[7].count=27;
//     L->r[8].count=49;
//     QuickSort(L);
//     for (int i=1; i<=L->length; i++) {
//         printf("%d ",L->r[i].count);
//     }
//     return 0;
// }

SqList* topk_hot_obj_multiBN;
int topk_upd_obj_num_multiBN[NUM_BACKENDNODE];
struct_hotobj topk_upd_obj_multiBN[NUM_BACKENDNODE][TOPK];
uint32_t * check_arr;