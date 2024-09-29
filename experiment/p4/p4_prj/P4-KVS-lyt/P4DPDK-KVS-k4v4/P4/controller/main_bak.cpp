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

using namespace std;

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

	// begin sniff
	cout << "-----------------------------------\n"
	     << "Service begin" << endl;

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

	char pkt_data[1024];
	struct timeval start_ini,finish_ini;
	struct timeval quit_start_ini,quit_finish_ini;
	long timeuse_ini;

	gettimeofday(&quit_start_ini, NULL );
	gettimeofday(&quit_finish_ini, NULL );
	uint32_t pkt_size = sizeof(LOCK_CTL_HEADER) + 14;
	printf("pkt_size = %d\n", pkt_size);
	while(true){
		timeuse_ini = 1000000 * ( quit_finish_ini.tv_sec - quit_start_ini.tv_sec ) + quit_finish_ini.tv_usec - quit_start_ini.tv_usec;
		if(timeuse_ini > 30000000){
			uint32_t quit_flag = 0;
			cout << "No packet comes within 20s, input 1 to quit, input 0 to wait:\n";
			cin >> quit_flag;
			if(quit_flag == 1){
				exit(1);
			}
			else{
				gettimeofday(&quit_start_ini, NULL );
			}
		}

		uint32_t rcv_num = rcv_pkt_seq(lcore_id, pkt_data, pkt_size);
		uint32_t continue_flag = 1;
		for(int i = 0; i < rcv_num; i++){
			auto *lock_ctl_header = (LOCK_CTL_HEADER *) (pkt_data + pkt_size * i + 14);
			if(lock_ctl_header->op == HOT_REPORT){
				rte_memcpy(pkt_data, pkt_data + pkt_size * i, pkt_size);
				continue_flag = 0;
			}
		}
		// printf("continue_flag = %d\n", continue_flag);
		if(continue_flag == 1){
			gettimeofday(&quit_finish_ini, NULL );
			timeuse_ini = 1000000 * ( quit_finish_ini.tv_sec - quit_start_ini.tv_sec ) + quit_finish_ini.tv_usec - quit_start_ini.tv_usec;
			// printf("No packet comes for %f us\n", timeuse_ini * 1.0);
			continue;
		}
		else{
			gettimeofday(&quit_start_ini, NULL );
		}

		gettimeofday(&start_ini, NULL ); //-----------------------------
		auto *lock_ctl_header = (LOCK_CTL_HEADER *) (pkt_data + 14);
		if(lock_ctl_header->op == HOT_REPORT){
			int n = lock_ctl_header->replace_num;
			int e = 0;

			if(debug) file << "Receive " << n << " hot lock" << endl;

			// program.beginBatch();
			for(int i = 0; i < n && i < TOPK; ++i)
			{
				uint64_t id = ntohl(lock_ctl_header->id[i]);
				uint64_t index = ntohl(lock_ctl_header->index[i]);
				uint64_t counter = 0xffffffff;//ntohl(lock_ctl_header->counter[i]);

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
//				    else if(reverse_mapping.size() < SLOT_SIZE)
//				    {
//				    	index = reverse_mapping.size();
//				    }

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
				// 可能需要放到循环外面单独batch（根据论文设计的时间点）
				suspend.keyHandlerReset();
				suspend.keyHandlerSetValue(suspend_index_id, index);
				suspend.dataHandlerReset();
				suspend.dataHandlerSetValue(suspend_data_id, (uint64_t) 0);
				suspend.tableEntryModify();
				suspend.completeOperations();
			}
			// program.endBatch(true);

			if(debug) file << "checkpoint 10" << endl;

			auto *ether_header = (ETH_HEADER *) pkt_data;
			swap(ether_header->DestMac, ether_header->SrcMac);
			lock_ctl_header->op = REPLACE_SUCC;

			// cout << "print one letter to send response:" << endl;
			// getchar();

			send_pkt_seq(lcore_id, pkt_data, sizeof(LOCK_CTL_HEADER) + 14);
			if(debug) cout << "Successfully handle " << n - e << " hot replace request" << endl;
			
			gettimeofday(&finish_ini, NULL); //----------------------------
			timeuse_ini = 1000000 * ( finish_ini.tv_sec - start_ini.tv_sec ) + finish_ini.tv_usec - start_ini.tv_usec;
			if(debug2) printf("TOPK(gettimeofday) %f\n", timeuse_ini * 1.0);
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
	int size = 10000;

	// parse option
	// {
	// 	int o;
	// 	while((o = getopt(argc, argv, "s:")) != -1){
	// 		switch(o){
	// 			case 's':
	// 				size = stoi(optarg);
	// 				break;
	// 			default:
	// 				printf("parameter error!\n");
	// 				exit(0);
	// 		}
	// 	}
	// 	if(program_name.empty()){
	// 		printf("program name empty!\n");
	// 		exit(0);
	// 	}
	// }

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

		// 11/-（面板端口） cpu control
		forward.keyHandlerReset();
		forward.keyHandlerSetValue(dst_addr_key_id, 0xb8599fe96b1c);
		forward.dataHandlerReset(send_action_id);
		forward.dataHandlerSetValue(send_action_port_data_id, (uint64_t) 40); // 11/-
		forward.tableEntryAdd();

		// 12/-（面板端口） cpu control
		forward.keyHandlerReset();
		forward.keyHandlerSetValue(dst_addr_key_id, 0x98039bca4838);
		forward.dataHandlerReset(send_action_id);
		forward.dataHandlerSetValue(send_action_port_data_id, (uint64_t) 32); // 12/-
		forward.tableEntryAdd();

		// 13/-（面板端口） cpu control
		forward.keyHandlerReset();
		forward.keyHandlerSetValue(dst_addr_key_id, 0x98039bc7c818);
		forward.dataHandlerReset(send_action_id);
		forward.dataHandlerSetValue(send_action_port_data_id, (uint64_t) 24); // 13/-
		forward.tableEntryAdd();

		// 14/-（面板端口） cpu control
		forward.keyHandlerReset();
		forward.keyHandlerSetValue(dst_addr_key_id, 0x98039bc7c0a8);
		forward.dataHandlerReset(send_action_id);
		forward.dataHandlerSetValue(send_action_port_data_id, (uint64_t) 16); // 14/-
		forward.tableEntryAdd();

		// 15/-（面板端口） cpu control
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

		// 20/-（面板端口） cpu control
		forward.keyHandlerReset();
		forward.keyHandlerSetValue(dst_addr_key_id, 0x98039bCA4008);
		forward.dataHandlerReset(send_action_id);
		forward.dataHandlerSetValue(send_action_port_data_id, (uint64_t) 28); // 20/-
		forward.tableEntryAdd();

		// 33/0（面板端口） cpu control
		forward.keyHandlerReset();
		forward.keyHandlerSetValue(dst_addr_key_id, 0x0090fb7063c7); 
		forward.dataHandlerReset(send_action_id);
		// forward.dataHandlerSetValue(send_action_port_data_id, (uint64_t) 66); // 故意搞反
		forward.dataHandlerSetValue(send_action_port_data_id, (uint64_t) 64); // 33/0
		forward.tableEntryAdd();

		// 33/2（面板端口） cpu control
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

		// client、backend node、control node
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