//
// Created by Alex_Li on 2021/8/23.
//

#ifndef KVS_SWITCH_MY_HEADER_H
#define KVS_SWITCH_MY_HEADER_H

#define KEY_BYTES 16

// Ethernet 14
struct ETH_HEADER
{
	u_char DestMac[6];
	u_char SrcMac[6];
	uint16_t Etype;
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

// hdr.lock_ctl
struct LOCK_CTL_HEADER
{
	uint8_t  op;
	uint8_t  replace_num;
	uint32_t index[8];
	uint32_t id[8];
	uint32_t counter[8];
} __attribute__((packed));


struct KVS_HEADER
{
    uint8_t op_type;
	uint8_t key[KEY_BYTES];
	//....
} __attribute__((packed));

struct FPGA_CTL_HEADER
{
    uint8_t op;
	uint32_t replace_num;
	uint32_t id[1];
	//....
} __attribute__((packed)); 


#endif //KVS_SWITCH_MY_HEADER_H
