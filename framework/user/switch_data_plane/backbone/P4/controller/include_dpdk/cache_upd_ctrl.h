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

#define OP_GET_COLD_COUNTER   8		//FPGA->switch����ȡ������??����
#define OP_CLR_COLD_COUNTER   9		//FPGA->switch����??�澫??��������
#define OP_LOCK_COLD_QUEUE    10		//FPGA->switch��֪ͨ��������ס??????�滻��������??
#define OP_UNLOCK_COLD_QUEUE  11		//��������ר��??
#define OP_LOCK_STAT 		  12		//FPGA->switch����ס???������??��CMS??
#define OP_UNLOCK_STAT 		  13		//FPGA->switch������???������??��CMS??
#define OP_HOT_REPORT 	      14		//FPGA->switch��֪ͨ����������������??
#define OP_REPLACE_SUCCESS 	  15		//switch->FPGA���������滻��ɺ󷵻���???
#define OP_CALLBACK 	      17		//switch->FPGA��֪ͨFPGA�������ϵ�����������??
#define OP_GET_HOT 			  18
#define OP_LOCK_HOT_QUEUE 	  19
#define OP_WAIT_QUEUE_TIMEOUT 20
#define OP_HOT_REPORT_FPGA    21
#define OP_HOT_REPORT_END_FPGA    22



//-----------------------developer-related-para starts-------------------
#define IP_CTRL               "10.0.0.9"
#define MAC_CTRL               {0x00, 0x90, 0xfb, 0x70, 0x63, 0xc6} //0x0090fb7063c6

#define MAX_SERVICE_SIZE            1000000 
#define MAX_CACHE_SIZE  32768
#define TOPK_TURN 1//10 
#define TOPK 800//800//1000 //8
#define HOTOBJ_IN_ONEPKT 20//20//20 //50 //20
#define SLEEP_US_BIGPKT 5//0 //1000 //0
#define FETCHM 4
#define NUM_SERVICE_CORE 23

// #define NUM_BACKENDNODE 2
// char* ip_backendNode_arr[NUM_BACKENDNODE] = {
//     "10.0.0.7"
//     "10.0.0.8"
// };
// struct rte_ether_addr mac_backendNode_arr[NUM_BACKENDNODE] = {
//     {.addr_bytes = {0x98, 0x03, 0x9b, 0xca, 0x40, 0x18}},
//     {.addr_bytes = {0x0c, 0x42, 0xa1, 0x2b, 0x0d, 0x70}}
// };
#define NUM_BACKENDNODE 1
char* ip_backendNode_arr[NUM_BACKENDNODE] = {
    "10.0.0.7"
};
struct rte_ether_addr mac_backendNode_arr[NUM_BACKENDNODE] = {
    {.addr_bytes = {0x98, 0x03, 0x9b, 0xca, 0x40, 0x18}}
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
    uint32_t     counter[HOTOBJ_IN_ONEPKT];
    uint8_t      ack;
    // uint8_t      padding[12];
} __attribute__((__packed__)) CtrlHeader_switch;

//obj��objidx�����ֶ�������������backend node��ʱ��key����ֱ�ӵ�������service�е�����

typedef struct  CtrlHeader_fpga_ {
    uint8_t     opType;
    uint32_t    replaceNum;
    var_obj    obj[HOTOBJ_IN_ONEPKT];
    // uint8_t     padding[13];
} __attribute__((__packed__)) CtrlHeader_fpga;

typedef struct  CtrlHeader_fpga_userupd_ {
    uint8_t     opType;
    uint32_t    replaceNum;
    var_obj    obj[HOTOBJ_IN_ONEPKT];
    var_obj    objIdx[HOTOBJ_IN_ONEPKT];
    // uint8_t     padding[13];
} __attribute__((__packed__)) CtrlHeader_fpga_userupd;

typedef struct  CtrlHeader_fpga_s_ {
    uint8_t     opType;
    uint8_t     replaceNum; 
    var_obj    obj[HOTOBJ_IN_ONEPKT];
    uint32_t    counter[HOTOBJ_IN_ONEPKT];
} __attribute__((__packed__)) CtrlHeader_fpga_big;

