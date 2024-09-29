#include <iostream>
#include <unistd.h>
#include <unordered_set>
#include <fstream>
#include <iomanip>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <linux/filter.h>
#include <arpa/inet.h>
// #include <ctime>

#include "config.h"
#include "my_header.h"
#include "p4_program.h"
#include "p4_table.h"

//my code
#include <sys/time.h>
#include <random>

#include "./include_dpdk/dpdk_program.h"
#include "./include_dpdk/cache_upd_ctrl.h"

using namespace std;

uint8_t hitrate_flag = 0;
double reciPowSum(
    double para_a,
	double para_n
){
    double res = 0;
	for(unsigned int i = 0; i < para_n; i++){
		res += pow(1.0 / (i + 1), para_a);
	}
	return res;
}

static void nic_0_func(uint32_t lcore_id, P4Program &program) {

	// ctrl function
	auto &mapping = program.getTable("Ingress.index_mapping_table");
	auto lock_id_key_id = mapping.getKey("hdr.kv.key");
	auto mapping_action_id = mapping.getAction("Ingress.mapping");
	auto mapping_action_mapped_index_data_id = mapping.getData("index", "Ingress.mapping");

	auto &suspend = program.getTable("Ingress.suspend_flag_reg");
	auto suspend_index_id = suspend.getKey("$REGISTER_INDEX");
	auto suspend_data_id = suspend.getData("Ingress.suspend_flag_reg.f1");

	// init entries
	int begin = 0;
	int end = 0;//0;//10000;
	int debug = 0;
	int debug2 = 0;
	int counter_reg_num = 4;
	unordered_map<uint32_t, uint32_t> index_mapping;
	unordered_map<uint32_t, uint32_t> reverse_mapping;
	{
		cout << "Initiating mapping..." << endl;
		cout << "Init lock from lock id " << begin << " to lock id " << end << endl;

		program.beginBatch();
		for(uint64_t i = begin; i < end; ++i)
		{
			uint32_t id = i;
			uint32_t index = i;

			index_mapping[id] = index;
			reverse_mapping[index] = id;

			uint8_t table_key[16] = {0};
			for(int j = 0; j < 4; j++){
				table_key[15-j] = (id >> (8 * j)) & 0xff;
			}

			mapping.keyHandlerReset();
			// mapping.keyHandlerSetValue(lock_id_key_id, reverse_mapping[i]);
			mapping.keyHandlerSetValue(lock_id_key_id, table_key, 16);
			mapping.dataHandlerReset(mapping_action_id);
			mapping.dataHandlerSetValue(mapping_action_mapped_index_data_id, i);
			mapping.tableEntryAdd();
		}
		program.endBatch(true);
	}

	// init precise counter
	vector<P4Table*> precise_counter;
	vector<bf_rt_id_t> precise_counter_index_id;
	vector<bf_rt_id_t> precise_counter_data_id;
	{
		cout << "Initiating precise counter..." << endl;

		for(int i = 0; i < 4; ++i)
		{
			precise_counter.push_back(&program.getTable("counter_bucket_reg_" + to_string(i)));
			precise_counter_index_id.push_back(precise_counter[i]->getKey("$REGISTER_INDEX"));
			precise_counter_data_id.push_back(precise_counter[i]->getData("Ingress.counter_bucket_reg_" + to_string(i) + ".f1"));
		}
	}

	// init value len
	uint64_t length = 128;//ntohl(128);
	auto &valueLen = program.getTable("Ingress.kvs.valid_length_reg");
	auto valueLen_index_id = valueLen.getKey("$REGISTER_INDEX");
	auto valueLen_data_id = valueLen.getData("Ingress.kvs.valid_length_reg.f1");
	{
		cout << "Initiating valueLen..." << endl;
		cout << "Init lock from obj id " << begin << " to obj id " << end << endl;

		program.beginBatch();
		for(uint64_t i = begin; i < end; ++i)
		{
			valueLen.keyHandlerReset();
			valueLen.keyHandlerSetValue(valueLen_index_id, i);
			valueLen.dataHandlerReset();
			valueLen.dataHandlerSetValue(valueLen_data_id, length);
			valueLen.tableEntryModify();
			valueLen.completeOperations();
		}
		program.endBatch(true);
	}



	/****************** r e c o r d - h o t s p o t s ******************/

	ifstream fin_hotspots;
	ofstream fout_hotspots;
	ofstream fout_hotspots_onlysuccoff;
	ofstream fout_hotspots_4ini;
	ofstream fout_hotspots_4check;
	fin_hotspots.open("/home/zxy/p4_prj/P4-KVS-lyt/P4DPDK-KVS-noturn/P4/c32k_last_hotspots_32k.txt",ios::in);
	fout_hotspots.open("/home/zxy/p4_prj/P4-KVS-lyt/P4DPDK-KVS-noturn/P4/hotspots.txt",ios::out);
	fout_hotspots_onlysuccoff.open("/home/zxy/p4_prj/P4-KVS-lyt/P4DPDK-KVS-noturn/P4/hotspots_onlysuccoff.txt",ios::out);
	fout_hotspots_4ini.open("/home/zxy/p4_prj/P4-KVS-lyt/P4DPDK-KVS-noturn/P4/hotspots_4ini.txt",ios::out);
	fout_hotspots_4check.open("/home/zxy/p4_prj/P4-KVS-lyt/P4DPDK-KVS-noturn/P4/hotspots_4check.txt",ios::out);

	double zipfPara_a = 0.99;
	double zipfPara_n = 1000000;
    double reciPowSum_res = reciPowSum(zipfPara_a, zipfPara_n);
	double* freq_arr = (double*)malloc(sizeof(double) * zipfPara_n * 2);
	uint8_t* state_arr = (uint8_t*)malloc(sizeof(uint8_t) * zipfPara_n * 2);
    for(uint32_t i = 0; i < zipfPara_n; i++){
        uint32_t x = i + 1;
        double tmpFreq = 1 / (pow(x, zipfPara_a) * reciPowSum_res); 
        freq_arr[i] = tmpFreq;
		// printf("freq_arr[%d] = %lf\n", i, tmpFreq);
		// getchar();
		state_arr[i] = 0;
    }

	double true_offloaded_count = 0;
	double hit_rate_total = 0;
	double valid_report_num = 0;
	double total_report_num = 0;

	if(fin_hotspots.is_open()){
		cout << "-----------------------------------\n"
		     << "r-mat initial with last_hotspots(input 0 to begin)" << endl;
		int tmp = 0;
		scanf("%d", &tmp);
		cout << "-----------------------------------\n"
		     << "r-mat initial with last_hotspots begin" << endl;

		program.beginBatch();
		for(uint64_t i = 0; i < SLOT_SIZE; ++i)
		{
			uint32_t key;
			fin_hotspots >> key;		

			uint32_t id = key;
			uint32_t index = i;

			index_mapping[id] = index;
			reverse_mapping[index] = id;

			uint8_t table_key[16] = {0};
			for(int j = 0; j < 4; j++){
				table_key[15-j] = (id >> (8 * j)) & 0xff;
			}

			mapping.keyHandlerReset();
			// mapping.keyHandlerSetValue(lock_id_key_id, reverse_mapping[i]);
			mapping.keyHandlerSetValue(lock_id_key_id, table_key, 16);
			mapping.dataHandlerReset(mapping_action_id);
			mapping.dataHandlerSetValue(mapping_action_mapped_index_data_id, i);
			mapping.tableEntryAdd();

			valueLen.keyHandlerReset();
			valueLen.keyHandlerSetValue(valueLen_index_id, i);
			valueLen.dataHandlerReset();
			valueLen.dataHandlerSetValue(valueLen_data_id, length);
			valueLen.tableEntryModify();
			valueLen.completeOperations();
			
			if(key != 0xffffffff){
				hit_rate_total += freq_arr[key];
				if(key < SLOT_SIZE){
					true_offloaded_count++;
				}
			}
		}
		program.endBatch(true);
		fout_hotspots << "hit_rate_ini:" << "\t" << hit_rate_total << "\tmiss_ratio_ini:" << "\t" << 1 - true_offloaded_count/SLOT_SIZE << endl;
	}


	ofstream file;
	if(debug)
	{
		auto time = std::time(nullptr);
		ostringstream ss;
		ss << put_time(std::localtime(&time), "%Y%m%d%H%M");
		string filename = "/root/Projects/test/log/replacement_" + ss.str() + ".log";
		file.open(filename);
		if(!file)
		{
			cout << "Fail to open file " << filename << endl;
			exit(1);
		}
	}

	char pkt_data[8192];
    char pkt_data_templete[1024];
	struct timeval start_ini,finish_ini;
	struct timeval quit_start_ini,quit_finish_ini;
	long timeuse_ini;

	gettimeofday(&quit_start_ini, NULL );
	gettimeofday(&quit_finish_ini, NULL );
	uint32_t pkt_size = sizeof(LOCK_CTL_HEADER) + 14;
	printf("pkt_size = %d\n", pkt_size);

	/**************************** c o n t r o l - s t a r t ***************************/
		cout << "-----------------------------------\n"
		     << "control begin(input 0 to begin)" << endl;
		int tmp = 0;
		scanf("%d", &tmp);
		cout << "-----------------------------------\n"
		     << "control begin" << endl;

    uint8_t header_template_local[HEADER_TEMPLATE_SIZE_CTRL];
    forCtrlNode_init_header_template_local_ctrl(header_template_local);

    // initiate lconf
    struct lcore_configuration *lconf = &lcore_conf[lcore_id];
    // memory related points
    struct rte_mbuf *mbuf; 

    struct rte_mbuf *mbuf_burst[NC_MAX_BURST_SIZE];
    struct rte_mbuf *mbuf_rcv;

    uint64_t cur_tsc = rte_rdtsc();
    uint64_t update_tsc = cur_tsc + rte_get_tsc_hz() / 1000 * ctrlPara.waitTime;
    uint64_t end_tsc = cur_tsc + rte_get_tsc_hz() * END_S;
    uint32_t pkt_count = 0;

    uint32_t analyze_count = 0;
    uint32_t update_count = 0;
    uint32_t clean_count = 0;
    uint64_t analyze_time_total = 0;   
    uint64_t update_time_total = 0;   
    uint64_t clean_time_total = 0;       
    uint64_t start_time = 0;
    uint64_t end_time = 0;
    uint64_t start_time_tmp = 0;
    uint64_t end_time_tmp = 0;
    uint64_t switch_start_time_tmp;
    uint64_t switch_end_time_tmp;
    uint64_t fpga_start_time_tmp;
    uint64_t fpga_end_time_tmp;
	int rx_queue_id = 0;

    float tsc_per_ms = rte_get_tsc_hz() / 1000;
    topk_hot_obj_multiBN=(SqList*)malloc(sizeof(SqList));

	while(true){
        cur_tsc = rte_rdtsc();
        if(unlikely(cur_tsc > end_tsc)){
            if(ctrlNode_stats_stage == STATS_COLLECT){
                runEnd_flag = 1;
                printf("average_analyze_time = %f ms\n", 1.0 * analyze_time_total / analyze_count / (rte_get_tsc_hz() / 1000));
                printf("average_update_time = %f ms\n", 1.0 * update_time_total / update_count / (rte_get_tsc_hz() / 1000));
                printf("average_clean_time = %f ms\n", 1.0 * clean_time_total / clean_count / (rte_get_tsc_hz() / 1000));
                break;
            } 
        }
        switch(ctrlNode_stats_stage){
            case STATS_COLLECT: { 
                if(cur_tsc > update_tsc){
                    //printf("828: STATS_COLLECT starts\n");

                    mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]); 
                    forCtrlNode_generate_ctrl_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, ether_type_switch, OP_LOCK_STAT_SWITCH, 0, NULL, mac_zero);
                    enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                    //printf("828: send OP_LOCK_STAT_SWITCH\n");
                    // enqueue_pkt_with_thres(lcore_id, mbuf, 1, 1);

                    for(int i = 0; i < NUM_BACKENDNODE; i++){
                        char* ip_dst_bn = ip_backendNode_arr[i];
                        struct rte_ether_addr mac_dst_bn = mac_backendNode_arr[i];
                        mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]);
                        forCtrlNode_generate_ctrl_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, ether_type_fpga, OP_LOCK_STAT_FPGA, 0, NULL, mac_dst_bn); 
                        enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0); // enqueue_pkt_with_thres(lcore_id, mbuf, 32, 0);
                    }
                    //printf("828: send OP_LOCK_STAT_FPGA\n");

                    enqueue_pkt_with_thres(lcore_id, mbuf, 1, 1);
                    // printf("send hot-in flag!\n");

                    int comingFlag_switch = 0;
                    int comingCount_fpga = 0;
                    // //printf("828: check point 1.6\n");

                    //printf("828: rcv loop starts\n");
                    while(comingFlag_switch == 0 || comingCount_fpga != NUM_BACKENDNODE){
                        for (int i = 0; i < lconf->n_rx_queue; i++) {
                            uint32_t nb_rx = rte_eth_rx_burst(lconf->port, lconf->rx_queue_list[i], mbuf_burst, NC_MAX_BURST_SIZE);
                            for (int j = 0; j < nb_rx; j++) {
                                mbuf_rcv = mbuf_burst[j];
                                rte_prefetch0(rte_pktmbuf_mtod(mbuf_rcv, void *));     
                                struct rte_ether_hdr* eth = rte_pktmbuf_mtod(mbuf_rcv, struct rte_ether_hdr *);
                                // printf("829: stage = %d, ether_type = %d\n", ctrlNode_stats_stage, ntohs(eth->ether_type));
                                if(ntohs(eth->ether_type) == ether_type_switch){
                                    CtrlHeader_switch* message_header = (CtrlHeader_switch*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE_CTRL);
                                    if(message_header->opType == OP_LOCK_STAT_SWITCH && comingFlag_switch == 0){
                                        comingFlag_switch = 1;
                                        //printf("828: OP_LOCK_STAT_SWITCH ok!\n");
                                        // printf("829: comingFlag_switch = %d\n", comingFlag_switch);
                                    }
                                }
                                else if(ntohs(eth->ether_type) == ether_type_fpga){
                                    CtrlHeader_fpga* message_header = (CtrlHeader_fpga*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE_CTRL);
                                    if(message_header->opType == OP_LOCK_STAT_FPGA && comingCount_fpga != NUM_BACKENDNODE){
                                        comingCount_fpga++;
                                        //printf("828: OP_LOCK_STAT_FPGA ok!\n");
                                        // printf("829: comingFlag_fpga = %d\n", comingFlag_fpga);
                                    }
                                }
                                rte_pktmbuf_free(mbuf_rcv);
                            }
                        }
                    }
                    //printf("828: rcv loop ends\n");

                    ctrlNode_stats_stage = STATS_ANALYZE;
                    epoch_idx = 0;

                    // //printf("828: check point 2\n");
                    //printf("828: STATS_COLLECT ends\n");
                }
                break;}
            case STATS_ANALYZE: {
                //printf("828: STATS_ANALYZE starts\n");
                // usleep(1000000);
                
                //printf("828: send-rcv loop starts (OP_GET_COLD_COUNTER_SWITCH)\n");
                // uint64_t start_time_1 = rte_rdtsc();

				start_time = rte_rdtsc();
				int loop = 2;
				int batch = ctrlPara.cacheSize / FETCHM / loop;
                for(int j = 0; j < TOPK+1; j++){
                    struct_coldobj cold_obj;
                    cold_obj.obj_idx = 0xffffffff;
                    cold_obj.cold_count = 0xffffffff;
                    topk_cold_obj_heap_tmp[j] = cold_obj;
                } 
				for(int i = 0; i < loop; i++){
					// printf("p1: i = %d, loop = %d, batch = %d\n", i, loop, batch);
					send_topk_cold_loop(lcore_id, rx_queue_id, ctrlPara, i * batch, (i + 1) * batch - 1);
					// printf("p2: i = %d\n", i);
					recv_topk_cold_loop(lcore_id, rx_queue_id, ctrlPara, i * batch, (i + 1) * batch - 1);
					// printf("p3: i = %d\n", i);
				}
				end_time = rte_rdtsc();
				int interval_us = 1.0 * (end_time - start_time) / (rte_get_tsc_hz() / 1000000);
				float throughput = ctrlPara.cacheSize / FETCHM / interval_us;
				//printf("828: time for get coldobj = %d us, throughput = %f Mqps\n", interval_us, throughput);


                // min heap !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                // uint64_t start_time_1 = rte_rdtsc();

                for(int i = 0; i < TOPK+1; i++){
                    struct_coldobj cold_obj;
                    cold_obj.obj_idx = 0xffffffff;
                    cold_obj.cold_count = 0xffffffff;
                    topk_cold_obj_heap[i] = cold_obj;
                }
                int findColdLoopNum = (n_lcores - 1) / 2;
                // for(int i = 0; i < findColdLoopNum; i++){
                    for(int k = 0; k < TOPK; k++){
                        struct_coldobj cold_obj = topk_cold_obj_heap_tmp[k+1];
                        if(cold_obj.cold_count < topk_cold_obj_heap[1].cold_count){
                            topk_cold_obj_heap[1] = cold_obj;
                            maxheap_downAdjust(topk_cold_obj_heap, 1, TOPK);
                        } 
                    }
                // }
                for(int i = 0; i < TOPK; i++){
                    topk_cold_obj[i] = topk_cold_obj_heap[i + 1];
                }
                
                // sort -- decreasing
                for(int i = 0; i < TOPK - 1; i++){
                    for(int j = 0; j < TOPK - 1 - i; j++){
                        if(topk_cold_obj[j].cold_count < topk_cold_obj[j+1].cold_count){
                            struct_coldobj temp = topk_cold_obj[j];
                            topk_cold_obj[j] = topk_cold_obj[j+1];
                            topk_cold_obj[j+1] = temp;
                        }
                    }
                }

                // uint64_t end_time_1 = rte_rdtsc();
                

                // end_time = rte_rdtsc(); 
                // printf("average_analyze_time = %f ms\n", 1.0 * (end_time - start_time) / (rte_get_tsc_hz() / 1000));
                // printf("topk_cold_obj (obj_idx, obj_counter):\n");
                // for(int i = 0; i < TOPK; i++){
                //     printf("(%d, %d)\t", topk_cold_obj[i].obj_idx, topk_cold_obj[i].cold_count);
                // }
                // printf("\n");
                //printf("828: OP_GET_COLD_COUNTER_SWITCH ok!\n");

                

                for(int i = 0; i < NUM_BACKENDNODE; i++){
                    char* ip_dst_bn = ip_backendNode_arr[i];
                    struct rte_ether_addr mac_dst_bn = mac_backendNode_arr[i];
                    mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]);
                    forCtrlNode_generate_ctrl_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, ether_type_fpga, OP_GET_HOT_COUNTER_FPGA, 0, NULL, mac_dst_bn); 
                    enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0); // enqueue_pkt_with_thres(lcore_id, mbuf, 32, 0);
                }
                enqueue_pkt_with_thres(lcore_id, mbuf, 1, 1);
                
                //printf("828: send OP_GET_HOT_COUNTER_FPGA\n");

                int topk_hot_obj_num = 0;
                int comingCount_fpga = 0;
                topk_hot_obj_multiBN->length = 0;
                
                while(comingCount_fpga != NUM_BACKENDNODE * max_pkts_from_one_bn){
                    for (int i = 0; i < lconf->n_rx_queue; i++) {
                        uint32_t nb_rx = rte_eth_rx_burst(lconf->port, lconf->rx_queue_list[i], mbuf_burst, NC_MAX_BURST_SIZE);
                        for (int j = 0; j < nb_rx; j++) {
                            mbuf_rcv = mbuf_burst[j];
                            rte_prefetch0(rte_pktmbuf_mtod(mbuf_rcv, void *)); 
                            struct rte_ether_hdr* eth = rte_pktmbuf_mtod(mbuf_rcv, struct rte_ether_hdr *);
                            // //printf("828: stage = %d, ether_type = %d\n", ctrlNode_stats_stage, ntohs(eth->ether_type));
                            if(ntohs(eth->ether_type) == ether_type_fpga){
                                CtrlHeader_fpga_big* message_header = (CtrlHeader_fpga_big*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE_CTRL);
                                if(message_header->opType == OP_GET_HOT_COUNTER_FPGA && comingCount_fpga != NUM_BACKENDNODE * max_pkts_from_one_bn){
                                    comingCount_fpga++;
                                    //!!!!!!!!!!!!!!!!!!!!!!!!!!!�ҵ���������滻��
                                    for(int k = 0; k < NUM_BACKENDNODE; k++){
                                        char* ip_dst_bn = ip_backendNode_arr[k];
                                        struct rte_ether_addr mac_dst_bn = mac_backendNode_arr[k];
                                        if(mac_compare(mac_dst_bn, eth->s_addr)){
                                            
                                            for(int m = 0; m < HOTOBJ_IN_ONEPKT; m++){
                                                SqNote tmp_hotobj;
                                                tmp_hotobj.obj = ntohl(message_header->obj[m]);
                                                tmp_hotobj.count = ntohl(message_header->counter[m]);
                                                tmp_hotobj.backendNode_idx = k;
                                                if(tmp_hotobj.count > 0){
                                                    topk_hot_obj_multiBN->r[1 + topk_hot_obj_multiBN->length] = tmp_hotobj;
                                                    topk_hot_obj_multiBN->length++;
                                                }
                                            }
                                            break;
                                        }
                                    }
                                }
                                else{
                                    //printf("828: comingCount_fpga = %d\n", comingCount_fpga);
                                    //printf("828: other pkts, ether_type = %u (expect = %u), op = %u (expect = %u)\n", ntohs(eth->ether_type), ether_type_fpga, message_header->opType, OP_GET_HOT_COUNTER_FPGA);
                                }
                            }
                            else{
                                // CtrlHeader_fpga_big* message_header = (CtrlHeader_switch*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE_CTRL);
                                // //printf("828: other pkts, ether_type = %u (expect = %u), op = %u (expect = %u)\n", ntohs(eth->ether_type), ether_type_fpga, message_header->opType, OP_GET_HOT_COUNTER_FPGA);
                            }
                            rte_pktmbuf_free(mbuf_rcv);
                        }
                    }
                }


                // uint64_t start_time_2 = rte_rdtsc();
                // sort -- increasing
                QuickSort(topk_hot_obj_multiBN);
                topk_hot_obj_num = topk_hot_obj_multiBN->length;
                //-----------------

                topk_upd_obj_num = 0;
                for(int i = 0; i < NUM_BACKENDNODE; i++){
                    topk_upd_obj_num_multiBN[i] = 0;
                }

                int bound = 0;
                if(topk_hot_obj_num < TOPK){
                    bound = topk_hot_obj_num;
                }
                else{
                    bound = TOPK;
                }
                for(int i = 0; i < bound; i++){
                    if(topk_hot_obj_multiBN->r[1+(topk_hot_obj_num-1-i)].count > topk_cold_obj[TOPK-1-i].cold_count){
                        topk_upd_obj_num++;
                    }
                }

                // uint64_t end_time_2 = rte_rdtsc();
                // printf("time_p1 = %f ms, time_p2 = %f ms\n", 1.0 * (end_time_1 - start_time_1) / tsc_per_ms, 1.0 * (end_time_2 - start_time_2) / tsc_per_ms);

                if(topk_upd_obj_num > 0){
                    for(int i = 0; i < topk_upd_obj_num; i++){
                        topk_hot_obj_upd[i].obj = topk_hot_obj_multiBN->r[1+(topk_hot_obj_num-1-i)].obj;
                        topk_hot_obj_upd[i].hot_count = topk_hot_obj_multiBN->r[1+(topk_hot_obj_num-1-i)].count;
                        topk_cold_obj_upd[i] = topk_cold_obj[TOPK-1-i]; 
                        topk_hot_obj_upd[i].obj_idx = topk_cold_obj_upd[i].obj_idx;
                        //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                        int tmp_idx = topk_hot_obj_multiBN->r[1+(topk_hot_obj_num-1-i)].backendNode_idx;
                        int tmp_num = topk_upd_obj_num_multiBN[tmp_idx];
                        topk_upd_obj_multiBN[tmp_idx][tmp_num].obj = topk_hot_obj_multiBN->r[1+(topk_hot_obj_num-1-i)].obj;
                        // topk_upd_obj_multiBN[tmp_idx][tmp_num].obj_idx = topk_cold_obj_upd[i].obj_idx;
                        topk_upd_obj_num_multiBN[tmp_idx]++;
                    }
                    ctrlNode_stats_stage = STATS_UPDATE;
                }
                else{

                    // /*********************** re p o r t   h o t s - p o t s *************************/
                    // forCtrlNode_generate_ctrl_pkt_mul(rx_queue_id, header_template_local, lcore_id, mbuf, ether_type_switch, OP_HOT_REPORT_SWITCH, topk_upd_obj_num,  topk_hot_obj_upd, mac_zero, 0);

                    //printf("828: checkpoint 2!\n");
                    for(int i = 0; i < NUM_BACKENDNODE; i++){
                        char* ip_dst_bn = ip_backendNode_arr[i];
                        struct rte_ether_addr mac_dst_bn = mac_backendNode_arr[i];
                        //printf("828: checkpoint 2.1!\n");
                        mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]);
                        //printf("828: checkpoint 2.2!\n");
                        forCtrlNode_generate_ctrl_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, ether_type_fpga, OP_LOCK_HOT_QUEUE_FPGA, topk_upd_obj_num_multiBN[i], topk_upd_obj_multiBN[i], mac_dst_bn);
                        //printf("828: checkpoint 2.3!\n");
                        enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0); // enqueue_pkt_with_thres(lcore_id, mbuf, 32, 0);
                    }
                    //printf("828: checkpoint 3!\n");
                    enqueue_pkt_with_thres(lcore_id, mbuf, 1, 1);

                    //printf("828: send OP_LOCK_HOT_QUEUE_FPGA (don't update)\n");

                    //printf("828: rcv loop starts\n");
                    int comingCount_fpga = 0;
                    while(comingCount_fpga != NUM_BACKENDNODE){
                        for (int i = 0; i < lconf->n_rx_queue; i++) {
                            uint32_t nb_rx = rte_eth_rx_burst(lconf->port, lconf->rx_queue_list[i], mbuf_burst, NC_MAX_BURST_SIZE);
                            for (int j = 0; j < nb_rx; j++) {
                                mbuf_rcv = mbuf_burst[j];
                                rte_prefetch0(rte_pktmbuf_mtod(mbuf_rcv, void *)); 
                                struct rte_ether_hdr* eth = rte_pktmbuf_mtod(mbuf_rcv, struct rte_ether_hdr *);
                                if(ntohs(eth->ether_type) == ether_type_fpga){
                                    CtrlHeader_fpga_big* message_header = (CtrlHeader_fpga_big*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE_CTRL);
                                    if(message_header->opType == OP_LOCK_HOT_QUEUE_FPGA && comingCount_fpga != NUM_BACKENDNODE){
                                        comingCount_fpga++;
                                        //printf("828: OP_LOCK_HOT_QUEUE_FPGA ok (don't update)\n");
                                    }
                                }
                                rte_pktmbuf_free(mbuf_rcv);
                            }
                        }
                    }
                    //printf("828: rcv loop ends\n");
                    
                    epoch_idx++;
                    if(epoch_idx < ctrlPara.updEpochs){
                        ctrlNode_stats_stage = STATS_ANALYZE;
                    }
                    else{
                        ctrlNode_stats_stage = STATS_CLEAN;
                    }
                }

                end_time = rte_rdtsc(); 
                analyze_time_total += end_time - start_time;
                analyze_count += 1;

                // printf("topk_upd_obj (obj, obj_idx, obj_counter):\n");
                // for(int i = 0; i < topk_upd_obj_num; i++){
                //     printf("(%d, %d, %d)\t", topk_hot_obj_upd[i].obj, topk_hot_obj_upd[i].obj_idx, topk_hot_obj_upd[i].hot_count);
                // }
                // printf("\n");
                //printf("828: STATS_ANALYZE ends\n");
                break;}


			case STATS_UPDATE: {
                //printf("828: STATS_UPDATE starts\n");

                start_time = rte_rdtsc();

                start_time_tmp = rte_rdtsc();//-------------------------------
                uint64_t start_time_1 = rte_rdtsc();
                // obj the queue on switch and wait for queue empty
                for(int i = 0; i < NUM_BACKENDNODE; i++){
                    char* ip_dst_bn = ip_backendNode_arr[i];
                    struct rte_ether_addr mac_dst_bn = mac_backendNode_arr[i];
                    uint32_t upd_num = 0;
                    if(topk_upd_obj_num_multiBN[i] == 0){
                        upd_num = 0xffffffff;
                        mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]);
                        forCtrlNode_generate_ctrl_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, ether_type_fpga, OP_LOCK_HOT_QUEUE_FPGA, upd_num, topk_upd_obj_multiBN[i], mac_dst_bn);
                        enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0); // enqueue_pkt_with_thres(lcore_id, mbuf, 32, 0);
                    }
                    else{
                        upd_num = topk_upd_obj_num_multiBN[i];
                        uint32_t pkts_to_each_bn = (upd_num - 1) / HOTOBJ_IN_ONEPKT + 1;
                        for(int j = 0; j < pkts_to_each_bn; j++){
                            if(j < pkts_to_each_bn - 1){
                                mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]);
                                forCtrlNode_generate_ctrl_pkt_mul(rx_queue_id, header_template_local, lcore_id, mbuf, ether_type_fpga, OP_LOCK_HOT_QUEUE_FPGA, upd_num, topk_upd_obj_multiBN[i] + HOTOBJ_IN_ONEPKT * j, mac_dst_bn, HOTOBJ_IN_ONEPKT);
                                enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0); // enqueue_pkt_with_thres(lcore_id, mbuf, 32, 0);
                                rte_sleep_us(SLEEP_US_BIGPKT);
                            }
                            else{
                                mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]);
                                forCtrlNode_generate_ctrl_pkt_mul(rx_queue_id, header_template_local, lcore_id, mbuf, ether_type_fpga, OP_LOCK_HOT_QUEUE_FPGA, upd_num, topk_upd_obj_multiBN[i] + HOTOBJ_IN_ONEPKT * j, mac_dst_bn, upd_num - HOTOBJ_IN_ONEPKT * j);
                                enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0); // enqueue_pkt_with_thres(lcore_id, mbuf, 32, 0);
                                rte_sleep_us(SLEEP_US_BIGPKT);
                            }
                            
                        }
                    }
                }
                enqueue_pkt_with_thres(lcore_id, mbuf, 1, 1);
                //printf("828: send OP_LOCK_HOT_QUEUE_FPGA\n");

                for (int i = 0; i < topk_upd_obj_num; i++) {
                    mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]); 
					//printf("828: topk_cold_obj_upd[%d].obj_idx = %d\n", i, topk_cold_obj_upd[i].obj_idx);
                    forCtrlNode_generate_ctrl_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, ether_type_switch, OP_LOCK_COLD_QUEUE_SWITCH, topk_cold_obj_upd[i].obj_idx, NULL, mac_zero);
                    enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0); // enqueue_pkt_with_thres(lcore_id, mbuf, 32, 0); 
                }
                enqueue_pkt_with_thres(lcore_id, mbuf, 1, 1);
                //printf("828: send OP_LOCK_COLD_QUEUE_SWITCH\n");
                end_time_tmp = rte_rdtsc();//-------------------------------
                // printf("211:OP_LOCK_HOT_QUEUE time = %f ms\n", 1.0 * (end_time_tmp - start_time_tmp) / tsc_per_ms);//-------------------------------

                start_time_tmp = rte_rdtsc();//-------------------------------
                //printf("828: rcv loop starts\n");
                int fpga_coming_count = 0;
                int fpga_empty_count = 0;
                int p4_empty_flag = 0;
                int p4_empty_count = 0;
                // while((fpga_coming_count + fpga_empty_count < NUM_BACKENDNODE + NUM_BACKENDNODE) || p4_empty_flag == 0){
                while(fpga_coming_count < NUM_BACKENDNODE || fpga_empty_count < NUM_BACKENDNODE || p4_empty_flag == 0){
                    for (int i = 0; i < lconf->n_rx_queue; i++) {
                        uint32_t nb_rx = rte_eth_rx_burst(lconf->port, lconf->rx_queue_list[i], mbuf_burst, NC_MAX_BURST_SIZE);
                        for (int j = 0; j < nb_rx; j++) {
                            mbuf_rcv = mbuf_burst[j];
                            rte_prefetch0(rte_pktmbuf_mtod(mbuf_rcv, void *)); 
                            struct rte_ether_hdr* eth = rte_pktmbuf_mtod(mbuf_rcv, struct rte_ether_hdr *);
                            if(ntohs(eth->ether_type) == ether_type_switch){
                                CtrlHeader_switch* message_header = (CtrlHeader_switch*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE_CTRL);
                                if(message_header->opType == OP_LOCK_COLD_QUEUE_SWITCH || message_header->opType == OP_CALLBACK){
                                    p4_empty_count++;
                                    //printf("828: obj idx = %d empty!\n", ntohl(message_header->objIdx));
                                    if(p4_empty_count == topk_upd_obj_num){
                                        p4_empty_flag = 1;
                                        //printf("828: OP_EMPTY_SWITCH ok\n");
                                    }
                                }
                                else{
                                    //printf("828: other pkts, ether_type = %u, opType = %u\n", ntohs(eth->ether_type), message_header->opType);
                                }
                            }
                            else if(ntohs(eth->ether_type) == ether_type_fpga){
                                CtrlHeader_fpga* message_header = (CtrlHeader_fpga*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE_CTRL);
                                if(message_header->opType == OP_QUEUE_EMPTY_FPGA){
                                    fpga_empty_count++;
                                    //printf("828: OP_EMPTY_FPGA: stage = %d, stage_count = %d, node = %d ok\n", message_header->replaceNum, message_header->obj[0], message_header->obj[1]);
                                }
                                else if(message_header->opType == OP_LOCK_HOT_QUEUE_FPGA){
                                    fpga_coming_count++;
                                    //printf("828: OP_LOCK_HOT_QUEUE_FPGA: stage = %d, stage_count = %d, node = %d ok\n", message_header->replaceNum, message_header->obj[0], message_header->obj[1]);
                                    // //printf("828: OP_LOCK_HOT_QUEUE_FPGA ok\n");
                                }
                                else{
                                    //printf("828: other pkts, ether_type = %u\n", ntohs(eth->ether_type));
                                }
                            }
                            else{
                                //printf("828: other pkts, ether_type = %u\n", ntohs(eth->ether_type));
                            }
                            rte_pktmbuf_free(mbuf_rcv);
                        }
                    }
                }

                uint64_t end_time_1 = rte_rdtsc();
                // printf("topk_upd_obj_num=%d, time_p1 = %f ms\n", topk_upd_obj_num, 1.0 * (end_time_1 - start_time_1) / tsc_per_ms);

                //printf("828: rcv loop ends\n");
                end_time_tmp = rte_rdtsc();//-------------------------------
                // printf("211:QUEUE EMPTY time = %f ms\n", 1.0 * (end_time_tmp - start_time_tmp) / tsc_per_ms);//-------------------------------

                start_time_tmp = rte_rdtsc();//-------------------------------
                //!!!!!!!!!!!!!!!!!!!!OP_HOT_REPORT_FPGA
                for(int i = 0; i < NUM_BACKENDNODE; i++){
                    char* ip_dst_bn = ip_backendNode_arr[i];
                    struct rte_ether_addr mac_dst_bn = mac_backendNode_arr[i];
                    mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]);
                    forCtrlNode_generate_ctrl_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, ether_type_fpga, OP_HOT_REPORT_FPGA, 0, NULL, mac_dst_bn);
                    enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0); // enqueue_pkt_with_thres(lcore_id, mbuf, 32, 0);
                }                
                enqueue_pkt_with_thres(lcore_id, mbuf, 1, 1);


                // uint32_t pkts_to_cp = (topk_upd_obj_num - 1) / HOTOBJ_IN_ONEPKT + 1;
                // for(int i = 0; i < pkts_to_cp; i++){
                //     if(i < pkts_to_cp - 1){
                //         mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]); 
                //         forCtrlNode_generate_ctrl_pkt_mul(rx_queue_id, header_template_local, lcore_id, mbuf, ether_type_switch, OP_HOT_REPORT_SWITCH, topk_upd_obj_num, topk_hot_obj_upd + HOTOBJ_IN_ONEPKT * i, mac_zero, HOTOBJ_IN_ONEPKT);
                //         enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0); // enqueue_pkt_with_thres(lcore_id, mbuf, 32, 0);
                //         rte_sleep_us(SLEEP_US_BIGPKT);
                //     }
                //     else{
                //         mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]); 
                //         forCtrlNode_generate_ctrl_pkt_mul(rx_queue_id, header_template_local, lcore_id, mbuf, ether_type_switch, OP_HOT_REPORT_SWITCH, topk_upd_obj_num, topk_hot_obj_upd + HOTOBJ_IN_ONEPKT * i, mac_zero, topk_upd_obj_num - HOTOBJ_IN_ONEPKT * i);
                //         enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0); // enqueue_pkt_with_thres(lcore_id, mbuf, 32, 0);
                //         rte_sleep_us(SLEEP_US_BIGPKT);
                //     }
                // }
                // enqueue_pkt_with_thres(lcore_id, mbuf, 1, 1);


				// uint32_t continue_flag = 1;
        		// uint32_t id_arr[TOPK];
        		// uint32_t index_arr[TOPK];
        		// uint32_t hot_obj_num = 0;
        		// uint32_t hot_obj_count = 0;

        		// uint32_t rcv_num = rcv_pkt_seq(lcore_id, pkt_data, pkt_size);
				// for(int i = 0; i < rcv_num; i++){
				// 	auto *lock_ctl_header = (LOCK_CTL_HEADER *) (pkt_data + pkt_size * i + 14);
				// 	if(lock_ctl_header->op == HOT_REPORT){
        		//         uint32_t tmp = ntohl(lock_ctl_header->replace_num);
				// 		if(hot_obj_num == 0){
        		//             hot_obj_num = tmp & 0xffff;
        		//             rte_memcpy(pkt_data_templete, pkt_data, pkt_size);
        		//         }
        		//         uint32_t hot_obj_num_in_pkt = tmp >> 16;
				// 		// printf("hot_obj_num = %d, hot_obj_num_in_pkt = %d, hot_obj_count = %d\n", hot_obj_num, hot_obj_num_in_pkt, hot_obj_count);
        		//         for(int j = 0; j < hot_obj_num_in_pkt; j++){
				// 			// printf("(%d, %d, %d)\n", j, ntohl(lock_ctl_header->id[j]), ntohl(lock_ctl_header->index[j]));
        		//             id_arr[hot_obj_count] = ntohl(lock_ctl_header->id[j]);
        		//             index_arr[hot_obj_count] = ntohl(lock_ctl_header->index[j]);
        		//             hot_obj_count++;
        		//         }
				// 		continue_flag = 0;

				// 	}
				// }
        		// if(continue_flag == 0){
        		//     while(hot_obj_count < hot_obj_num){
        		//         uint32_t rcv_num = rcv_pkt_seq(lcore_id, pkt_data, pkt_size);
				//         for(int i = 0; i < rcv_num; i++){
				//         	auto *lock_ctl_header = (LOCK_CTL_HEADER *) (pkt_data + pkt_size * i + 14);
				//         	if(lock_ctl_header->op == HOT_REPORT){
        		//                 uint32_t tmp = ntohl(lock_ctl_header->replace_num);
        		//                 uint32_t hot_obj_num_in_pkt = tmp >> 16;
				// 				// printf("addition: hot_obj_num = %d, hot_obj_num_in_pkt = %d, hot_obj_count = %d\n", hot_obj_num, hot_obj_num_in_pkt, hot_obj_count);
        		//                 for(int j = 0; j < hot_obj_num_in_pkt; j++){
				// 					// printf("(%d, %d, %d)\n", j, ntohl(lock_ctl_header->id[j]), ntohl(lock_ctl_header->index[j]));
        		//                     id_arr[hot_obj_count] = ntohl(lock_ctl_header->id[j]);
        		//                     index_arr[hot_obj_count] = ntohl(lock_ctl_header->index[j]);
        		//                     hot_obj_count++;
        		//                 }
				//         	}
				//         }
        		//     }  
        		// }
				// // printf("continue_flag = %d\n", continue_flag);
				// if(continue_flag == 1){
				// 	gettimeofday(&quit_finish_ini, NULL );
				// 	timeuse_ini = 1000000 * ( quit_finish_ini.tv_sec - quit_start_ini.tv_sec ) + quit_finish_ini.tv_usec - quit_start_ini.tv_usec;
				// 	// printf("No packet comes for %f us\n", timeuse_ini * 1.0);
				// 	continue;
				// }
				// else{
				// 	gettimeofday(&quit_start_ini, NULL );
				// }

				// gettimeofday(&start_ini, NULL ); //-----------------------------


				uint32_t hot_obj_num = topk_upd_obj_num;
        		uint32_t id_arr[TOPK];
        		uint32_t index_arr[TOPK];
				for(int i = 0; i < hot_obj_num; i++){
        			id_arr[i] = topk_hot_obj_upd[i].obj;
        			index_arr[i] = topk_hot_obj_upd[i].obj_idx;
				}

				if(hot_obj_num > 0){
					int n = hot_obj_num;
					int e = 0;

					if(debug) file << "Receive " << n << " hot lock" << endl;

					// program.beginBatch();
					for(int i = 0; i < n && i < TOPK; ++i)
					{
					
						uint64_t id = id_arr[i];
						uint64_t index = index_arr[i];
						uint64_t counter = 0xffffffff;//ntohl(lock_ctl_header->counter[i]);



        		        /****************** r e c o r d - h o t s p o t s ******************/
				        valid_report_num++;
				        total_report_num++;
				        uint32_t key_insert = id;
				        uint32_t key_delete = 0xffffffff;
				        auto reverse_iter = reverse_mapping.find(index);
				        if(key_insert < SLOT_SIZE){
				        	true_offloaded_count++;
				        }
				        hit_rate_total += freq_arr[key_insert];
				        if(reverse_iter != reverse_mapping.cend()){
				        	key_delete = reverse_iter->second;
				        }
				        if(key_delete != 0xffffffff){
				        	hit_rate_total -= freq_arr[key_delete];
				        	if(key_delete < SLOT_SIZE){
				        		true_offloaded_count--;
				        	}
				        } 
				        if(hitrate_flag == 1) fout_hotspots << key_insert << "\t" << key_delete << "\t" << hit_rate_total << "\t" << 1 - true_offloaded_count/SLOT_SIZE << "\t" << valid_report_num / total_report_num << endl;
				        if(hitrate_flag == 1) fout_hotspots_onlysuccoff << key_insert << "\t" << key_delete << "\t" << hit_rate_total << "\t" << 1 - true_offloaded_count/SLOT_SIZE << "\t" << valid_report_num / total_report_num << endl;                



						if(debug) file << "checkpoint 1: i = " << i << "; id = " << id << "; index = " << index << "; counter = " << counter << endl;

						if(index >= SLOT_SIZE)
						{
							if(debug) file << "{" << id << ", " << index << "}: index error" << endl;
							continue;
						}

						if(debug) file << "checkpoint 2" << endl;

						auto index_iter = index_mapping.find(id);
						if(index_iter == index_mapping.cend())
						{
							if(debug) file << "checkpoint 3" << endl;
							auto reverse_iter = reverse_mapping.find(index);
							if(reverse_iter != reverse_mapping.cend())
							{
								if(debug) file << "checkpoint 4" << endl;
								index_mapping.erase(reverse_iter->second);

								uint8_t table_key[16] = {0};
								for(int j = 0; j < 4; j++){
									table_key[15-j] = (reverse_iter->second >> (8 * j)) & 0xff;
								}

								if(debug) file << "checkpoint 5" << endl;
								mapping.keyHandlerReset();
								// mapping.keyHandlerSetValue(lock_id_key_id, reverse_iter->second);
								mapping.keyHandlerSetValue(lock_id_key_id, table_key, 16);
								mapping.tableEntryDelete();
								mapping.completeOperations();

								if(debug) file << "{" << reverse_iter->second << ", " << index << "} ==> ";
							}
//						    else if(reverse_mapping.size() < SLOT_SIZE)
//						    {
//						    	index = reverse_mapping.size();
//						    }

							if(debug) file << "checkpoint 6" << endl;

							index_mapping[id] = index;
							reverse_mapping[index] = id;

							if(debug) file << "checkpoint 7" << endl;

							uint8_t table_key[16] = {0};
							for(int j = 0; j < 4; j++){
								table_key[15-j] = (id >> (8 * j)) & 0xff;
							}

							if(debug) file << "checkpoint 8" << endl;

							// mapping.modifyEntry(make_tuple(index, id));
							mapping.keyHandlerReset();
							// mapping.keyHandlerSetValue(lock_id_key_id, id);
							mapping.keyHandlerSetValue(lock_id_key_id, table_key, 16);
							mapping.dataHandlerReset(mapping_action_id);
							mapping.dataHandlerSetValue(mapping_action_mapped_index_data_id, index);
							mapping.tableEntryAdd();
							mapping.completeOperations();

							if(debug) file << "checkpoint 9" << endl;

							precise_counter[index % counter_reg_num]->keyHandlerReset();
							precise_counter[index % counter_reg_num]->keyHandlerSetValue(precise_counter_index_id[index % counter_reg_num], index / counter_reg_num);
							precise_counter[index % counter_reg_num]->dataHandlerReset();
							precise_counter[index % counter_reg_num]->dataHandlerSetValue(precise_counter_data_id[index % counter_reg_num], counter);
							precise_counter[index % counter_reg_num]->tableEntryModify();
							precise_counter[index % counter_reg_num]->completeOperations();

							if(debug2) cout << "{" << id << ", " << index << "}" << endl;
							if(debug) file << "{" << id << ", " << index << "}" << endl;
						}
						else
						{
							if(debug2) cout << "{" << id << ", " << index_iter->second << "} <=> {" << id << ", " << index << "}" << endl;
							if(debug) file << "{" << id << ", " << index_iter->second << "} <=> {" << id << ", " << index << "}" << endl;
							e++;
						}

						valueLen.keyHandlerReset();
						valueLen.keyHandlerSetValue(valueLen_index_id, index);
						valueLen.dataHandlerReset();
						valueLen.dataHandlerSetValue(valueLen_data_id, length);
						valueLen.tableEntryModify();
						valueLen.completeOperations();

						// suspend.modifyEntry(index, 0);
						// ������Ҫ�ŵ�ѭ�����浥��batch������������Ƶ�ʱ��㣩
						suspend.keyHandlerReset();
						suspend.keyHandlerSetValue(suspend_index_id, index);
						suspend.dataHandlerReset();
						suspend.dataHandlerSetValue(suspend_data_id, (uint64_t) 0);
						suspend.tableEntryModify();
						suspend.completeOperations();
					}
					// program.endBatch(true);

					if(debug) file << "checkpoint 10" << endl;

					// auto *ether_header = (ETH_HEADER *) pkt_data_templete;
        		    // auto *lock_ctl_header = (LOCK_CTL_HEADER *) (pkt_data_templete + 14);
					// swap(ether_header->DestMac, ether_header->SrcMac);
					// lock_ctl_header->op = REPLACE_SUCC;

					// // cout << "print one letter to send response:" << endl;
					// // getchar();

					// send_pkt_seq(lcore_id, pkt_data_templete, sizeof(LOCK_CTL_HEADER) + 14);
					if(debug) cout << "Successfully handle " << n - e << " hot replace request" << endl;

					gettimeofday(&finish_ini, NULL); //----------------------------
					timeuse_ini = 1000000 * ( finish_ini.tv_sec - start_ini.tv_sec ) + finish_ini.tv_usec - start_ini.tv_usec;
					if(debug2) printf("TOPK(gettimeofday) %f\n", timeuse_ini * 1.0);
				}



				/****************** r e c o r d - h o t s p o t s ******************/
				if(hitrate_flag == 1){
					for(int i = hot_obj_num; i < TOPK; i++){
						total_report_num++;
						fout_hotspots << "dontknow" << "\t" << "dontknow" << "\t" << hit_rate_total << "\t" << 1 - true_offloaded_count/SLOT_SIZE << "\t" << valid_report_num / total_report_num << endl;
					}
				} 


        		//printf("828: rcv loop starts\n");
        		int comingFlag_switch = 1;
        		int comingCount_fpga = 0;
        		switch_start_time_tmp = rte_rdtsc();//-------------------------------
        		fpga_start_time_tmp = rte_rdtsc();//-------------------------------
        		// pkt_count = 2;
        		while(comingFlag_switch == 0 || comingCount_fpga != NUM_BACKENDNODE){
        		    for (int i = 0; i < lconf->n_rx_queue; i++) {
        		        uint32_t nb_rx = rte_eth_rx_burst(lconf->port, lconf->rx_queue_list[i], mbuf_burst, NC_MAX_BURST_SIZE);
        		        for (int j = 0; j < nb_rx; j++) {
        		            mbuf_rcv = mbuf_burst[j];
        		            rte_prefetch0(rte_pktmbuf_mtod(mbuf_rcv, void *)); 
        		            struct rte_ether_hdr* eth = rte_pktmbuf_mtod(mbuf_rcv, struct rte_ether_hdr *);
        		            if(ntohs(eth->ether_type) == ether_type_switch){
        		                CtrlHeader_switch* message_header = (CtrlHeader_switch*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE_CTRL);
        		                if(message_header->opType == OP_REPLACE_SUCCESS && comingFlag_switch == 0){
        		                    comingFlag_switch = 1;
        		                    switch_end_time_tmp = rte_rdtsc();//-------------------------------
        		                    // printf("211:OP_HOT_REPORT time (switch) = %f ms\n", 1.0 * (switch_end_time_tmp - switch_start_time_tmp) / tsc_per_ms);//-------------------------------
        		                    //printf("828: OP_REPLACE_SUCCESS_SWITCH ok\n");
        		                }
        		                else{
        		                    //printf("828: other pkts, ether_type = %u (expect = %u), op = %u (expect = %u)\n", ntohs(eth->ether_type), ether_type_switch, message_header->opType, OP_REPLACE_SUCCESS);
        		                }
        		            }
        		            else if(ntohs(eth->ether_type) == ether_type_fpga){
        		                CtrlHeader_fpga* message_header = (CtrlHeader_fpga*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE_CTRL);
        		                if(message_header->opType == OP_HOT_REPORT_END_FPGA && comingCount_fpga != NUM_BACKENDNODE){
        		                    comingCount_fpga++;
        		                    //printf("828: OP_HOT_REPORT_END_FPGA ok\n");
        		                    if(comingCount_fpga == NUM_BACKENDNODE){
        		                        fpga_end_time_tmp = rte_rdtsc();//-------------------------------
        		                        // printf("211:OP_HOT_REPORT time (fpga) = %f ms\n", 1.0 * (fpga_end_time_tmp - fpga_start_time_tmp) / tsc_per_ms);//-------------------------------
        		                    }
        		                }
        		                else{
        		                    //printf("828: other pkts, ether_type = %u (expect = %u), op = %u (expect = %u)\n", ntohs(eth->ether_type), ether_type_fpga, message_header->opType, OP_REPLACE_SUCCESS);
        		                }
        		            }
        		            else{
        		                CtrlHeader_switch* message_header = (CtrlHeader_switch*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE_CTRL);
        		                //printf("828: other pkts, ether_type = %u (expect = %u), op = %u (expect = %u)\n", ntohs(eth->ether_type), ether_type_switch, message_header->opType, OP_REPLACE_SUCCESS);
        		            }
        		            rte_pktmbuf_free(mbuf_rcv);
        		        }
        		    }
        		}
        		//printf("828: rcv loop ends\n");
        		end_time_tmp = rte_rdtsc();//-------------------------------
        		// printf("211:OP_HOT_REPORT time = %f ms\n", 1.0 * (end_time_tmp - start_time_tmp) / tsc_per_ms);//-------------------------------
        		start_time_tmp = rte_rdtsc();//-------------------------------
        		for(int i = 0; i < NUM_BACKENDNODE; i++){
        		    char* ip_dst_bn = ip_backendNode_arr[i];
        		    struct rte_ether_addr mac_dst_bn = mac_backendNode_arr[i];
        		    mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]);
        		    forCtrlNode_generate_ctrl_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, ether_type_fpga, OP_REPLACE_SUCCESS, topk_upd_obj_num_multiBN[i], topk_upd_obj_multiBN[i], mac_dst_bn);
        		    enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0); // enqueue_pkt_with_thres(lcore_id, mbuf, 32, 0);
        		}
        		enqueue_pkt_with_thres(lcore_id, mbuf, 1, 1);
        		//printf("828: send OP_REPLACE_SUCCESS_FPGA\n");
        		//printf("828: rcv loop starts\n");
        		comingCount_fpga = 0;
        		while(comingCount_fpga != NUM_BACKENDNODE){
        		    for (int i = 0; i < lconf->n_rx_queue; i++) {
        		        uint32_t nb_rx = rte_eth_rx_burst(lconf->port, lconf->rx_queue_list[i], mbuf_burst, NC_MAX_BURST_SIZE);
        		        for (int j = 0; j < nb_rx; j++) {
        		            mbuf_rcv = mbuf_burst[j];
        		            rte_prefetch0(rte_pktmbuf_mtod(mbuf_rcv, void *)); 
        		            struct rte_ether_hdr* eth = rte_pktmbuf_mtod(mbuf_rcv, struct rte_ether_hdr *);
        		            if(ntohs(eth->ether_type) == ether_type_fpga){
        		                CtrlHeader_fpga* message_header = (CtrlHeader_fpga*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE_CTRL);
        		                if(message_header->opType == OP_REPLACE_SUCCESS && comingCount_fpga != NUM_BACKENDNODE){
        		                    comingCount_fpga++;
        		                    //printf("828: OP_REPLACE_SUCCESS_FPGA ok\n");
        		                }
        		                else{
        		                    //printf("828: other pkts, ether_type = %u (expect = %u), op = %u (expect = %u)\n", ntohs(eth->ether_type), ether_type_fpga, message_header->opType, OP_REPLACE_SUCCESS);
        		                }
        		            }
        		            else{
        		                CtrlHeader_switch* message_header = (CtrlHeader_switch*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE_CTRL);
        		                //printf("828: other pkts, ether_type = %u (expect = %u), op = %u (expect = %u)\n", ntohs(eth->ether_type), ether_type_fpga, message_header->opType, OP_REPLACE_SUCCESS);
        		            }
        		            rte_pktmbuf_free(mbuf_rcv);
        		        }
        		    }
        		}
        		//printf("828: rcv loop ends\n");
        		end_time_tmp = rte_rdtsc();//-------------------------------
        		// printf("211:OP_REPLACE_SUCCESS time = %f ms\n", 1.0 * (end_time_tmp - start_time_tmp) / tsc_per_ms);//-------------------------------
        		epoch_idx++;
        		if(epoch_idx < ctrlPara.updEpochs){
        		    ctrlNode_stats_stage = STATS_ANALYZE;
        		}
        		else{
        		    ctrlNode_stats_stage = STATS_CLEAN;
        		}
        		end_time = rte_rdtsc();
        		update_time_total += end_time - start_time;
        		update_count += 1;
        		//printf("828: STATS_UPDATE ends\n");
        		break;}


            case STATS_CLEAN: {
                //printf("828: STATS_CLEAN starts\n");
                start_time = rte_rdtsc();

                start_time_tmp = rte_rdtsc();//-------------------------------
                for(int i = 0; i < NUM_BACKENDNODE; i++){
                    char* ip_dst_bn = ip_backendNode_arr[i];
                    struct rte_ether_addr mac_dst_bn = mac_backendNode_arr[i];
                    mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]);
                    forCtrlNode_generate_ctrl_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, ether_type_fpga, OP_CLN_HOT_COUNTER_FPGA, 0, NULL, mac_dst_bn); 
                    enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0); // enqueue_pkt_with_thres(lcore_id, mbuf, 32, 0);
                }                
                enqueue_pkt_with_thres(lcore_id, mbuf, 1, 1);

                //printf("828: send OP_CLN_HOT_COUNTER_FPGA\n");

                //printf("828: rcv loop starts\n");
                int fpga_clean_count = 0;
                while(fpga_clean_count != NUM_BACKENDNODE){
                    for (int i = 0; i < lconf->n_rx_queue; i++) {
                        uint32_t nb_rx = rte_eth_rx_burst(lconf->port, lconf->rx_queue_list[i], mbuf_burst, NC_MAX_BURST_SIZE);
                        for (int j = 0; j < nb_rx; j++) {
                            mbuf_rcv = mbuf_burst[j];
                            rte_prefetch0(rte_pktmbuf_mtod(mbuf_rcv, void *)); 
                            struct rte_ether_hdr* eth = rte_pktmbuf_mtod(mbuf_rcv, struct rte_ether_hdr *);
                            if(ntohs(eth->ether_type) == ether_type_fpga){
                                CtrlHeader_fpga* message_header = (CtrlHeader_fpga*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE_CTRL);
                                if(message_header->opType == OP_CLN_HOT_COUNTER_FPGA && fpga_clean_count != NUM_BACKENDNODE){
                                    fpga_clean_count++;
                                    //printf("828: OP_CLN_HOT_COUNTER_FPGA ok\n");
                                }
                            }
                            rte_pktmbuf_free(mbuf_rcv);
                        }
                    }
                }
                //printf("828: rcv loop ends\n");
                end_time_tmp = rte_rdtsc();//-------------------------------
                // printf("210:OP_CLN_HOT_COUNTER_FPGA time = %f ms\n", 1.0 * (end_time_tmp - start_time_tmp) / tsc_per_ms);//-------------------------------
                
                start_time_tmp = rte_rdtsc();//-------------------------------
                //printf("828: send-rcv loop starts (OP_CLN_COLD_COUNTER_SWITCH)\n");
                for(int i = 0; i < ctrlPara.cacheSize / FETCHM; i++){
                    mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]);
                    forCtrlNode_generate_ctrl_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, ether_type_switch, OP_CLN_COLD_COUNTER_SWITCH, i, NULL, mac_zero);
                    enqueue_pkt_with_thres(lcore_id, mbuf, 32, 0);
                }
                enqueue_pkt_with_thres(lcore_id, mbuf, 1, 1);
                end_time_tmp = rte_rdtsc();//-------------------------------
                // printf("210:OP_CLN_COLD_COUNTER_SWITCH time = %f ms\n", 1.0 * (end_time_tmp - start_time_tmp) / tsc_per_ms);//-------------------------------

                start_time_tmp = rte_rdtsc();//-------------------------------
                //printf("828: send-rcv loop starts (OP_CLN_COLD_COUNTER_SWITCH)\n");
                // unobj stats
                mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]); 
                forCtrlNode_generate_ctrl_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, ether_type_switch, OP_UNLOCK_STAT_SWITCH, 0, NULL, mac_zero);
                enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0);
                //printf("828: send OP_UNLOCK_STAT_SWITCH\n");

                for(int i = 0; i < NUM_BACKENDNODE; i++){
                    char* ip_dst_bn = ip_backendNode_arr[i];
                    struct rte_ether_addr mac_dst_bn = mac_backendNode_arr[i];
                    mbuf = rte_pktmbuf_alloc(pktmbuf_pool[rx_queue_id]);
                    forCtrlNode_generate_ctrl_pkt(rx_queue_id, header_template_local, lcore_id, mbuf, ether_type_fpga, OP_UNLOCK_STAT_FPGA, 0, NULL, mac_dst_bn);                    
                    enqueue_pkt_with_thres(lcore_id, mbuf, 1, 0); // enqueue_pkt_with_thres(lcore_id, mbuf, 32, 0);
                }
                enqueue_pkt_with_thres(lcore_id, mbuf, 1, 1);
                //printf("828: send OP_UNLOCK_STAT_FPGA\n");

                //printf("828: rcv loop starts\n");
                int comingFlag_switch = 0;
                int comingCount_fpga = 0;
                while(comingFlag_switch == 0 || comingCount_fpga != NUM_BACKENDNODE){
                    for (int i = 0; i < lconf->n_rx_queue; i++) {
                        uint32_t nb_rx = rte_eth_rx_burst(lconf->port, lconf->rx_queue_list[i], mbuf_burst, NC_MAX_BURST_SIZE);
                        for (int j = 0; j < nb_rx; j++) {
                            mbuf_rcv = mbuf_burst[j];
                            rte_prefetch0(rte_pktmbuf_mtod(mbuf_rcv, void *)); 
                            struct rte_ether_hdr* eth = rte_pktmbuf_mtod(mbuf_rcv, struct rte_ether_hdr *);
                            // printf("829: stage = %d, ntohs(eth->ether_type) = %x\n", ctrlNode_stats_stage, ntohs(eth->ether_type));
                            if(ntohs(eth->ether_type) == ether_type_switch){
                                CtrlHeader_switch* message_header = (CtrlHeader_switch*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE_CTRL);
                                // printf("829: stage = %d, ntohs(eth->ether_type) = %x, message_header->opType = %d\n", ctrlNode_stats_stage, ntohs(eth->ether_type), message_header->opType);
                                
                                if(message_header->opType == OP_UNLOCK_STAT_SWITCH && comingFlag_switch == 0){
                                    comingFlag_switch = 1; 
                                    //printf("828: OP_UNLOCK_STAT_SWITCH ok\n");
                                }
                                else{
                                    //printf("828: other pkts\n");
                                }
                            }
                            else if(ntohs(eth->ether_type) == ether_type_fpga){
                                CtrlHeader_fpga* message_header = (CtrlHeader_fpga*) ((uint8_t *) eth + HEADER_TEMPLATE_SIZE_CTRL);
                                // printf("829: stage = %d, ntohs(eth->ether_type) = %x, message_header->opType = %d\n", ctrlNode_stats_stage, ntohs(eth->ether_type), message_header->opType);
                                
                                if(message_header->opType == OP_UNLOCK_STAT_FPGA && comingCount_fpga != NUM_BACKENDNODE){
                                    comingCount_fpga++;
                                    //printf("828: OP_UNLOCK_STAT_FPGA ok\n");
                                }
                                else{
                                    //printf("828: other pkts\n");
                                }
                            }
                            else{
                                //printf("828: other pkts\n");
                            }
                            rte_pktmbuf_free(mbuf_rcv);
                        }
                    }
                }
                //printf("828: rcv loop ends\n");
                end_time_tmp = rte_rdtsc();//-------------------------------
                // printf("210:OP_UNLOCK_STAT time = %f ms\n", 1.0 * (end_time_tmp - start_time_tmp) / tsc_per_ms);//-------------------------------


                ctrlNode_stats_stage = STATS_COLLECT;
                cur_tsc = rte_rdtsc();
                update_tsc = cur_tsc + rte_get_tsc_hz() / 1000 * ctrlPara.waitTime;

                end_time = rte_rdtsc(); 
                clean_time_total += end_time - start_time;
                clean_count += 1;
                //printf("828: STATS_CLEAN ends\n");
                break;}
		}
				
	}
	if(debug) file.close(); 
}

