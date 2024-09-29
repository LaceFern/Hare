//
// Created by Alex_Li on 2021/8/23.
//

#ifndef KVS_SWITCH_MY_HEADER_H
#define KVS_SWITCH_MY_HEADER_H

// Ethernet 14
struct ETH_HEADER
{
	u_char DestMac[6];
	u_char SrcMac[6];
	u_char Etype[2];
};

// IPv4 20
struct IP4_HEADER
{
	int header_len: 4;
	int version: 4;
	u_char tos: 8;
	int total_len: 16;
	int ident: 16;
	int flags: 16;
	u_char ttl: 8;
	u_char proto: 8;
	int checksum: 16;
	u_char sourceIP[4];
	u_char destIP[4];
};

// UDP 8
struct UDP_HEADER
{
	int src_port: 16;
	int dst_port: 16;
	int len: 16;
	int checksum: 16;
};

// NC 9
struct NET_HEADER
{
	uint8_t  op: 8;
	uint32_t key: 32;
	uint32_t value: 32;
} __attribute__((packed));

// // hdr.lock_ctl
// struct LOCK_CTL_HEADER
// {
// 	uint8_t  op;
// 	uint32_t  replace_num;
// 	uint32_t index[HOTOBJ_IN_ONEPKT];
// 	uint32_t id[HOTOBJ_IN_ONEPKT];
// 	// uint32_t counter[TOPK];
// } __attribute__((packed));


struct KV_QUERY_HEADER
{
	uint8_t  op;
	uint8_t key[KEY_BYTES];
	uint8_t value[VALUE_BYTES];
} __attribute__((packed));

struct STATS_HEADER{
	ETH_HEADER hdr_eth;
	IP4_HEADER hdr_ip;
	UDP_HEADER hdr_udp;
	KV_QUERY_HEADER hdr_kv_query;
} __attribute__((packed));

struct KV_CTRL_HEADER{
	uint8_t  op;
    uint32_t hash_index;
    uint32_t col_index;
	uint8_t key[KEY_COLNUM][KEY_BYTES];
	uint8_t value[VALUE_COLNUM][VALUE_BYTES];
	uint8_t ack;
} __attribute__((packed));

struct CTRL_HEADER{
	ETH_HEADER hdr_eth;
	KV_CTRL_HEADER hdr_kv_ctrl;
} __attribute__((packed));

#endif //KVS_SWITCH_MY_HEADER_H
