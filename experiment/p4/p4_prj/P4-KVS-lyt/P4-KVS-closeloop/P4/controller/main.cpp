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

using namespace std;

void usage(){
	const string basename = "hello_bfrt";
	cout << "USAGE: << " << basename << " {-p <...> | -c <...>} [OPTIONS -- SWITCHD_OPTIONS]" << '\n'
	     << "Options for running switchd:" << '\n'
	     << "  -p <p4_program_name>" << '\n'
	     << "    Load driver with artifacts associated with P4 program" << '\n'
	     << "  -c TARGET_CONFIG_FILE" << '\n'
	     << "    TARGET_CONFIG_FILE that describes P4 artifacts of the device" << '\n'
	     << "  -r REDIRECTLOG" << '\n'
	     << "    logfile to redirect" << '\n'
	     << "  -C" << '\n'
	     << "    Start CLI immediately" << '\n'
	     << "  --skip-p4" << '\n'
	     << "    Skip loading of P4 program in device" << '\n'
	     << "  --skip-hld <skip_hld_mgr_list>" << '\n'
	     << "    Skip high level drivers:" << '\n'
	     << "    p:pipe_mgr, m:mc_mgr, k:pkt_mgr, r:port_mgr, t:traffic_mgr" << '\n'
	     << "  --skip-port-add" << '\n'
	     << "    Skip adding ports" << '\n'
	     << "  --kernel-pkt" << '\n'
	     << "    use kernel space packet processing" << '\n'
	     << "  -h" << '\n'
	     << "    Print this message" << '\n'
	     << "  -g" << '\n'
	     << "    Run with gdb" << '\n'
	     << "  --gdb-server" << '\n'
	     << "    Run with gdbserver; Listening on port 12345 " << '\n'
	     << "  --no-status-srv" << '\n'
	     << "    Do not start bf_switchd's status server" << '\n'
	     << "  --status-port <port number>" << '\n'
	     << "    Specify the port that bf_switchd's status server will use; the default is 7777" << '\n'
	     << "  -s" << '\n'
	     << "    Don't stop on the first error when running under the address sanitizer" << '\n'
	     << "  --init-mode <cold|fastreconfig>" << '\n'
	     << "    Specify if devices should be cold booted or under go fast reconfig" << '\n'
	     << "  --arch <Tofino|Tofino2>" << '\n'
	     << "    Specify the chip architecture, defaults to Tofino" << '\n'
	     << "  --server-listen-local-only" << '\n'
	     << "    Servers can only be connected from localhost" << endl;
}