static int32_t user_loop(void *arg) {
    uint32_t lcore_id = rte_lcore_id();
	if(lcore_id == 0){
		nic_0_func(lcore_id, *(P4Program *)(arg));
	}
	else{
		printf("too many lcores! (only allow -l 0)\n");
        exit(0);
	}
}

void data_plane_init(P4Program &program, int argc, char **argv){
	string program_name;
    // config mac forwarding
	{
		cout << "Initiating mac forward table..." << endl;

		auto &forward = program.getTable("Ingress.mac_forward");

		auto dst_addr_key_id = forward.getKey("hdr.ethernet.dst_addr");
		auto send_action_id = forward.getAction("Ingress.send");
		auto drop_action_id = forward.getAction("Ingress.drop");
		auto send_action_port_data_id = forward.getData("port", "Ingress.send");
		auto multicast_action_id = forward.getAction("Ingress.multicast");
		auto multicast_action_port_data_id = forward.getData("mcast_grp", "Ingress.multicast");

		// 11/-�����˿ڣ� cpu control
		forward.keyHandlerReset();
		forward.keyHandlerSetValue(dst_addr_key_id, 0xb8599fe96b1c);
		forward.dataHandlerReset(send_action_id);
		forward.dataHandlerSetValue(send_action_port_data_id, (uint64_t) 40); // 11/-
		forward.tableEntryAdd();

		// 12/-�����˿ڣ� cpu control
		forward.keyHandlerReset();
		forward.keyHandlerSetValue(dst_addr_key_id, 0x98039bca4838);
		forward.dataHandlerReset(send_action_id);
		forward.dataHandlerSetValue(send_action_port_data_id, (uint64_t) 32); // 12/-
		forward.tableEntryAdd();

		// 13/-�����˿ڣ� cpu control
		forward.keyHandlerReset();
		forward.keyHandlerSetValue(dst_addr_key_id, 0x98039bc7c818);
		forward.dataHandlerReset(send_action_id);
		forward.dataHandlerSetValue(send_action_port_data_id, (uint64_t) 24); // 13/-
		forward.tableEntryAdd();

		// 14/-�����˿ڣ� cpu control
		forward.keyHandlerReset();
		forward.keyHandlerSetValue(dst_addr_key_id, 0x98039bc7c0a8);
		forward.dataHandlerReset(send_action_id);
		forward.dataHandlerSetValue(send_action_port_data_id, (uint64_t) 16); // 14/-
		forward.tableEntryAdd();

		// 15/-�����˿ڣ� cpu control
		forward.keyHandlerReset();
		forward.keyHandlerSetValue(dst_addr_key_id, 0x043f72deba44);
		forward.dataHandlerReset(send_action_id);
		forward.dataHandlerSetValue(send_action_port_data_id, (uint64_t) 8); // 15/-
		forward.tableEntryAdd();

		// 16/- cpu control
		forward.keyHandlerReset();
		forward.keyHandlerSetValue(dst_addr_key_id, 0x98039bc7c7fc);
		forward.dataHandlerReset(send_action_id);
		forward.dataHandlerSetValue(send_action_port_data_id, (uint64_t) 0); // 16/-
		forward.tableEntryAdd();

		// 17/- cpu control
		forward.keyHandlerReset();
		forward.keyHandlerSetValue(dst_addr_key_id, 0x98039bca4018);
		forward.dataHandlerReset(send_action_id);//forward.dataHandlerReset(drop_action_id);//
		forward.dataHandlerSetValue(send_action_port_data_id, (uint64_t) 4); // 17/-
		forward.tableEntryAdd();

		// 18/- cpu control
		forward.keyHandlerReset();
		forward.keyHandlerSetValue(dst_addr_key_id, 0x0c42a12b0d70);
		forward.dataHandlerReset(send_action_id);
		forward.dataHandlerSetValue(send_action_port_data_id, (uint64_t) 12); // 18/-
		forward.tableEntryAdd();

		// 20/-�����˿ڣ� cpu control
		forward.keyHandlerReset();
		forward.keyHandlerSetValue(dst_addr_key_id, 0x98039bCA4008);
		forward.dataHandlerReset(send_action_id);
		forward.dataHandlerSetValue(send_action_port_data_id, (uint64_t) 28); // 20/-
		forward.tableEntryAdd();

		// 33/0�����˿ڣ� cpu control
		forward.keyHandlerReset();
		forward.keyHandlerSetValue(dst_addr_key_id, 0x0090fb7063c7); 
		forward.dataHandlerReset(send_action_id);
		// forward.dataHandlerSetValue(send_action_port_data_id, (uint64_t) 66); // ����㷴
		forward.dataHandlerSetValue(send_action_port_data_id, (uint64_t) 64); // 33/0
		forward.tableEntryAdd();

		// 33/2�����˿ڣ� cpu control
		forward.keyHandlerReset();
		forward.keyHandlerSetValue(dst_addr_key_id, 0x0090fb7063c6); 
		forward.dataHandlerReset(send_action_id);
		// forward.dataHandlerSetValue(send_action_port_data_id, (uint64_t) 64); // 
		forward.dataHandlerSetValue(send_action_port_data_id, (uint64_t) 66); // 33/2
		forward.tableEntryAdd();

		//multicast
		forward.keyHandlerReset();
		forward.keyHandlerSetValue(dst_addr_key_id, 0xffffffffffff);
		forward.dataHandlerReset(multicast_action_id);
		forward.dataHandlerSetValue(multicast_action_port_data_id, (uint64_t) 1); // 18/-
		forward.tableEntryAdd();
	}

    // config ports and mirroring
	{
		cout << "Initiating ports..." << endl;

		// client��backend node��control node
		program.portAdd("11/0", BF_SPEED_100G, BF_FEC_TYP_RS); //40
	    program.portAdd("12/0", BF_SPEED_100G, BF_FEC_TYP_RS); //32
	    program.portAdd("13/0", BF_SPEED_100G, BF_FEC_TYP_RS); //24
	    program.portAdd("14/0", BF_SPEED_100G, BF_FEC_TYP_RS); //16
	    program.portAdd("15/0", BF_SPEED_100G, BF_FEC_TYP_RS); //8
	    // program.portAdd("16/0", BF_SPEED_100G, BF_FEC_TYP_RS); //0
	    program.portAdd("17/0", BF_SPEED_100G, BF_FEC_TYP_RS); //4
	    program.portAdd("18/0", BF_SPEED_100G, BF_FEC_TYP_RS); //12
		program.portAdd("20/0", BF_SPEED_100G, BF_FEC_TYP_RS); //28
		program.portAdd("33/0", BF_SPEED_10G, BF_FEC_TYP_NONE); //64
		program.portAdd("33/2", BF_SPEED_10G, BF_FEC_TYP_NONE); //66

		//multicast
		program.configMulticast(1, 11, {40, 32, 24, 16, 8, 0, 4, 12, 28});
	}
}

int main(int argc, char **argv) {
	int ret = nc_init(argc, argv);
	argc -= ret;
	argv += ret;

	P4Program program("P4Lock");
	data_plane_init(program, argc, argv);
	sleep(10);

    uint32_t lcore_id;
    rte_eal_mp_remote_launch(user_loop, (void *)(&program), CALL_MASTER);
    RTE_LCORE_FOREACH_SLAVE(lcore_id) {
        if (rte_eal_wait_lcore(lcore_id) < 0) {
            break;
        } 
    }
}