typedef struct  UpdHeader_ {
    uint8_t     opType;
    uint32_t     replaceNum;
    uint32_t    objIdx[HOTOBJ_IN_ONEPKT];
    uint32_t    obj[HOTOBJ_IN_ONEPKT];
} __attribute__((__packed__)) UpdHeader;

typedef struct  heapNode_ {
    var_obj    obj;
    uint32_t    counter;
} heapNode;


static void forCtrlNode_init_header_template_local_ctrl(uint8_t header_template_local[HEADER_TEMPLATE_SIZE_CTRL]) {
    memset(header_template_local, 0, HEADER_TEMPLATE_SIZE_CTRL);
    struct rte_ether_hdr *eth = (struct rte_ether_hdr *)header_template_local;
    eth->ether_type = rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4);
    struct rte_ether_addr src_addr = {
         .addr_bytes = MAC_CTRL}; 
    struct rte_ether_addr dst_addr = {
        .addr_bytes = {0x23, 0x23, 0x23, 0x23, 0x23, 0x23}};
    rte_ether_addr_copy(&src_addr, &eth->s_addr);
    rte_ether_addr_copy(&dst_addr, &eth->d_addr);
}


void minheap_downAdjust(heapNode heap[], int low, int high){
    int i = low, j = i * 2;
    while(j <= high){
        if(j+1 <= high && heap[j+1].counter < heap[j].counter){ //TOPKCOLD�Ļ�С�ں���Ҫ�ĳɴ��ں�
            j = j + 1;
        }
        if(heap[j].counter < heap[i].counter){  //TOPKCOLD�Ļ���Ҫ�ĳɴ��ں�
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
#define END_S 400

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
struct_coldobj topk_cold_obj_heap_tmp[TOPK + 1];
struct_coldobj topk_cold_obj_upd[TOPK];
int pkt_count_4findCold[32] = {0};
uint32_t tmp_topk_hot_obj_num[NUM_BACKENDNODE];
uint32_t topk_hot_obj_num;
uint32_t topk_cold_obj_num;
uint32_t topk_upd_obj_num;
volatile uint32_t ctrlNode_stats_stage = STATS_COLLECT;//STATS_ANALYZE;//STATS_COLLECT;//STATS_COLLECT;//STATS_COLLECT;//STATS_CLEAN;
uint32_t find_topk_hot_ready_flag;
volatile uint32_t find_topk_cold_ready_flag[32];
volatile uint32_t p4_clean_flag = 1;
uint32_t runEnd_flag = 0;
volatile uint32_t epoch_idx = 0;
volatile int slidingWin[8][4096] = {0};
volatile int pktSendCount[32] = {0};
volatile int pktRecvCount[32] = {0};
volatile int minheap_downAdjustCount[32] = {0};

uint32_t max_pkts_from_one_bn = (TOPK - 1) / HOTOBJ_IN_ONEPKT + 1;

typedef struct mine_ctrlPara{
    uint32_t waitTime;
    uint32_t cacheSize;
    uint32_t globalFlag;
    uint32_t updEpochs;
    uint32_t cacheSetSize;
}struct_ctrlPara;

struct_ctrlPara ctrlPara = {
    50,
    MAX_CACHE_SIZE,
    1,
    TOPK_TURN,
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
        if(j+1 <= high && heap[j+1].cold_count > heap[j].cold_count){ //TOPKCOLD�Ļ�С�ں���Ҫ�ĳɴ��ں�
            j = j + 1;
        }
        if(heap[j].cold_count > heap[i].cold_count){  //TOPKCOLD�Ļ���Ҫ�ĳɴ��ں�
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

static void forCtrlNode_generate_ctrl_pkt_mul(uint16_t rx_queue_id, uint8_t header_template[], uint32_t lcore_id, struct rte_mbuf *mbuf, \
            uint16_t ether_type, uint8_t opType, uint32_t objIdx_or_objNum, struct_hotobj hotobj[], struct rte_ether_addr mac_dst_bn, uint32_t hot_obj_num_in_pkt) {
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
        // CtrlHeader_fpga_userupd* message_header = (CtrlHeader_fpga_userupd*) ((uint8_t*)eth + HEADER_TEMPLATE_SIZE_CTRL);
        message_header->opType = opType;
        switch(opType){
            case OP_LOCK_HOT_QUEUE_FPGA:{
                message_header->replaceNum = htonl((hot_obj_num_in_pkt << 16) | (objIdx_or_objNum)); //=objnum here.
                uint32_t bound = hot_obj_num_in_pkt;
                for(int i = 0; i < bound; i++){
                    message_header->obj[i] = htonl(hotobj[i].obj);
                    // message_header->objIdx[i] = htonl(hotobj[i].obj_idx);
                }
                for(int i = bound; i < HOTOBJ_IN_ONEPKT; i++){
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
            case OP_HOT_REPORT_SWITCH:{
                UpdHeader* message_header_s = (UpdHeader*) ((uint8_t*)eth + HEADER_TEMPLATE_SIZE_CTRL);
                message_header_s->replaceNum = htonl((hot_obj_num_in_pkt << 16) | (objIdx_or_objNum));
                for(int i = 0; i < hot_obj_num_in_pkt; i++){
                    message_header_s->obj[i] = htonl(hotobj[i].obj);
                    message_header_s->objIdx[i] = htonl(hotobj[i].obj_idx);
                    // message_header_s->obj[i] = htonl(hotobj[i].obj_idx);
                    // message_header_s->objIdx[i] = htonl(hotobj[i].obj);
                    // printf("check: (obj = %d, obj_idx = %d, pkt_len = %d)\n", hotobj[i].obj, hotobj[i].obj_idx, mbuf->pkt_len);
                }
                break;
            }
            default:{ 
                message_header->objIdx = htonl(objIdx_or_objNum);
                break;
            }
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
                uint32_t bound = (objIdx_or_objNum == 0xffffffff) ? 0 : objIdx_or_objNum;
                for(int i = 0; i < bound; i++){
                    message_header->obj[i] = htonl(hotobj[i].obj);
                }
                for(int i = bound; i < HOTOBJ_IN_ONEPKT; i++){
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

//������¼�Ľṹ��
typedef struct {
    var_obj obj;
    int count;
    int backendNode_idx;

}SqNote;
//��¼���Ľṹ��
typedef struct {
    SqNote r[1 + NUM_BACKENDNODE * TOPK];
    int length;
}SqList;
//�˷����У��洢��¼�������У��±�Ϊ 0 ��λ��ʱ���ŵģ������κμ�¼����¼���±�Ϊ 1 ����ʼ���δ��?
int Partition(SqList *L,int low,int high){
    L->r[0]=L->r[low];
    int pivotcount=L->r[low].count;
    //ֱ����ָ���������������?
    while (low<high) {
        //highָ�����ƣ�ֱ��������pivotcountֵС�ļ�¼��ָ��ֹͣ�ƶ�
        while (low<high && L->r[high].count>=pivotcount) {
            high--;
        }
        //ֱ�ӽ�highָ���С��֧��ļ�¼�ƶ���lowָ���λ�á�?
        L->r[low]=L->r[high];
        //low ָ�����ƣ�ֱ��������pivotcountֵ��ļ�¼��ָ��ֹͣ�ƶ�?
        while (low<high && L->r[low].count<=pivotcount) {
            low++;
        }
        //ֱ�ӽ�lowָ��Ĵ���֧��ļ�¼�ƶ���highָ���λ��?
        L->r[high]=L->r[low];
    }
    //��֧�����ӵ�׼ȷ��λ��
    L->r[low]=L->r[0];
    return low;
}
void QSort(SqList *L,int low,int high){
    if (low<high) {
        //�ҵ�֧���λ��?
        int pivotloc=Partition(L, low, high);
        //��֧�������ӱ���������
        QSort(L, low, pivotloc-1);
        //��֧���Ҳ���ӱ���������?
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


void rte_sleep_us(uint64_t sleep_us){
    uint64_t sleep_tsc = sleep_us * rte_get_tsc_hz() / 1000000;
    uint64_t start_tsc = rte_rdtsc();
    uint64_t finish_tsc = start_tsc + sleep_tsc;
    uint64_t current_tsc = start_tsc;
    while(current_tsc < finish_tsc){
        current_tsc = rte_rdtsc();
    }
}



static void send_topk_cold_loop(uint32_t lcore_id, uint16_t rx_queue_id, struct_ctrlPara ctrlPara, int start_index, int end_index){
    struct lcore_configuration *lconf = &lcore_conf[lcore_id];
    struct rte_mbuf *mbuf_burst[NC_MAX_BURST_SIZE];
    struct rte_mbuf *mbuf_rcv;
    uint8_t header_template_local[HEADER_TEMPLATE_SIZE_CTRL];
    forCtrlNode_init_header_template_local_ctrl(header_template_local);
    struct rte_mbuf *mbuf;

    for(int i = start_index; i <= end_index; i++){
        mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]); 
        forCtrlNode_generate_ctrl_pkt_fetchCold(rx_queue_id, header_template_local, lcore_id, mbuf, ether_type_switch, OP_GET_COLD_COUNTER_SWITCH, i, 0);
        enqueue_pkt_with_thres(lcore_id, mbuf, 32, 0);
    }
    enqueue_pkt_with_thres(lcore_id, mbuf, 0, 1);
}

static void recv_topk_cold_loop(uint32_t lcore_id, uint16_t rx_queue_id, struct_ctrlPara ctrlPara, int start_index, int end_index){
    struct lcore_configuration *lconf = &lcore_conf[lcore_id];
    struct rte_mbuf *mbuf_burst[NC_MAX_BURST_SIZE];
    struct rte_mbuf *mbuf_rcv;
    uint8_t header_template_local[HEADER_TEMPLATE_SIZE_CTRL];
    forCtrlNode_init_header_template_local_ctrl(header_template_local);
    struct rte_mbuf *mbuf;
    int pkt_count = 0;

    while(pkt_count < end_index - start_index + 1){
        for (int i = 0; i < lconf->n_rx_queue; i++) { 
            uint32_t nb_rx = rte_eth_rx_burst(lconf->port, lconf->rx_queue_list[i], mbuf_burst, NC_MAX_BURST_SIZE);
            for (int j = 0; j < nb_rx; j++) { 
                mbuf_rcv = mbuf_burst[j]; 
                rte_prefetch0(rte_pktmbuf_mtod(mbuf_rcv, void *));     
                struct rte_ether_hdr* eth = rte_pktmbuf_mtod(mbuf_rcv, struct rte_ether_hdr *);
                if(ntohs(eth->ether_type) == ether_type_switch){
                    pkt_count++;
                    // printf("pkt_count = %d, max = %d\n", pkt_count, end_index - start_index + 1);
                    CtrlHeader_switch* message_header = (CtrlHeader_switch*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE_CTRL);
                    if(message_header->opType == OP_GET_COLD_COUNTER_SWITCH){
                        int index = ntohl(message_header->objIdx);
                        // if(pkt_count != index + 1) printf("error, index = %d\n", index);
                        for(int k = 0; k < FETCHM; k++){
                            struct_coldobj cold_obj;
                            cold_obj.obj_idx = ntohl(message_header->objIdx) * FETCHM + k;//epoch_idx * cache_set_size + i * TOPK + k;
                            cold_obj.cold_count = ntohl(message_header->counter[k]);//(pktNumPartial - bias) * FETCHM - k;//ntohl(message_header->objCounter[k]);
                            // printf("cold_obj.obj_idx = %d, cold_obj.cold_count = %d, cold_obj.cold_count_tmp = %d\n", cold_obj.obj_idx, cold_obj.cold_count, ntohl(message_header->counter[k+4]));
                            if(cold_obj.cold_count < topk_cold_obj_heap_tmp[1].cold_count){
                                topk_cold_obj_heap_tmp[1] = cold_obj;
                                maxheap_downAdjust(topk_cold_obj_heap_tmp, 1, TOPK);
                            } 
                        }
                        
                    }
                }  
                rte_pktmbuf_free(mbuf_rcv);
            }   
        }
    }
}