int main(int argc, char **argv)
{
	string program_name;
	int size = 10000;

	// parse option
	{
		int o;
		while((o = getopt(argc, argv, "p:s:")) != -1)
		{
			switch(o)
			{
				case 'p':
					program_name = optarg;
					break;
				case 's':
					size = stoi(optarg);
					break;
				case '?':
					usage();
					return 0;
				default:
					return 0;
			}
		}

		if(program_name.empty())
		{
			usage();
			return 0;
		}
	}

	P4Program program(program_name);

    // config ip/arp forwarding 不需要交换机来实现对不同key的转发，只要发包的时候对不同key发往不同ip就可以了，由于有mac的存在其实ip转发也不需要

    // config mac forwarding 问英韬要个完整的表
	{
		cout << "Initiating mac forward table..." << endl;

		auto &forward = program.getTable("Ingress.mac_forward");

		auto dst_addr_key_id = forward.getKey("hdr.ethernet.dst_addr");
		auto send_action_id = forward.getAction("Ingress.send");
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
		forward.dataHandlerReset(send_action_id);
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

		//multicast
		program.configMulticast(1, 11, {40, 32, 24, 16, 8, 0, 4, 12, 28});
	}

	// init network
	int sock_fd;
	sockaddr_ll sl{};
	{
		cout << "Initiating network..." << endl;

		sock_fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
		if(-1 == sock_fd)
		{
			cout << "Create socket error(" << errno << "): " << strerror(errno) << endl;
			return -1;
		}

		ifreq ifr{};
		bzero(&ifr, sizeof(ifr));
		strcpy(ifr.ifr_name, "enp6s0");

		/* set promiscuous */
		ioctl(sock_fd, SIOCGIFFLAGS, &ifr);
		ifr.ifr_flags |= IFF_PROMISC;
		ioctl(sock_fd, SIOCSIFFLAGS, &ifr);
		ioctl(sock_fd, SIOCGIFINDEX, &ifr);

		bzero(&sl, sizeof(sl));
		sl.sll_family = PF_PACKET;
		sl.sll_protocol = htons(ETH_P_ALL);
		sl.sll_ifindex = ifr.ifr_ifindex;

		if(-1 == bind(sock_fd, (sockaddr *) &sl, sizeof(sl)))
		{
			cout << "Bind error(" << errno << "): " << strerror(errno) << endl;
			return -1;
		}

		sock_fprog filter{};
		sock_filter code[] = { // tcpdump -dd ether proto 0x2333 -s 0
				{0x28, 0, 0, 0x0000000c},
				{0x15, 0, 1, 0x00002333},
				{0x6,  0, 0, 0x00040000},
				{0x6,  0, 0, 0x00000000},
		};

		filter.len = sizeof(code) / sizeof(sock_filter);
		filter.filter = code;

		if(-1 == setsockopt(sock_fd, SOL_SOCKET, SO_ATTACH_FILTER, &filter, sizeof(filter)))
		{
			cout << "Set socket option error(" << errno << "): " << strerror(errno) << endl;
			return -1;
		}
	}

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
	int debug2 = 1;
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
			return -1;
		}
	}

	u_char pkt_data[1024];

	// //debug
	// getchar();
	// // u_char rcv_data[1024];
	// auto* eth = (ETH_HEADER*)pkt_data;
	// for(int i = 0; i < 6; i++){
	// 	eth->DestMac[i] = 0x2;
	// 	eth->SrcMac[i] = 0x2;
	// }
	// // eth->Etype = htons(0x2333);
	// auto *lock_ctl_header = (LOCK_CTL_HEADER *) (pkt_data + 14);
	// lock_ctl_header->op = REPLACE_SUCC;
	// eth->Etype[0] = 0x23;
	// eth->Etype[1] = 0x33;
	// // auto* ctrl = (DEBUG_HEADER *)(pkt_data + 14);
	// // ctrl->opType = 8;
	// // ctrl->objIdx = htonl(0x0);;
	// auto r = sendto(sock_fd, pkt_data, 64, 0, (sockaddr *) &sl, sizeof(sl));
	// if(r < 0){
	// 	cout << "error" << endl;
	// 	exit(1);
	// }
	// // auto len = recvfrom(sock_fd, rcv_data, sizeof(rcv_data), 0, nullptr, nullptr);
	// // if(-1 == len){
	// // 	cout << "Receive error(" << errno << "): " << strerror(errno) << endl;
	// // 	exit(1);
	// // }
	// // auto* rcv_eth = (ETH_HEADER*)rcv_data;
	// // auto* rcv_ctrl = (DEBUG_HEADER *) (rcv_data + 14);
	// // printf("recv: ethType = %d, op = %d, idx = %d, counter[0] = %u, counter[1] = %u, counter[2] = %u, counter[3] = %u\n", htons(rcv_eth->Etype), rcv_ctrl->opType, ntohl(rcv_ctrl->objIdx), ntohl(rcv_ctrl->counter[0]), ntohl(rcv_ctrl->counter[1]), ntohl(rcv_ctrl->counter[2]), ntohl(rcv_ctrl->counter[3]));
	// while(true){}
	

	struct timeval start_ini,finish_ini;
	long timeuse_ini;
	
	while(true)
	{
		auto len = recvfrom(sock_fd, pkt_data, sizeof(pkt_data), 0, nullptr, nullptr);
		gettimeofday(&start_ini, NULL ); //-----------------------------

		if(-1 == len)
		{
			cout << "Receive error(" << errno << "): " << strerror(errno) << endl;
			continue;
		}
		auto *lock_ctl_header = (LOCK_CTL_HEADER *) (pkt_data + 14);

		if(lock_ctl_header->op == HOT_REPORT)
		{
			
			// if(debug2) cout << "imhere" << endl;
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

					cout << "{" << id << ", " << index << "}" << endl;
					if(debug) file << "{" << id << ", " << index << "}" << endl;
				}
				else
				{
					cout << "{" << id << ", " << index_iter->second << "} <=> {" << id << ", " << index << "}" << endl;
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

			auto r = sendto(sock_fd, pkt_data, len < 64 ? len : 64, 0, (sockaddr *) &sl, sizeof(sl));
			if(r < 0)
			{
				cout << "error" << endl;
				break;
			}
			if(debug) cout << "Successfully handle " << n - e << " hot replace request" << endl;
			
			gettimeofday(&finish_ini, NULL); //----------------------------
			timeuse_ini = 1000000 * ( finish_ini.tv_sec - start_ini.tv_sec ) + finish_ini.tv_usec - start_ini.tv_usec;
			// if(debug2) printf("TOPK(gettimeofday) %f\n", timeuse_ini * 1.0);
		}
	}

	if(debug) file.close();

	return 0;




