#ifndef _HEADER_P4_
#define _HEADER_P4_


#include "types.p4"
#include "config.p4"
#include "resubmit.p4"

/***********************  H E A D E R S  ************************/

header ethernet_h
{
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> ether_type;
}

header arp_h
{
    bit<16> hw_type;
    bit<16> proto_type;
    bit<8>  hw_addr_len;
    bit<8>  proto_addr_len;
    bit<16> opcode;

    bit<48> src_hw_addr;
    bit<32> src_proto_addr;
    bit<48> dst_hw_addr;
    bit<32> dst_proto_addr;
}

header ipv4_h
{
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  diffserv;
    bit<16> total_len;
    bit<16> identification;
    bit<3>  flags;
    bit<13> frag_offset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdr_checksum;
    bit<32> src_addr;
    bit<32> dst_addr;
}

header tcp_h
{
    bit<16> src_port;
    bit<16> dst_port;
    bit<32> seq_no;
    bit<32> ack_no;
    bit<4>  data_offset;
    bit<3>  res;
    bit<3>  ecn;
    bit<6>  ctrl;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgent_ptr;
}

header udp_t
{
    bit<16> src_port;
    bit<16> dst_port;
    bit<16> len;
    bit<16> checksum;
}

/* Customized struct */

header ctrl_header_t
{
    oper_t  op_code;

    bit<32> index;
    bit<32> counter_0;
    bit<32> counter_1;
    bit<32> counter_2;
    bit<32> counter_3;
    bit<32> counter_4;
    bit<32> counter_5;
    bit<32> counter_6;
    bit<32> counter_7;

    bit<8>  ack;
}

header kv_header_t
{
    oper_t         op_code;
    bit<8>  valid_len;
    bit<KEY_WIDTH> key;
}

/* USER_REGION_1 begins*/
/* USER_REGION_1 ends*/

struct my_ingress_headers_t
{
    /* USER_REGION_2 begins*/
    /* USER_REGION_2 ends*/
}

/******  G L O B A L   I N G R E S S   M E T A D A T A  *********/

struct my_ingress_metadata_t
{
    bit<32> dst_ipv4;
    // bit<4>  op_type; 

    // bridge_md_t bridge_md;
    bit<OBJIDX_WIDTH> hit_index;
    bit<8> hit_flag;
}

/***********************  H E A D E R S  ************************/

struct my_egress_headers_t
{
    ethernet_h ethernet;
    ipv4_h     ipv4;
    udp_t      udp;

    kv_header_t      kv;
    kv_data_header_t kv_data;
}

/********  G L O B A L   E G R E S S   M E T A D A T A  *********/

struct my_egress_metadata_t
{
    MirrorId_t      mirror_session;
    egress_mirror_h egress_mirror;
}


#endif //_HEADER_P4_