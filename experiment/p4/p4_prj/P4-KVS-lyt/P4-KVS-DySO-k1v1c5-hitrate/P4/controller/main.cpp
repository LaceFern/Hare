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
//my code
#include <sys/time.h>
#include <random>

#include "p4_program.h"
#include "p4_table.h"
#include "./include_dpdk/dpdk_program.h"

#include "config.h"
#include "my_header.h"
#include "my_spacesaving.h"

using namespace std;

uint8_t debug_flag = 0;
uint8_t hitrate_flag = 0;


/****************** c o r e - f u n c t i o n ******************/
static void nic_0_func(uint32_t lcore_id, P4Program &program) {
	cout << "-----------------------------------\n"
	     << "nic_0_func begin" << endl;



	/****************** r e c o r d - h o t s p o t s ******************/
	ifstream fin_hotspots;
	ofstream fout_hotspots;
	ofstream fout_hotspots_4ini;
	ofstream fout_hotspots_4check;
	fin_hotspots.open("/home/zxy/p4_prj/P4-KVS-lyt/P4-KVS-DySO-k1v1c5-hitrate/P4/no.txt",ios::in);//c32k_last_hotspots_32k.txt
	fout_hotspots.open("/home/zxy/p4_prj/P4-KVS-lyt/P4-KVS-DySO-k1v1c5-hitrate/P4/hotspots.txt",ios::out);
	fout_hotspots_4ini.open("/home/zxy/p4_prj/P4-KVS-lyt/P4-KVS-DySO-k1v1c5-hitrate/P4/hotspots_4ini.txt",ios::out);
	fout_hotspots_4check.open("/home/zxy/p4_prj/P4-KVS-lyt/P4-KVS-DySO-k1v1c5-hitrate/P4/hotspots_4check.txt",ios::out);


    double zipfPara_a = 0.99;
	double zipfPara_n = 1000000;
    double reciPowSum_res = reciPowSum(zipfPara_a, zipfPara_n);
	double* freq_arr = (double*)malloc(sizeof(double) * zipfPara_n * 2);
	uint8_t* state_arr = (uint8_t*)malloc(sizeof(uint8_t) * zipfPara_n * 2);
	uint32_t* count_arr = (uint32_t*)malloc(sizeof(uint32_t) * zipfPara_n * 2);
    for(uint32_t i = 0; i < zipfPara_n; i++){
        uint32_t x = i + 1;
        double tmpFreq = 1 / (pow(x, zipfPara_a) * reciPowSum_res); 
        freq_arr[i] = tmpFreq;
		// printf("freq_arr[%d] = %lf\n", i, tmpFreq);
		// getchar();
		state_arr[i] = 0;
		count_arr[i] = 0;
    }
	double true_offloaded_count = 0;
	double hit_rate_total = 0;	

	char pkt_data_rcv[65536];
    char pkt_data_send[65536];
	struct timeval fresh_start_ini,fresh_finish_ini;
	struct timeval quit_start_ini,quit_finish_ini;
	struct timeval print_start_ini,print_finish_ini;
	long timeuse_ini, timeuse_fresh, timeuse_print;
	uint32_t stats_pkt_count = 0;
	uint32_t modify_pkt_count = 0;
	gettimeofday(&quit_start_ini, NULL );
	gettimeofday(&quit_finish_ini, NULL );
	gettimeofday(&fresh_start_ini, NULL );
	gettimeofday(&fresh_finish_ini, NULL );
	uint32_t pkt_size = sizeof(STATS_HEADER);
	printf("pkt_size = %d\n", pkt_size);

	struct timeval offload_start_ini,offload_finish_ini;
	long timeuse_offload;
	uint8_t start_flag = 0;
	uint32_t offload_total = 0;

	Space_saving* SP = new Space_saving();
	// SP->space_saving_init(2500, 4, 64);
	SP->space_saving_init(KEY_ROWNUM, KEY_COLNUM, 64);
	// if(debug_flag) printf("checkpoint -2\n");

	cout << "-----------------------------------\n"
	     << "r-mat initial (input 0 to begin)" << endl;
	int tmp = 0;
	scanf("%d", &tmp);
	cout << "-----------------------------------\n"
	     << "r-mat initial begin" << endl;
	auto *ctrl_header = (CTRL_HEADER *) (pkt_data_send);
	ctrl_header->hdr_eth.Etype[0] = 0x23;
	ctrl_header->hdr_eth.Etype[1] = 0x33;
	for(uint32_t row_idx = 0; row_idx < KEY_ROWNUM; row_idx++){
		ctrl_header->hdr_kv_ctrl.hash_index = htonl(row_idx);
		for(uint32_t col_idx = 0; col_idx < KEY_COLNUM_HW; col_idx++){
    		for(uint32_t byte_idx = 0; byte_idx < KEY_BYTES; byte_idx++){
    		    ctrl_header->hdr_kv_ctrl.key[col_idx][KEY_BYTES - 1 - byte_idx] = 0xff;
    		}
    		for(uint32_t byte_idx = 0; byte_idx < VALUE_BYTES; byte_idx++){
				ctrl_header->hdr_kv_ctrl.value[col_idx][VALUE_BYTES - 1 - byte_idx] = 0xff;
    		}
			printf("send pkt!\n");
			send_pkt_seq(lcore_id, pkt_data_send, sizeof(CTRL_HEADER));
		}
	}

	/****************** r e c o r d - h o t s p o t s ******************/
	if(fin_hotspots.is_open()){
		cout << "-----------------------------------\n"
		     << "r-mat initial with last_hotspots(input 0 to begin)" << endl;
		int tmp = 0;
		scanf("%d", &tmp);
		cout << "-----------------------------------\n"
		     << "r-mat initial with last_hotspots begin" << endl;

		for(uint32_t row_idx = 0; row_idx < KEY_ROWNUM; row_idx++){

			ctrl_header->hdr_kv_ctrl.hash_index = htonl(row_idx);
			for(uint32_t col_idx = 0; col_idx < KEY_COLNUM; col_idx++){

				uint32_t key;
				fin_hotspots >> key;			

        		uint8_t key_tmp[KEY_BYTES] = {0};
        		key_tmp[0] = key & 0xff;
        		key_tmp[1] = (key >> 8) & 0xff;
        		key_tmp[2] = (key >> 16) & 0xff;
        		key_tmp[3] = (key >> 24) & 0xff;

    			for(uint32_t byte_idx = 0; byte_idx < KEY_BYTES; byte_idx++){
    			    ctrl_header->hdr_kv_ctrl.key[col_idx][KEY_BYTES - 1 - byte_idx] = key_tmp[byte_idx];
    			}
    			for(uint32_t byte_idx = 0; byte_idx < VALUE_BYTES; byte_idx++){
					ctrl_header->hdr_kv_ctrl.value[col_idx][VALUE_BYTES - 1 - byte_idx] = 0xff;
    			}
				printf("send pkt! row_idx = %d, col_idx = %d\n", row_idx, col_idx);
				send_pkt_seq(lcore_id, pkt_data_send, sizeof(CTRL_HEADER));

				SP->obj4col_arr[row_idx][col_idx] = key;
				if(key != 0xffffffff){
					hit_rate_total += freq_arr[key];
					state_arr[key] = 1;
					if(key < KEY_COLNUM * KEY_ROWNUM){
						true_offloaded_count++;
					}
				}
			}
		}
		fout_hotspots << "hit_rate_ini:" << "\t" << hit_rate_total << "\tmiss_ratio_ini:" << "\t" << 1 - true_offloaded_count/(KEY_COLNUM * KEY_ROWNUM) << endl;
	}
	fout_hotspots << "key_insert" << "\t" << "key_delete" << "\t" << "hit_rate" << "\t" << "miss_ratio" << endl;
	

	cout << "-----------------------------------\n"
	     << "Service begin (input 0 to begin)" << endl;
	tmp = 0;
	scanf("%d", &tmp);
	cout << "-----------------------------------\n"
	     << "Service begin" << endl;
	while(true){
		timeuse_ini = 1000000 * ( quit_finish_ini.tv_sec - quit_start_ini.tv_sec ) + quit_finish_ini.tv_usec - quit_start_ini.tv_usec;
		if(timeuse_ini > 30000000){
			uint32_t quit_flag = 0;
			cout << "No packet comes within 30s, input 0 to wait, input others to quit:\n";
			cin >> quit_flag;
			if(quit_flag == 0){
				gettimeofday(&quit_start_ini, NULL );
			}
			else{
				/****************** r e c o r d - h o t s p o t s ******************/
				timeuse_offload = 1000000 * ( offload_finish_ini.tv_sec - offload_start_ini.tv_sec ) + offload_finish_ini.tv_usec - offload_start_ini.tv_usec;
				cout << "Number of offload objects = " << offload_total << endl;
				cout << "Time (us) to offload objects = " << timeuse_offload << endl;
				cout << "Average time (us) to offload objects = " << timeuse_offload / offload_total << endl;
				fout_hotspots.close();
				for(uint32_t row_idx = 0; row_idx < KEY_ROWNUM; row_idx++){
					for(uint32_t col_idx = 0; col_idx < KEY_COLNUM; col_idx++){
						uint32_t key = SP->space_saving_getcol(row_idx, col_idx);
						fout_hotspots_4ini << key << endl;
						if(key != 0xffffffff){
							fout_hotspots_4check << key << "\t" << freq_arr[key] << endl;
						}
						else{
							fout_hotspots_4check << key << endl;
						}
					}
				}	
				fout_hotspots_4ini.close();
				fout_hotspots_4check.close();
				exit(1);
			}
		}

		gettimeofday(&print_finish_ini, NULL );
		timeuse_print = 1000000 * ( print_finish_ini.tv_sec - print_start_ini.tv_sec ) + print_finish_ini.tv_usec - print_start_ini.tv_usec;
		if(timeuse_print > 1000000){
			gettimeofday(&print_start_ini, NULL );
			// printf("stats_pkt_count = %d Mpps, modify_pkt_count = %d Mpps\n", stats_pkt_count / 10000000, modify_pkt_count / 10000000);
			printf("stats_pkt_count = %d pps, modify_pkt_count = %d pps\n", stats_pkt_count, modify_pkt_count);
			stats_pkt_count = 0;
			modify_pkt_count = 0;
		}

		gettimeofday(&fresh_finish_ini, NULL );
		timeuse_fresh = 1000000 * ( fresh_finish_ini.tv_sec - fresh_start_ini.tv_sec ) + fresh_finish_ini.tv_usec - fresh_start_ini.tv_usec;
		if(timeuse_fresh > 1000000){
			// printf("im here!!!!\n");
			SP->space_saving_fresh(0.5); 
			gettimeofday(&fresh_start_ini, NULL );
		}
		// if(timeuse_fresh > 50000000){
		// 	// printf("im here!!!!\n");
		// 	SP->space_saving_fresh(0.5); 
		// 	gettimeofday(&fresh_start_ini, NULL );
		// }

		uint32_t continue_flag = 1;
		// if(debug_flag) printf("checkpoint -1\n");
        uint32_t rcv_num = rcv_pkt_seq(lcore_id, pkt_data_rcv, pkt_size);
		for(int i = 0; i < rcv_num; i++){

			// if(debug_flag) printf("checkpoint 0: i = %d\n", i);
			auto *stats_header = (STATS_HEADER *) (pkt_data_rcv + pkt_size * i);
			// printf("stats_header->hdr_kv_query.op = %d, OP_GET = %d, if equal = %d\n",stats_header->hdr_kv_query.op, OP_GET, stats_header->hdr_kv_query.op == OP_GET);
			if(stats_header->hdr_kv_query.op == OP_GET){
				stats_pkt_count++;
				// if(debug_flag) printf("checkpoint 1\n");
				// �����key����src_port�е���������һ��space saving������
                uint8_t key_tmp[KEY_BYTES] = {0};
				// if(debug_flag) printf("checkpoint 1.0.1\n");
                for(uint32_t byte_idx = 0; byte_idx < KEY_BYTES; byte_idx++){
					// if(debug_flag) printf("checkpoint 1.0.2: byte_idx = %d\n", byte_idx);
                    key_tmp[byte_idx] = stats_header->hdr_kv_query.key[KEY_BYTES - 1 - byte_idx];
                }                    
				// if(debug_flag) printf("checkpoint 1.1\n");
                uint32_t key = ((uint32_t)key_tmp[0]) | ((uint32_t)key_tmp[1]) << 8 | ((uint32_t)key_tmp[2]) << 16 | ((uint32_t)key_tmp[3]) << 24;

			
				//debug
				if(key != 4500000){
					count_arr[key]++;
				}
				/****************** c o u n t - s c a l e ******************/
				
				if(count_arr[key] == 3){
					count_arr[key] = 0;
				}
				else{
					continue_flag = 0;
					continue;
				}


				// if(debug_flag) printf("checkpoint 1.2\n");
				uint32_t hash_index = ntohs(stats_header->hdr_udp.src_port);
				// if(debug_flag) printf("checkpoint 1.3: hash_index = %d\n", hash_index);
				uint8_t col_change_flag = SP->space_saving_update(hash_index, key); 
				// if(debug_flag) printf("checkpoint 2\n");
				// ����������ǰKEY_COLNUM�������ֵ�Ƿ�����??�������??�ͷ�һЩ���ư�ȥ�޸������������??
				// if(debug_flag) printf("checkpoint 2: hash_index = %d, col_change_flag = %d, key = %d\n", hash_index, col_change_flag, key);
				if(col_change_flag == 1){

					/****************** r e c o r d - h o t s p o t s ******************/
					uint32_t key_insert = key;
					uint32_t key_delete = SP->space_saving_getKeyDeleted(hash_index);
					uint8_t replace_valid_flag = 0;
					if(state_arr[key_insert] == 0){
						hit_rate_total += freq_arr[key_insert];
						state_arr[key_insert] = 1;
						if(key_insert < KEY_COLNUM * KEY_ROWNUM){
							true_offloaded_count++;
						}
						replace_valid_flag = 1;
					}
					if(key_delete != 0xffffffff && state_arr[key_delete] == 1){
						hit_rate_total -= freq_arr[key_delete];
						state_arr[key_delete] = 0;
						if(key_delete < KEY_COLNUM * KEY_ROWNUM){
							true_offloaded_count--;
						}
					} 

					if(replace_valid_flag == 1){
						if(start_flag == 0){
							start_flag = 1;
							gettimeofday(&offload_start_ini, NULL );
							gettimeofday(&offload_finish_ini, NULL );
						}
						offload_total++;
						gettimeofday(&offload_finish_ini, NULL );
						if(hitrate_flag == 1) fout_hotspots << key_insert << "\t" << key_delete << "\t" << hit_rate_total << "\t" << 1 - true_offloaded_count/(KEY_COLNUM * KEY_ROWNUM) << endl;
					}
					
					


					auto *ctrl_header = (CTRL_HEADER *) (pkt_data_send);
					ctrl_header->hdr_eth.Etype[0] = 0x23;
					ctrl_header->hdr_eth.Etype[1] = 0x33;
					
					for(uint32_t col_idx = 0; col_idx < KEY_COLNUM; col_idx++){
						uint32_t key = SP->space_saving_getcol(hash_index, col_idx);
						// if(debug_flag) printf("checkpoint 3: hash_index = %d, col_idx = %d, key = %d\n", hash_index, col_idx, key);
        				uint8_t key_tmp[KEY_BYTES] = {0};
        				key_tmp[0] = key & 0xff;
        				key_tmp[1] = (key >> 8) & 0xff;
        				key_tmp[2] = (key >> 16) & 0xff;
        				key_tmp[3] = (key >> 24) & 0xff;
        				for(uint32_t byte_idx = 0; byte_idx < KEY_BYTES; byte_idx++){
        				    ctrl_header->hdr_kv_ctrl.key[col_idx][KEY_BYTES - 1 - byte_idx] = key_tmp[byte_idx];
        				}
					}

					for(uint32_t col_idx = KEY_COLNUM; col_idx < KEY_COLNUM_HW; col_idx++){
						uint32_t key = 0xffffffff;
        				uint8_t key_tmp[KEY_BYTES] = {0};
        				key_tmp[0] = key & 0xff;
        				key_tmp[1] = (key >> 8) & 0xff;
        				key_tmp[2] = (key >> 16) & 0xff;
        				key_tmp[3] = (key >> 24) & 0xff;
        				for(uint32_t byte_idx = 0; byte_idx < KEY_BYTES; byte_idx++){
        				    ctrl_header->hdr_kv_ctrl.key[col_idx][KEY_BYTES - 1 - byte_idx] = key_tmp[byte_idx];
        				}
					}

					ctrl_header->hdr_kv_ctrl.hash_index = htonl(hash_index);
					send_pkt_seq(lcore_id, pkt_data_send, sizeof(CTRL_HEADER));
					modify_pkt_count++;
				}
				continue_flag = 0;
			}
		}

		if(continue_flag == 1){
			gettimeofday(&quit_finish_ini, NULL );
			timeuse_ini = 1000000 * ( quit_finish_ini.tv_sec - quit_start_ini.tv_sec ) + quit_finish_ini.tv_usec - quit_start_ini.tv_usec;
			continue;
		}
		else{
			gettimeofday(&quit_start_ini, NULL );
		}
	}
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
	int size = 10000;

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
		// forward.dataHandlerSetValue(send_action_port_data_id, (uint64_t) 66); // �����??
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
		program.configMulticast(3, 33, {66}); //control node
	}
}

int main(int argc, char **argv) {
	int ret = nc_init(argc, argv);
	argc -= ret;
	argv += ret;

	P4Program program("P4DySO");
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