//-------------------------------my test---------------------------------
    // init network

    // // kvs function
    // {
    //     // kvs table
	//     auto &mapping = program.getTable("Ingress.index_mapping_table");
	//     auto lock_id_key_id = mapping.getKey("hdr.kv.key");
	//     auto mapping_action_id = mapping.getAction("Ingress.mapping");
	//     auto mapping_action_mapped_index_data_id = mapping.getData("index", "Ingress.mapping");
	
		// // my code for reading precise counters
		// {
		// 	cout << "My code region begins !!!" << endl;
		// 	int counter_size = size;

		// 	P4Table* precise_counter = &program.getTable("Ingress.counter_bucket_reg_" + to_string(0));
		// 	vector<uint64_t> counters(counter_size);
		// 	bf_rt_id_t precise_counter_index_id;
		// 	bf_rt_id_t precise_counter_data_id;
		// 	{
		// 		cout << "Initiating precise counter..." << endl;
		// 		precise_counter = &program.getTable("Ingress.counter_bucket_reg_" + to_string(0));
		// 		precise_counter_index_id = precise_counter->getKey("$REGISTER_INDEX");
		// 		precise_counter_data_id = precise_counter->getData("Ingress.counter_bucket_reg_" + to_string(0) + ".f1");
		// 		precise_counter->setOperation(bfrt::TableOperationsType::REGISTER_SYNC);
		// 		precise_counter->initEntries();
		// 	}

		// 	struct timeval start_ini_2,finish_ini_2;
		// 	long timeuse_ini_2;
	    // 	gettimeofday(&start_ini_2, NULL );

		// 	// read data from the switch data plane
		// 	precise_counter->syncSoftwareShadow();
		// 	// precise_counter->tableGet();
		// 	// const auto table_data = precise_counter->tableDateGet();

		// 	gettimeofday(&finish_ini_2, NULL);
	    // 	timeuse_ini_2 = 1000000 * ( finish_ini_2.tv_sec - start_ini_2.tv_sec ) + finish_ini_2.tv_usec - start_ini_2.tv_usec;
	    // 	printf("sync gettimeofday: counter_size = %d, total_time=%f, average_time=%f\n",counter_size, timeuse_ini_2/1.0, timeuse_ini_2/1.0/counter_size);

	    // 	gettimeofday(&start_ini_2, NULL );
		// 	for(int k = 0; k < counter_size; ++k)
		// 	{
		// 		vector<uint64_t> counter;
		// 		precise_counter->keyHandlerReset();
		// 		precise_counter->keyHandlerSetValue(precise_counter_index_id, k);
		// 		precise_counter->dataHandlerReset();
		// 		precise_counter->tableEntryGet(bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_SW);
		// 		precise_counter->getValue(precise_counter_data_id, &counter);
		// 		counters[k] = counter[0];
		// 	}

	    // 	gettimeofday(&finish_ini_2, NULL);
	    // 	timeuse_ini_2 = 1000000 * ( finish_ini_2.tv_sec - start_ini_2.tv_sec ) + finish_ini_2.tv_usec - start_ini_2.tv_usec;
	    // 	printf("read counters gettimeofday: counter_size = %d, total_time=%f, average_time=%f\n",counter_size, timeuse_ini_2/1.0, timeuse_ini_2/1.0/counter_size);

		// }

        // // my code for cache update
	    // {
	    // 	// init entries
	    // 	cout << "My code region begins !!!" << endl;
	    // 	unordered_map<uint32_t, uint32_t> index_mapping;
	    // 	unordered_map<uint32_t, uint32_t> reverse_mapping;
	    // 	int random_size = size;
	    // 	int cache_size = SLOT_SIZE;
	    // 	{
	    // 		cout << "Initiating mapping for my test!" << endl;
	    // 		clock_t start_ini = clock();
	    // 		struct timeval start_ini_2,finish_ini_2;
	    // 		gettimeofday(&start_ini_2, NULL );
	    // 		program.beginBatch();
	    // 		for(uint64_t i = 0; i < cache_size; ++i){
	    // 			uint32_t id = i;
	    // 			uint32_t index = i;

	    // 			index_mapping[id] = index;
	    // 			reverse_mapping[index] = id;

	    // 			mapping.keyHandlerReset();
        //             uint8_t table_key[16] = {0};
        //             for(int j = 0; j < 4; j++){
        //                 table_key[15-j] = (id >> (8 * j)) & 0xff;  
        //             }
        //             if(i == 1){
        //                 for(int j = 0; j < 16; j++){
        //                     printf("%d\t", table_key[15-j]);
        //                 }
        //                 printf("\n");
        //             }
	    // 			mapping.keyHandlerSetValue(lock_id_key_id, table_key, 16);
	    // 			mapping.dataHandlerReset(mapping_action_id);
	    // 			mapping.dataHandlerSetValue(mapping_action_mapped_index_data_id, i);
	    // 			mapping.tableEntryAdd();
	    // 		}
	    // 		program.endBatch(true);
	    // 		clock_t finish_ini = clock();
	    // 		gettimeofday(&finish_ini_2, NULL);
	    // 		long timeuse_ini_2 = 1000000 * ( finish_ini_2.tv_sec - start_ini_2.tv_sec ) + finish_ini_2.tv_usec - start_ini_2.tv_usec;
	    // 		printf("ini! clock: cache_size = %d, total_time=%f, average_time=%f\n",cache_size, (double)(finish_ini-start_ini), (double)(finish_ini-start_ini)/cache_size);
	    // 		printf("ini! gettimeofday: cache_size = %d, total_time=%f, average_time=%f\n",cache_size, timeuse_ini_2/1.0, timeuse_ini_2/1.0/cache_size);

	    // 		cout << "Initiating random number for my test!" << endl;
	    // 		uniform_int_distribution<unsigned> u(0,cache_size);
	    // 		default_random_engine e;
	    // 		vector<uint32_t> random_cache_obj(cache_size);
	    // 		vector<uint32_t> random_cache_idx(cache_size);

	    // 		// printf("random check (cache_obj, cache_idx)!");
	    // 		for (size_t i = 0; i < random_size; i++){
	    // 			random_cache_obj[i] = u(e);
	    // 			random_cache_idx[i] = u(e);
	    // 		}
                
	    // 		cout << "Random insertion begins!" << endl;
	    // 		clock_t start_test = clock();
	    // 		struct timeval start_test_2,finish_test_2;
	    // 		gettimeofday(&start_test_2, NULL );

	    // 		program.beginBatch();
	    // 		for(uint64_t i = 0; i < random_size; ++i){
	    // 			uint32_t id = random_cache_obj[i] + 32768;
	    // 			uint32_t index = random_cache_idx[i];

	    // 			auto index_iter = index_mapping.find(id);
	    // 			if(index_iter == index_mapping.cend()){
	    // 				auto reverse_iter = reverse_mapping.find(index);
	    // 				if(reverse_iter != reverse_mapping.cend()){
	    // 					index_mapping.erase(reverse_iter->second);

	    // 					mapping.keyHandlerReset();
        //                     uint8_t table_key[16] = {0};
        //                     for(int j = 0; j < 4; j++){
        //                         table_key[15-j] = ((reverse_iter->second) >> (8 * j)) & 0xff;  
        //                     }
        //                     mapping.keyHandlerSetValue(lock_id_key_id, table_key, 16);
	    // 					mapping.tableEntryDelete();
	    // 					// mapping.completeOperations();
	    // 				}

        //                 // if(i < 32) printf("id[%d]=%d, index[%d]=%d\n", i, id, i, index);

	    // 				index_mapping[id] = index;
	    // 				reverse_mapping[index] = id;

	    // 				mapping.keyHandlerReset();
        //                 uint8_t table_key[16] = {0};
        //                 for(int j = 0; j < 4; j++){
        //                     table_key[15-j] = (id >> (8 * j)) & 0xff;  
        //                 }
	    // 				mapping.keyHandlerSetValue(lock_id_key_id, table_key, 16);
	    // 				mapping.dataHandlerReset(mapping_action_id);
	    // 				mapping.dataHandlerSetValue(mapping_action_mapped_index_data_id, (uint64_t)index);
	    // 				mapping.tableEntryAdd();
	    // 				// mapping.completeOperations();
	    // 			}
	    // 		}
	    // 		program.endBatch(true);
	    // 		gettimeofday(&finish_test_2, NULL);
	    // 		clock_t finish_test = clock();
	    // 		long timeuse_test_2 = 1000000 * ( finish_test_2.tv_sec - start_test_2.tv_sec ) + finish_test_2.tv_usec - start_test_2.tv_usec;
	    // 		printf("clock: random_size = %d, total_time=%f, average_time=%f\n",random_size, (double)(finish_test-start_test), (double)(finish_test-start_test)/random_size);
	    // 		printf("gettimeofday: random_size = %d, total_time=%f, average_time=%f\n",random_size, timeuse_test_2/1.0, timeuse_test_2/1.0/random_size);
	    // 	}	
	    // 	cout << "My code region ends !!!" << endl;

        //     // program.idle();
	    // }    
    
    // }

    // stats function
}