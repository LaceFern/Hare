#include <iostream>
#include <unistd.h>
#include <unordered_set>
#include <fstream>
#include <iomanip>

#include <thread>
#include <mutex>
#include <condition_variable>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <linux/filter.h>
#include <arpa/inet.h>
#include <ctime>

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


[[noreturn]] void control_thread(uint32_t &lock_id, bool &ready, mutex &mutex, condition_variable &cv)
{
	cout << "Initiating control network..." << endl;

	int sock_fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if(-1 == sock_fd)
	{
		cout << "Create socket error(" << errno << "): " << strerror(errno) << endl;
		throw runtime_error("Cannot create socket");
	}

	ifreq ifr{};
	bzero(&ifr, sizeof(ifr));
	strcpy(ifr.ifr_name, "enp6s0"); // strcpy(ifr.ifr_name, "enp4s0f1"); //strcpy(ifr.ifr_name, "enp6s0");

	/* set promiscuous */
	ioctl(sock_fd, SIOCGIFFLAGS, &ifr);
	ifr.ifr_flags |= IFF_PROMISC;
	ioctl(sock_fd, SIOCSIFFLAGS, &ifr);
	ioctl(sock_fd, SIOCGIFINDEX, &ifr);

	sockaddr_ll sl{};
	bzero(&sl, sizeof(sl));
	sl.sll_family = PF_PACKET;
	sl.sll_protocol = htons(ETH_P_ALL);
	sl.sll_ifindex = ifr.ifr_ifindex;

	if(-1 == bind(sock_fd, (sockaddr *) &sl, sizeof(sl)))
	{
		cout << "Bind error(" << errno << "): " << strerror(errno) << endl;
		throw runtime_error("Cannot not bind with socket");
	}

	sock_fprog filter{};
	sock_filter code[] = { // tcpdump -dd udp -s 0
			{ 0x28, 0, 0, 0x0000000c },
			{ 0x15, 0, 5, 0x000086dd },
			{ 0x30, 0, 0, 0x00000014 },
			{ 0x15, 6, 0, 0x00000011 },
			{ 0x15, 0, 6, 0x0000002c },
			{ 0x30, 0, 0, 0x00000036 },
			{ 0x15, 3, 4, 0x00000011 },
			{ 0x15, 0, 3, 0x00000800 },
			{ 0x30, 0, 0, 0x00000017 },
			{ 0x15, 0, 1, 0x00000011 },
			{ 0x6, 0, 0, 0x00040000 },
			{ 0x6, 0, 0, 0x00000000 },
	};

	filter.len = sizeof(code) / sizeof(sock_filter);
	filter.filter = code;

	if(-1 == setsockopt(sock_fd, SOL_SOCKET, SO_ATTACH_FILTER, &filter, sizeof(filter)))
	{
		cout << "Set socket option error(" << errno << "): " << strerror(errno) << endl;
		throw runtime_error("Cannot set socket option");
	}

	unique_lock<std::mutex> lock(mutex);
	u_char pkt_data[1024] = {};
// 	while(true)
// 	{
// 		auto len = recvfrom(sock_fd, pkt_data, sizeof(pkt_data), 0, nullptr, nullptr);
// 		if(-1 == len)
// 		{
// 			cout << "Receive error(" << errno << "): " << strerror(errno) << endl;
// 			continue;
// 		}

// 		auto *lock_header = (KVS_HEADER *) (pkt_data + 42);

//         unsigned char key[KEY_BYTES];
// 		// printf("op = %d\t", lock_header->op_type);
//         for(int i=0; i<KEY_BYTES; i++){
//             key[i] = lock_header->key[KEY_BYTES-1-i];
// 			// printf("key[%d] = %d\t", i, key[i]);
//         }
// 		// printf("\n");
// 		uint32_t key_id = (((uint32_t)key[3]) << 24) | (((uint32_t)key[2]) << 16) | (((uint32_t)key[1]) << 8) | (uint32_t)key[0];

// 		if(lock_header->op_type == OP_GET)
// 		{
// 			if(!ready && lock.owns_lock() || lock.try_lock())
// 			{
// //				cout << "Receive hot report " << ntohl(lock_header->lock_id) << endl;

// 				lock_id = key_id;

// 				ready = true;
// 				lock.unlock();
// 				cv.notify_one();
// 			}
// 		}
// 	}


	clock_t s, e;
	while(true)
	{
			s = clock();
			auto len = recvfrom(sock_fd, pkt_data, sizeof(pkt_data), 0, nullptr, nullptr);

			if(-1 == len)
			{
				cout << "Receive error(" << errno << "): " << strerror(errno) << endl;
				continue;
			}
	
			auto *lock_header = (KVS_HEADER *) (pkt_data + 42);
			if(lock_header->op_type == OP_GET){

				while(true){
					if(ready == false && (lock.owns_lock() || lock.try_lock())) break;
				}

				unsigned char key[KEY_BYTES];
				// printf("op = %d\t", lock_header->op_type);
        		for(int i=0; i<KEY_BYTES; i++){
        		    key[i] = lock_header->key[KEY_BYTES-1-i];
					// printf("key[%d] = %d\t", i, key[i]);
        		}
				// printf("\n");
				uint32_t key_id = (((uint32_t)key[3]) << 24) | (((uint32_t)key[2]) << 16) | (((uint32_t)key[1]) << 8) | (uint32_t)key[0];
				lock_id = key_id;
				ready = true;

				lock.unlock();
				cv.notify_one();
			}
			e = clock();
			// cout << "Receiving core " << (e - s) * 1000000 / CLOCKS_PER_SEC << "us" << endl;
	}

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

		// // 16/- cpu control
		// forward.keyHandlerReset();
		// forward.keyHandlerSetValue(dst_addr_key_id, 0x98039bc7c7fc);
		// forward.dataHandlerReset(send_action_id);
		// forward.dataHandlerSetValue(send_action_port_data_id, (uint64_t) 0); // 16/-
		// forward.tableEntryAdd();

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

		// // mac forward 4 congest control
		// auto &forward_4cc = program.getTable("Ingress.mac_forward_4cc");

		// auto dst_addr_key_id_4cc = forward_4cc.getKey("hdr.ethernet.dst_addr");
		// auto send_action_id_4cc = forward_4cc.getAction("Ingress.send");
		// auto drop_action_id_4cc = forward_4cc.getAction("Ingress.drop");
		// auto send_action_port_data_id_4cc = forward_4cc.getData("port", "Ingress.send");
		// auto multicast_action_id_4cc = forward_4cc.getAction("Ingress.multicast");
		// auto multicast_action_port_data_id_4cc = forward_4cc.getData("mcast_grp", "Ingress.multicast");

		// // 11/-（面板端口） cpu control
		// forward_4cc.keyHandlerReset();
		// forward_4cc.keyHandlerSetValue(dst_addr_key_id_4cc, 0xb8599fe96b1c);
		// forward_4cc.dataHandlerReset(send_action_id_4cc);
		// forward_4cc.dataHandlerSetValue(send_action_port_data_id_4cc, (uint64_t) 40); // 11/-
		// forward_4cc.tableEntryAdd();

		// // 12/-（面板端口） cpu control
		// forward_4cc.keyHandlerReset();
		// forward_4cc.keyHandlerSetValue(dst_addr_key_id_4cc, 0x98039bca4838);
		// forward_4cc.dataHandlerReset(send_action_id_4cc);
		// forward_4cc.dataHandlerSetValue(send_action_port_data_id_4cc, (uint64_t) 32); // 12/-
		// forward_4cc.tableEntryAdd();

		// // 13/-（面板端口） cpu control
		// forward_4cc.keyHandlerReset();
		// forward_4cc.keyHandlerSetValue(dst_addr_key_id_4cc, 0x98039bc7c818);
		// forward_4cc.dataHandlerReset(send_action_id_4cc);
		// forward_4cc.dataHandlerSetValue(send_action_port_data_id_4cc, (uint64_t) 24); // 13/-
		// forward_4cc.tableEntryAdd();

		// // 14/-（面板端口） cpu control
		// forward_4cc.keyHandlerReset();
		// forward_4cc.keyHandlerSetValue(dst_addr_key_id_4cc, 0x98039bc7c0a8);
		// forward_4cc.dataHandlerReset(send_action_id_4cc);
		// forward_4cc.dataHandlerSetValue(send_action_port_data_id_4cc, (uint64_t) 16); // 14/-
		// forward_4cc.tableEntryAdd();

		// // 15/-（面板端口） cpu control
		// forward_4cc.keyHandlerReset();
		// forward_4cc.keyHandlerSetValue(dst_addr_key_id_4cc, 0x043f72deba44);
		// forward_4cc.dataHandlerReset(send_action_id_4cc);
		// forward_4cc.dataHandlerSetValue(send_action_port_data_id_4cc, (uint64_t) 8); // 15/-
		// forward_4cc.tableEntryAdd();

		// // // 16/- cpu control
		// // forward.keyHandlerReset();
		// // forward.keyHandlerSetValue(dst_addr_key_id, 0x98039bc7c7fc);
		// // forward.dataHandlerReset(send_action_id);
		// // forward.dataHandlerSetValue(send_action_port_data_id, (uint64_t) 0); // 16/-
		// // forward.tableEntryAdd();

		// // 17/- cpu control
		// forward_4cc.keyHandlerReset();
		// forward_4cc.keyHandlerSetValue(dst_addr_key_id_4cc, 0x98039bca4018);
		// forward_4cc.dataHandlerReset(send_action_id_4cc);//forward.dataHandlerReset(drop_action_id);//
		// forward_4cc.dataHandlerSetValue(send_action_port_data_id_4cc, (uint64_t) 4); // 17/-
		// forward_4cc.tableEntryAdd();

		// // 18/- cpu control
		// forward_4cc.keyHandlerReset();
		// forward_4cc.keyHandlerSetValue(dst_addr_key_id_4cc, 0x0c42a12b0d70);
		// forward_4cc.dataHandlerReset(send_action_id_4cc);
		// forward_4cc.dataHandlerSetValue(send_action_port_data_id_4cc, (uint64_t) 12); // 18/-
		// forward_4cc.tableEntryAdd();

		// // 20/-（面板端口） cpu control
		// forward_4cc.keyHandlerReset();
		// forward_4cc.keyHandlerSetValue(dst_addr_key_id_4cc, 0x98039bCA4008);
		// forward_4cc.dataHandlerReset(send_action_id_4cc);
		// forward_4cc.dataHandlerSetValue(send_action_port_data_id_4cc, (uint64_t) 28); // 20/-
		// forward_4cc.tableEntryAdd();

		// // 33/0（面板端口） cpu control
		// forward_4cc.keyHandlerReset();
		// forward_4cc.keyHandlerSetValue(dst_addr_key_id_4cc, 0x0090fb7063c7); 
		// forward_4cc.dataHandlerReset(send_action_id_4cc);
		// // forward.dataHandlerSetValue(send_action_port_data_id, (uint64_t) 66); // 故意搞反
		// forward_4cc.dataHandlerSetValue(send_action_port_data_id_4cc, (uint64_t) 64); // 33/0
		// forward_4cc.tableEntryAdd();

		// // 33/2（面板端口） cpu control
		// forward_4cc.keyHandlerReset();
		// forward_4cc.keyHandlerSetValue(dst_addr_key_id_4cc, 0x0090fb7063c6); 
		// forward_4cc.dataHandlerReset(send_action_id_4cc);
		// // forward.dataHandlerSetValue(send_action_port_data_id, (uint64_t) 64); // 
		// forward_4cc.dataHandlerSetValue(send_action_port_data_id_4cc, (uint64_t) 66); // 33/2
		// forward_4cc.tableEntryAdd();

		// //multicast
		// forward_4cc.keyHandlerReset();
		// forward_4cc.keyHandlerSetValue(dst_addr_key_id_4cc, 0xffffffffffff);
		// forward_4cc.dataHandlerReset(multicast_action_id_4cc);
		// forward_4cc.dataHandlerSetValue(multicast_action_port_data_id_4cc, (uint64_t) 1); // 18/-
		// forward_4cc.tableEntryAdd();
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
		// program.configMulticast(1, 11, {40, 32, 24, 16, 8, 0, 4, 12, 28});
		program.configMulticast(1, 11, {40, 32, 24, 16, 8, 4, 12, 28});
		program.configMulticast(2, 22, {192}); // program.configMulticast(2, 22, {64}); //program.configMulticast(2, 22, {192});
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

	auto &clean = program.getTable("Ingress.clean_flag_reg");
	auto clean_index_id = clean.getKey("$REGISTER_INDEX");
	auto clean_data_id = clean.getData("Ingress.clean_flag_reg.f1");

	// init entries
	int begin = 0;//0;
	int end = 3000;//10000;//0;//10000;
	uint32_t id_ini_bias = 0;
	int debug = 0;
	int counter_reg_num = 4;
	int filter_counter_reg_num = 3;
	unordered_map<uint32_t, uint32_t> index_mapping;
	unordered_map<uint32_t, uint32_t> reverse_mapping;
	{
		cout << "Initiating mapping..." << endl;
		cout << "Init lock from lock id " << begin << " to lock id " << end << endl;

		program.beginBatch();
		for(uint64_t i = begin; i < end; ++i)
		{
			uint32_t id = i + id_ini_bias;
			uint32_t index = i;

			index_mapping[id] = index;
			reverse_mapping[index] = id;

			uint8_t table_key[16] = {0};
			for(int j = 0; j < 4; j++){
				table_key[15-j] = ((id) >> (8 * j)) & 0xff;
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

		for(int i = 0; i < counter_reg_num; ++i)
		{
			precise_counter.push_back(&program.getTable("counter_bucket_reg_" + to_string(i)));
			precise_counter_index_id.push_back(precise_counter[i]->getKey("$REGISTER_INDEX"));
			precise_counter_data_id.push_back(precise_counter[i]->getData("Ingress.counter_bucket_reg_" + to_string(i) + ".f1"));

			precise_counter[i]->setOperation(bfrt::TableOperationsType::REGISTER_SYNC);
			precise_counter[i]->initEntries();
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


	// init sketch
	vector<P4Table*> sketch_counter;
	vector<bf_rt_id_t> sketch_counter_index_id;
	vector<bf_rt_id_t> sketch_counter_data_id;
	{
		cout << "Initiating sketch counter..." << endl;

		for(int i = 0; i < counter_reg_num; ++i)
		{
			sketch_counter.push_back(&program.getTable("Ingress.sketch_reg_" + to_string(i)));
			sketch_counter_index_id.push_back(sketch_counter[i]->getKey("$REGISTER_INDEX"));
			sketch_counter_data_id.push_back(sketch_counter[i]->getData("Ingress.sketch_reg_" + to_string(i) + ".f1"));

			sketch_counter[i]->setOperation(bfrt::TableOperationsType::REGISTER_SYNC);
			sketch_counter[i]->initEntries();
		}
	}

	// init filter sketch
	vector<P4Table*> filter_sketch_counter;
	vector<bf_rt_id_t> filter_sketch_counter_index_id;
	vector<bf_rt_id_t> filter_sketch_counter_data_id;
	{
		cout << "Initiating filter_sketch counter..." << endl;

		for(int i = 0; i < filter_counter_reg_num; ++i)
		{
			filter_sketch_counter.push_back(&program.getTable("Ingress.filter_sketch_reg_" + to_string(i)));
			filter_sketch_counter_index_id.push_back(filter_sketch_counter[i]->getKey("$REGISTER_INDEX"));
			filter_sketch_counter_data_id.push_back(filter_sketch_counter[i]->getData("Ingress.filter_sketch_reg_" + to_string(i) + ".f1"));

			filter_sketch_counter[i]->setOperation(bfrt::TableOperationsType::REGISTER_SYNC);
			filter_sketch_counter[i]->initEntries();
		}
	}


	// begin sniff
	cout << "-----------------------------------\n"
	     << "Service begin" << endl;

	// cout << "-----------------------------------\n"
	//      << "Service stall" << endl;
	// while(1);

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
	u_char pkt_receive[1024];
	u_char pkt_send[1024] = {CTRL_PKT_TEMPLATE};

	// // test
	// printf("input one letter to go ahead:\n");
	// getchar();
	// auto *fpga_ctl_send = (FPGA_CTL_HEADER *) (pkt_send + 14);
	// fpga_ctl_send->op = LOCK_HOT_QUEUE;
	// fpga_ctl_send->replace_num = htonl(1);
	// fpga_ctl_send->id[0] = htonl(1234);
	// if(0 > sendto(sock_fd, pkt_send, 64, 0, (sockaddr *) &sl, sizeof(sl)))
	// {
	// 	cout << "error" << endl;
	// }
	// while(1){}

	// 	auto *ether_receive = (ETH_HEADER *) pkt_receive;
	// 	while(true)
	// 	{
	// 		auto len = recvfrom(sock_fd, pkt_receive, sizeof(pkt_receive), 0, nullptr, nullptr);
	// 		if(-1 == len)
	// 		{
	// 			cout << "Receive error(" << errno << "): " << strerror(errno) << endl;
	// 			continue;
	// 		}
	// 		printf("ether_receive->Etype = %x\n", ntohs(ether_receive->Etype));
	// 	}

	// printf("input one letter to get non-zero value from sketch and filter:\n");
	// getchar();
	// unordered_map<uint64_t, uint64_t> sketch_counters;
	// unordered_map<uint64_t, uint64_t> filter_counters;
	// //读出sketch和filter的值看一下
	// for(int i = 0; i < 4; ++i)
	// {
	// 	sketch_counter[i]->syncSoftwareShadow();
	// 	for(int k = 0; k < SKETCH_DEPTH; ++k)
	// 	{
	// 		vector<uint64_t> counter;
	// 		sketch_counter[i]->keyHandlerReset();
	// 		sketch_counter[i]->keyHandlerSetValue(sketch_counter_index_id[i], k);
	// 		sketch_counter[i]->dataHandlerReset();
	// 		sketch_counter[i]->tableEntryGet(bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_SW);
	// 		sketch_counter[i]->getValue(sketch_counter_data_id[i], &counter);
	// 		sketch_counters[i * SKETCH_DEPTH + k] = counter[0];
	// 	}
	// }
	// printf("non-zero position (sketch):\n");
	// for(int k = 0; k < SKETCH_DEPTH; k++){
	// 	for(int i = 0; i < 4; ++i){
	// 		if(sketch_counters[i * SKETCH_DEPTH + k] > 0){
	// 			printf("[%d,%d]:%d\t\t", k, i, sketch_counters[i * SKETCH_DEPTH + k]);
	// 		}
	// 	}
	// 	// printf("\n");
	// }
	// printf("\n");

	// for(int i = 0; i < 3; ++i)
	// {
	// 	filter_sketch_counter[i]->syncSoftwareShadow();
	// 	for(int k = 0; k < FILTER_SKETCH_DEPTH; ++k)
	// 	{
	// 		vector<uint64_t> counter;
	// 		filter_sketch_counter[i]->keyHandlerReset();
	// 		filter_sketch_counter[i]->keyHandlerSetValue(filter_sketch_counter_index_id[i], k);
	// 		filter_sketch_counter[i]->dataHandlerReset();
	// 		filter_sketch_counter[i]->tableEntryGet(bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_SW);
	// 		filter_sketch_counter[i]->getValue(filter_sketch_counter_data_id[i], &counter);
	// 		filter_counters[i * FILTER_SKETCH_DEPTH + k] = counter[0];
	// 	}
	// }
	// printf("non-zero position (filter):\n");
	// for(int k = 0; k < FILTER_SKETCH_DEPTH; k++){
	// 	for(int i = 0; i < 3; ++i){
	// 		if(filter_counters[i * FILTER_SKETCH_DEPTH + k] > 0){
	// 			printf("[%d,%d]:%d\t\t", k, i, filter_counters[i * FILTER_SKETCH_DEPTH + k]);
	// 		}
	// 	}
	// 	// printf("\n");
	// }
	// printf("\n");
// ---------------------------------------------------------------------------------------

	// init thread
	uint32_t id = 0;
	bool ready = false;
	mutex mutex;
	condition_variable cv;
	thread control(control_thread, ref(id), ref(ready), ref(mutex), ref(cv));
	uint64_t loop_count = 0;
	uint64_t report_count = 0;

	auto ts = clock();
	clock_t s, e;
	struct timeval t_gtod, ts_gtod;
	while(true){

		//-----------------------------clean---------------------------
		auto t = clock();
		gettimeofday(&t_gtod, NULL );

		long t_minus_ts = 1000000 * ( t_gtod.tv_sec - ts_gtod.tv_sec ) + t_gtod.tv_usec - ts_gtod.tv_usec;
		// if(t - ts > 500000)
		// if(t_minus_ts > 500000)
		// {
		// 	program.beginBatch();
		// 	for(int i = 0; i < 3; ++i)
		// 	{
		// 		filter_sketch_counter[i]->tableClear();
		// 		// for(uint64_t k = 0; k < FILTER_SKETCH_DEPTH; ++k)
		// 		// {
		// 		// 	filter_sketch_counter[i]->keyHandlerReset();
		// 		// 	filter_sketch_counter[i]->keyHandlerSetValue(filter_sketch_counter_index_id[i], k);
		// 		// 	filter_sketch_counter[i]->dataHandlerReset();
		// 		// 	filter_sketch_counter[i]->dataHandlerSetValue(filter_sketch_counter_data_id[i], (uint64_t) 0);
		// 		// 	filter_sketch_counter[i]->tableEntryModify();
		// 		// }
		// 	}
		// 	program.endBatch(true);
		// }
		
		// if(t - ts > 5000000)
		if(t_minus_ts > 500000) //300000//1000000//5000000
		{		
			cout << "report_count = " << report_count << endl;
			// while(1);
			report_count = 0;
			s = clock();

			clean.keyHandlerReset();
			clean.keyHandlerSetValue(clean_index_id, (uint64_t) 0);
			clean.dataHandlerReset();
			clean.dataHandlerSetValue(clean_data_id, (uint64_t) 1);
			clean.tableEntryModify();
			clean.completeOperations();

			struct timeval start_ini_2,finish_ini_2;
			long timeuse_ini_2;
	    	gettimeofday(&start_ini_2, NULL );

			program.beginBatch();
			for(int i = 0; i < counter_reg_num; ++i)
			{
				precise_counter[i]->tableClear();
				// for(uint64_t k = 0; k * counter_reg_num < SLOT_SIZE; ++k)
				// {
				// 	precise_counter[i]->keyHandlerReset();
				// 	precise_counter[i]->keyHandlerSetValue(precise_counter_index_id[i], k);
				// 	precise_counter[i]->dataHandlerReset();
				// 	precise_counter[i]->dataHandlerSetValue(precise_counter_data_id[i], (uint64_t) 0);
				// 	precise_counter[i]->tableEntryModify();
				// }
			}
			for(int i = 0; i < 4; ++i)
			{
				sketch_counter[i]->tableClear();
				// for(uint64_t k = 0; k < SKETCH_DEPTH; ++k)
				// {
				// 	sketch_counter[i]->keyHandlerReset();
				// 	sketch_counter[i]->keyHandlerSetValue(sketch_counter_index_id[i], k);
				// 	sketch_counter[i]->dataHandlerReset();
				// 	sketch_counter[i]->dataHandlerSetValue(sketch_counter_data_id[i], (uint64_t) 0);
				// 	sketch_counter[i]->tableEntryModify();
				// }
			}
			for(int i = 0; i < 3; ++i)
			{
				filter_sketch_counter[i]->tableClear();
				// for(uint64_t k = 0; k < FILTER_SKETCH_DEPTH; ++k)
				// {
				// 	filter_sketch_counter[i]->keyHandlerReset();
				// 	filter_sketch_counter[i]->keyHandlerSetValue(filter_sketch_counter_index_id[i], k);
				// 	filter_sketch_counter[i]->dataHandlerReset();
				// 	filter_sketch_counter[i]->dataHandlerSetValue(filter_sketch_counter_data_id[i], (uint64_t) 0);
				// 	filter_sketch_counter[i]->tableEntryModify();
				// }
			}
			program.endBatch(true);

	    	gettimeofday(&finish_ini_2, NULL);
	    	timeuse_ini_2 = 1000000 * ( finish_ini_2.tv_sec - start_ini_2.tv_sec ) + finish_ini_2.tv_usec - start_ini_2.tv_usec;
	    	printf("clean gettimeofday: total_time=%f\n",timeuse_ini_2/1.0);
			// printf("im here -2\n");
			clean.keyHandlerReset();
			clean.keyHandlerSetValue(clean_index_id, (uint64_t) 0);
			clean.dataHandlerReset();
			clean.dataHandlerSetValue(clean_data_id, (uint64_t) 0);
			clean.tableEntryModify();
			clean.completeOperations();
			// printf("im here -1\n");
			
			ts = clock();
			gettimeofday(&ts_gtod, NULL );
			e = clock();
			if(debug) cout << "Clean up " << (e - s) * 1000000 / CLOCKS_PER_SEC << "us" << endl;
		}


		//-----------------------------process---------------------------
		s = clock();
		// printf("im here -0.5\n");
		unique_lock<std::mutex> lock(mutex);
		// printf("im here 0\n");
		cv.wait_for(lock, std::chrono::microseconds(100));
		// printf("im here 1\n");
		if(ready == false){
			continue;
		}
		e = clock();
		if(debug) cout << "Wait ready " << (e - s) * 1000000 / CLOCKS_PER_SEC << "us" << endl;
		// if((cv.wait_for(lock, std::chrono::seconds(1)) == std::cv_status::timeout)){
		// 	printf("im here 1!\n");
		// 	continue;
		// }
		// cv.wait(lock, [&ready]{ return ready; });


		ready = false;

		if(debug) cout << "Get hot report id " << id << endl;
		report_count++;

		if(index_mapping.find(id) != index_mapping.end()){
			continue;
		}
		

		// find cold lock (type1：顺序采样块)
		unordered_map<uint64_t, uint64_t> counters;
		s = clock();
		uint64_t scan_index = loop_count++ % (SLOT_SIZE / (counter_reg_num * SAMPLE_SET));
		for(int i = 0; i < counter_reg_num; ++i)
		{
			precise_counter[i]->syncSoftwareShadow();
		}
		e = clock();
		if(debug) cout << "Sync cold " << (e - s) * 1000000 / CLOCKS_PER_SEC << "us" << endl;
		s = clock();
		for(int i = 0; i < counter_reg_num; ++i)
		{
			for(int k = 0; k < SAMPLE_SET; ++k)
			{
				vector<uint64_t> counter;
				precise_counter[i]->keyHandlerReset();
				precise_counter[i]->keyHandlerSetValue(precise_counter_index_id[i], scan_index * SAMPLE_SET + k);
				precise_counter[i]->dataHandlerReset();
				precise_counter[i]->tableEntryGet(bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_SW);
				precise_counter[i]->getValue(precise_counter_data_id[i], &counter);

				counters[scan_index * counter_reg_num * SAMPLE_SET + k * counter_reg_num + i] = counter[0]; // cache index matches counters index!!!!!!!!!!!!!!!!
			}
		}
		e = clock();
		if(debug) cout << "Read cold " << (e - s) * 1000000 / CLOCKS_PER_SEC << "us" << endl;


		// // find cold lock (type2：总共采样范围)
		// unordered_map<uint64_t, uint64_t> counters;
		// s = clock();
		// for(int i = 0; i < counter_reg_num; ++i)
		// {
		// 	precise_counter[i]->syncSoftwareShadow();
		// }
		// e = clock();
		// if(debug) cout << "Sync cold " << (e - s) * 1000000 / CLOCKS_PER_SEC << "us" << endl;
		// s = clock();
		// for(int i = 0; i < counter_reg_num; ++i)
		// {
		// 	for(int k = 0; k < SAMPLE_SET; ++k)
		// 	{
		// 		vector<uint64_t> counter;
		// 		precise_counter[i]->keyHandlerReset();
		// 		precise_counter[i]->keyHandlerSetValue(precise_counter_index_id[i], k);
		// 		precise_counter[i]->dataHandlerReset();
		// 		precise_counter[i]->tableEntryGet(bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_SW);
		// 		precise_counter[i]->getValue(precise_counter_data_id[i], &counter);

		// 		counters[k * counter_reg_num + i] = counter[0]; // cache index matches counters index!!!!!!!!!!!!!!!!
		// 	}
		// }
		// e = clock();
		// if(debug) cout << "Read cold " << (e - s) * 1000000 / CLOCKS_PER_SEC << "us" << endl;



		uint64_t index = counters.cbegin()->first;
		uint64_t min = counters.cbegin()->second;

		s = clock();
		for(const auto &p : counters)
		{
			if(p.second < min)
			{
				index = p.first;
				min = p.second;
			}
		}
		e = clock();
		if(debug) cout << "Find minimum " << (e - s) * 1000000 / CLOCKS_PER_SEC << "us" << endl;

		if(counters[index] > THRESHOLD)
		{
//			cout << "Cannot replace id " << id << endl;
			continue;
		}

		// hangup both queue in data plane and fpga
		s = clock();

		suspend.keyHandlerReset();
		suspend.keyHandlerSetValue(suspend_index_id, index);
		suspend.dataHandlerReset();
		suspend.dataHandlerSetValue(suspend_data_id, (uint64_t) 1);
		suspend.tableEntryModify();
		suspend.completeOperations();

		auto *ether_receive = (ETH_HEADER *) pkt_receive;
		auto *fpga_ctl_receive = (FPGA_CTL_HEADER *) (pkt_receive + 14);

		e = clock();
		if(debug) cout << "Data plane queue always! empty " << (e - s) * 1000000 / CLOCKS_PER_SEC << "us" << endl;

		s = clock();

		auto *fpga_ctl_send = (FPGA_CTL_HEADER *) (pkt_send + 14);
		fpga_ctl_send->op = LOCK_HOT_QUEUE;
		fpga_ctl_send->replace_num = htonl(1);
		fpga_ctl_send->id[0] = htonl(id);

		usleep(100);

		// struct timeval tv;
		// int ret;
		// tv.tv_sec = 0;
		// tv.tv_usec = 100;
		// if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
		// 	if(debug) printf("socket option SO_RCVTIMEO not support\n");
		// }

		// if(0 > sendto(sock_fd, pkt_send, 64, 0, (sockaddr *) &sl, sizeof(sl))) {
		// 	if(debug) cout << "sendto error" << endl;
		// }

		// while(true) {
		// 	ret = recvfrom(sock_fd, pkt_receive, sizeof(pkt_receive), 0, nullptr, nullptr);
		// 	if (ret < 0) {
		// 		if (ret == EWOULDBLOCK || ret == EAGAIN)
		// 			if(debug) printf("recvfrom timeout\n");
		// 		else
		// 			if(debug) printf("recvfrom err:%d\n", ret);
		// 		if(0 > sendto(sock_fd, pkt_send, 64, 0, (sockaddr *) &sl, sizeof(sl))) {
		// 			if(debug) cout << "sendto error" << endl;
		// 		}
		// 	}
		// 	else{
		// 		if(debug) printf("ether_receive->Etype = %x, fpga_ctl_receive->op = %d\n", ntohs(ether_receive->Etype), fpga_ctl_receive->op);
		// 		if(ntohs(ether_receive->Etype) == 0x2333 && fpga_ctl_receive->op == OP_CALLBACK) {
		// 			break;
		// 		}
		// 	}
		// 	// s = clock();
		// 	// auto len = recvfrom(sock_fd, pkt_receive, sizeof(pkt_receive), 0, nullptr, nullptr);
		// 	// if(-1 == len) {
		// 	// 	if(debug) cout << "Receive error(" << errno << "): " << strerror(errno) << endl;
		// 	// 	continue;
		// 	// }	
		// 	// if(debug) printf("ether_receive->Etype = %x, fpga_ctl_receive->op = %d\n", ntohs(ether_receive->Etype), fpga_ctl_receive->op);
		// 	// if(ntohs(ether_receive->Etype) == 0x2333 && fpga_ctl_receive->op == OP_CALLBACK) {
		// 	// 	retry_flag = 0;
		// 	// 	break;
		// 	// }
		// 	// e = clock();
		// 	// if((e - s) * 1000000 / CLOCKS_PER_SEC > 100){
		// 	// 	retry_flag = 0;
		// 	// } 
		// }


		e = clock();
		if(debug) cout << "Sever queue empty " << (e - s) * 1000000 / CLOCKS_PER_SEC << "us" << endl;

		// replace
		s = clock();
		auto index_iter = index_mapping.find(id);
		if(index_iter == index_mapping.cend())
		{
			auto reverse_iter = reverse_mapping.find(index);
			if(reverse_iter != reverse_mapping.cend())
			{
				if(debug) printf("delete: index = %d, key = %d\n", index, reverse_iter->second);

				index_mapping.erase(reverse_iter->second);

				uint8_t table_key[16] = {0};
				for(int j = 0; j < 4; j++){
					table_key[15-j] = (reverse_iter->second >> (8 * j)) & 0xff;
				}

				mapping.keyHandlerReset();
				mapping.keyHandlerSetValue(lock_id_key_id, table_key, 16);
				mapping.tableEntryDelete();
				mapping.completeOperations();

				//打印一下被踢掉的对象，打印一下要插入的对象！！！！！！！！！！！！！！！，index是什么东西?
				if(debug) printf("insert: index = %d, key = %d\n", index, reverse_iter->second);
				// if(debug) file << "{" << reverse_iter->second << ", " << index << "} ==> ";
			}

			index_mapping[id] = index;
			reverse_mapping[index] = id;

			uint8_t table_key[16] = {0};
			for(int j = 0; j < 4; j++){
				table_key[15-j] = (id >> (8 * j)) & 0xff;
			}

			

			mapping.keyHandlerReset();
			mapping.keyHandlerSetValue(lock_id_key_id, table_key, 16);
			mapping.dataHandlerReset(mapping_action_id);
			mapping.dataHandlerSetValue(mapping_action_mapped_index_data_id, index);
			mapping.tableEntryAdd();
			mapping.completeOperations();

			if(debug) cout << "check point 14: index = " << index << endl;

			precise_counter[index % counter_reg_num]->keyHandlerReset();
			precise_counter[index % counter_reg_num]->keyHandlerSetValue(precise_counter_index_id[index % counter_reg_num], index / counter_reg_num);
			precise_counter[index % counter_reg_num]->dataHandlerReset();
			precise_counter[index % counter_reg_num]->dataHandlerSetValue(precise_counter_data_id[index % counter_reg_num], (uint64_t) THRESHOLD);
			precise_counter[index % counter_reg_num]->tableEntryModify();
			precise_counter[index % counter_reg_num]->completeOperations();

			if(debug) printf("count: array_index = %d, element_index = %d\n", index % counter_reg_num, index / counter_reg_num); 
			// if(debug) file << "{" << id << ", " << index << "}" << endl;
		}
		else
		{
			if(debug) printf("id = %d all ready exists. something wrong!\n", id);
			// if(debug) file << "{" << id << ", " << index_iter->second << "} <=> {" << id << ", " << index << "}" << endl;
		}

		valueLen.keyHandlerReset();
		valueLen.keyHandlerSetValue(valueLen_index_id, index);
		valueLen.dataHandlerReset();
		valueLen.dataHandlerSetValue(valueLen_data_id, length);
		valueLen.tableEntryModify();
		valueLen.completeOperations();

		suspend.keyHandlerReset();
		suspend.keyHandlerSetValue(suspend_index_id, index);
		suspend.dataHandlerReset();
		suspend.dataHandlerSetValue(suspend_data_id, (uint64_t) 0);
		suspend.tableEntryModify();
		suspend.completeOperations();

		// notify fpga replace success
		fpga_ctl_send->op = REPLACE_SUCC;

		if(0 > sendto(sock_fd, pkt_send, 64, 0, (sockaddr *) &sl, sizeof(sl)))
		{
			if(debug) cout << "error" << endl;
			break;
		}

		e = clock();
		if(debug) cout << "Replace " << (e - s) * 1000000 / CLOCKS_PER_SEC << "us" << endl;
	}


// // 	while(true)
// // 	{
// // 		auto len = recvfrom(sock_fd, pkt_data, sizeof(pkt_data), 0, nullptr, nullptr);
// // 		if(-1 == len)
// // 		{
// // 			cout << "Receive error(" << errno << "): " << strerror(errno) << endl;
// // 			continue;
// // 		}
// // 		auto *lock_ctl_header = (LOCK_CTL_HEADER *) (pkt_data + 14);
// // 		printf("recv: op = %d\n", lock_ctl_header->op);

// // 		if(lock_ctl_header->op == HOT_REPORT)
// // 		{
// // 			int n = lock_ctl_header->replace_num;
// // 			int e = 0;

// // 			if(debug) file << "Receive " << n << " hot lock" << endl;

// // 			for(int i = 0; i < n && i < 8; ++i)
// // 			{
// // 				uint64_t id = ntohl(lock_ctl_header->id[i]);
// // 				uint64_t index = ntohl(lock_ctl_header->index[i]);
// // 				uint64_t counter = 0xffffffff;//ntohl(lock_ctl_header->counter[i]);

// // 				if(debug) file << "checkpoint 1: i = " << i << "; id = " << id << "; index = " << index << "; counter = " << counter << endl;

// // 				if(index >= SLOT_SIZE)
// // 				{
// // 					if(debug) file << "{" << id << ", " << index << "}: index error" << endl;
// // 					continue;
// // 				}

// // 				if(debug) file << "checkpoint 2" << endl;

// // 				auto index_iter = index_mapping.find(id);
// // 				if(index_iter == index_mapping.cend())
// // 				{
// // 					if(debug) file << "checkpoint 3" << endl;
// // 					auto reverse_iter = reverse_mapping.find(index);
// // 					if(reverse_iter != reverse_mapping.cend())
// // 					{
// // 						if(debug) file << "checkpoint 4" << endl;
// // 						index_mapping.erase(reverse_iter->second);

// // 						uint8_t table_key[16] = {0};
// // 						for(int j = 0; j < 4; j++){
// // 							table_key[15-j] = (reverse_iter->second >> (8 * j)) & 0xff;
// // 						}

// // 						if(debug) file << "checkpoint 5" << endl;
// // 						mapping.keyHandlerReset();
// // 						// mapping.keyHandlerSetValue(lock_id_key_id, reverse_iter->second);
// // 						mapping.keyHandlerSetValue(lock_id_key_id, table_key, 16);
// // 						mapping.tableEntryDelete();
// // 						mapping.completeOperations();

// // 						if(debug) file << "{" << reverse_iter->second << ", " << index << "} ==> ";
// // 					}
// // //				    else if(reverse_mapping.size() < SLOT_SIZE)
// // //				    {
// // //				    	index = reverse_mapping.size();
// // //				    }

// // 					if(debug) file << "checkpoint 6" << endl;

// // 					index_mapping[id] = index;
// // 					reverse_mapping[index] = id;

// // 					if(debug) file << "checkpoint 7" << endl;

// // 					uint8_t table_key[16] = {0};
// // 					for(int j = 0; j < 4; j++){
// // 						table_key[15-j] = (id >> (8 * j)) & 0xff;
// // 					}

// // 					if(debug) file << "checkpoint 8" << endl;

// // 					// mapping.modifyEntry(make_tuple(index, id));
// // 					mapping.keyHandlerReset();
// // 					// mapping.keyHandlerSetValue(lock_id_key_id, id);
// // 					mapping.keyHandlerSetValue(lock_id_key_id, table_key, 16);
// // 					mapping.dataHandlerReset(mapping_action_id);
// // 					mapping.dataHandlerSetValue(mapping_action_mapped_index_data_id, index);
// // 					mapping.tableEntryAdd();
// // 					mapping.completeOperations();

// // 					if(debug) file << "checkpoint 9" << endl;

// // 					precise_counter[index % counter_reg_num]->keyHandlerReset();
// // 					precise_counter[index % counter_reg_num]->keyHandlerSetValue(precise_counter_index_id[index % counter_reg_num], index / counter_reg_num);
// // 					precise_counter[index % counter_reg_num]->dataHandlerReset();
// // 					precise_counter[index % counter_reg_num]->dataHandlerSetValue(precise_counter_data_id[index % counter_reg_num], counter);
// // 					precise_counter[index % counter_reg_num]->tableEntryModify();
// // 					precise_counter[index % counter_reg_num]->completeOperations();

// // 					if(debug) file << "{" << id << ", " << index << "}" << endl;
// // 				}
// // 				else
// // 				{
// // 					if(debug) file << "{" << id << ", " << index_iter->second << "} <=> {" << id << ", " << index << "}" << endl;
// // 					e++;
// // 				}

// // 				// suspend.modifyEntry(index, 0);
// // 				suspend.keyHandlerReset();
// // 				suspend.keyHandlerSetValue(suspend_index_id, index);
// // 				suspend.dataHandlerReset();
// // 				suspend.dataHandlerSetValue(suspend_data_id, (uint64_t) 0);
// // 				suspend.tableEntryModify();
// // 				suspend.completeOperations();
// // 			}

// // 			if(debug) file << "checkpoint 10" << endl;

// // 			auto *ether_header = (ETH_HEADER *) pkt_data;
// // 			swap(ether_header->DestMac, ether_header->SrcMac);
// // 			lock_ctl_header->op = REPLACE_SUCC;

// // 			// cout << "print one letter to send response:" << endl;
// // 			// getchar();

// // 			auto r = sendto(sock_fd, pkt_data, len < 64 ? len : 64, 0, (sockaddr *) &sl, sizeof(sl));
// // 			if(r < 0)
// // 			{
// // 				cout << "error" << endl;
// // 				break;
// // 			}

// // 			if(debug) cout << "Successfully handle " << n - e << " hot replace request" << endl;
// // 		}
// // 	}




	if(debug) file.close();

	return 0;
}