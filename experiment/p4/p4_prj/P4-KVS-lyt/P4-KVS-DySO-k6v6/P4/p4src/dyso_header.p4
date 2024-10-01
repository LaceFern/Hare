#ifndef _HEADER_P4_
#define _HEADER_P4_


#include "types.p4"
#include "config.p4"
#include "dyso_config_append.p4"
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
// header ctrl_header_t{
//     oper_t  op_code;
//     bit<KEY_REGLENWIDTH> hash_index;
//     bit<KEY_REGLENWIDTH> col_index;
//     bit<KEY_REGWIDTH> key_0_0;
//     bit<KEY_REGWIDTH> key_1_0;
//     bit<KEY_REGWIDTH> key_2_0;
//     bit<KEY_REGWIDTH> key_3_0;
//     bit<KEY_REGWIDTH> key_0_1;
//     bit<KEY_REGWIDTH> key_1_1;
//     bit<KEY_REGWIDTH> key_2_1;
//     bit<KEY_REGWIDTH> key_3_1;
//     bit<KEY_REGWIDTH> key_0_2; // col = 4
//     bit<KEY_REGWIDTH> key_1_2;
//     bit<KEY_REGWIDTH> key_2_2;
//     bit<KEY_REGWIDTH> key_3_2;
//     bit<KEY_REGWIDTH> key_0_3;
//     bit<KEY_REGWIDTH> key_1_3;
//     bit<KEY_REGWIDTH> key_2_3;
//     bit<KEY_REGWIDTH> key_3_3;
//     bit<VALUE_REGWIDTH> value_0_0;
//     bit<VALUE_REGWIDTH> value_1_0;
//     bit<VALUE_REGWIDTH> value_2_0;
//     bit<VALUE_REGWIDTH> value_3_0;
//     bit<VALUE_REGWIDTH> value_0_1;
//     bit<VALUE_REGWIDTH> value_1_1;
//     bit<VALUE_REGWIDTH> value_2_1;
//     bit<VALUE_REGWIDTH> value_3_1;
//     bit<VALUE_REGWIDTH> value_0_2; // col = 4
//     bit<VALUE_REGWIDTH> value_1_2; 
//     bit<VALUE_REGWIDTH> value_2_2;
//     bit<VALUE_REGWIDTH> value_3_2;
//     bit<VALUE_REGWIDTH> value_0_3;
//     bit<VALUE_REGWIDTH> value_1_3;
//     bit<VALUE_REGWIDTH> value_2_3;
//     bit<VALUE_REGWIDTH> value_3_3;
//     bit<8>  ack;
// }

header ctrl_opHashInfo_header_t{
    oper_t  op_code;
    bit<KEY_REGLENWIDTH> hash_index;
    bit<KEY_REGLENWIDTH> col_index;
}

header ctrl_keyInfo_header_t{
    bit<KEY_REGWIDTH> key_0_0;
    bit<KEY_REGWIDTH> key_1_0;
    bit<KEY_REGWIDTH> key_2_0;
    bit<KEY_REGWIDTH> key_3_0;
    bit<KEY_REGWIDTH> key_4_0;
    bit<KEY_REGWIDTH> key_5_0;
    bit<KEY_REGWIDTH> key_6_0;
    bit<KEY_REGWIDTH> key_7_0;
    bit<KEY_REGWIDTH> key_8_0;
    bit<KEY_REGWIDTH> key_9_0;
    bit<KEY_REGWIDTH> key_10_0;
    bit<KEY_REGWIDTH> key_11_0;
    bit<KEY_REGWIDTH> key_12_0;
    bit<KEY_REGWIDTH> key_13_0;
    bit<KEY_REGWIDTH> key_14_0;
    bit<KEY_REGWIDTH> key_15_0;
    bit<KEY_REGWIDTH> key_16_0;
    bit<KEY_REGWIDTH> key_17_0;
    bit<KEY_REGWIDTH> key_18_0;
    bit<KEY_REGWIDTH> key_19_0;
    bit<KEY_REGWIDTH> key_20_0;
    bit<KEY_REGWIDTH> key_21_0;
    bit<KEY_REGWIDTH> key_22_0;
    bit<KEY_REGWIDTH> key_23_0;
    // bit<KEY_REGWIDTH> key_0_0;
    // bit<KEY_REGWIDTH> key_1_0;
    // bit<KEY_REGWIDTH> key_2_0;
    // bit<KEY_REGWIDTH> key_3_0;
    // bit<KEY_REGWIDTH> key_0_1;
    // bit<KEY_REGWIDTH> key_1_1;
    // bit<KEY_REGWIDTH> key_2_1;
    // bit<KEY_REGWIDTH> key_3_1;
    // bit<KEY_REGWIDTH> key_0_2; // col = 4
    // bit<KEY_REGWIDTH> key_1_2;
    // bit<KEY_REGWIDTH> key_2_2;
    // bit<KEY_REGWIDTH> key_3_2;
    // bit<KEY_REGWIDTH> key_0_3;
    // bit<KEY_REGWIDTH> key_1_3;
    // bit<KEY_REGWIDTH> key_2_3;
    // bit<KEY_REGWIDTH> key_3_3;
}

header ctrl_valueInfo_header_t{
    bit<VALUE_REGWIDTH> value_0_0;
    bit<VALUE_REGWIDTH> value_1_0;
    bit<VALUE_REGWIDTH> value_2_0;
    bit<VALUE_REGWIDTH> value_3_0;
    bit<VALUE_REGWIDTH> value_4_0;
    bit<VALUE_REGWIDTH> value_5_0;
    bit<VALUE_REGWIDTH> value_6_0;
    bit<VALUE_REGWIDTH> value_7_0;
    bit<VALUE_REGWIDTH> value_8_0; // col = 4
    bit<VALUE_REGWIDTH> value_9_0; 
    bit<VALUE_REGWIDTH> value_10_0;
    bit<VALUE_REGWIDTH> value_11_0;
    bit<VALUE_REGWIDTH> value_12_0;
    bit<VALUE_REGWIDTH> value_13_0;
    bit<VALUE_REGWIDTH> value_14_0;
    bit<VALUE_REGWIDTH> value_15_0;
    bit<VALUE_REGWIDTH> value_16_0;
    bit<VALUE_REGWIDTH> value_17_0;
    bit<VALUE_REGWIDTH> value_18_0;
    bit<VALUE_REGWIDTH> value_19_0;
    bit<VALUE_REGWIDTH> value_20_0;
    bit<VALUE_REGWIDTH> value_21_0;
    bit<VALUE_REGWIDTH> value_22_0;
    bit<VALUE_REGWIDTH> value_23_0;
    // bit<VALUE_REGWIDTH> value_0_0;
    // bit<VALUE_REGWIDTH> value_1_0;
    // bit<VALUE_REGWIDTH> value_2_0;
    // bit<VALUE_REGWIDTH> value_3_0;
    // bit<VALUE_REGWIDTH> value_0_1;
    // bit<VALUE_REGWIDTH> value_1_1;
    // bit<VALUE_REGWIDTH> value_2_1;
    // bit<VALUE_REGWIDTH> value_3_1;
    // bit<VALUE_REGWIDTH> value_0_2; // col = 4
    // bit<VALUE_REGWIDTH> value_1_2; 
    // bit<VALUE_REGWIDTH> value_2_2;
    // bit<VALUE_REGWIDTH> value_3_2;
    // bit<VALUE_REGWIDTH> value_0_3;
    // bit<VALUE_REGWIDTH> value_1_3;
    // bit<VALUE_REGWIDTH> value_2_3;
    // bit<VALUE_REGWIDTH> value_3_3;
}

header ctrl_ack_header_t
{
    bit<8>  ack;
}

// header ctrl_header_t{
//     ctrl_opHashInfo_header_t opHashInfo;
//     ctrl_keyInfo_header_t keyInfo;
//     ctrl_valueInfo_header_t valueInfo;
//     bit<8>  ack;
// }

header kv_header_t
{
    oper_t         op_code;
    bit<8>  valid_len;
    bit<KEY_REGWIDTH> key_0;
    bit<KEY_REGWIDTH> key_1;
    bit<KEY_REGWIDTH> key_2;
    bit<KEY_REGWIDTH> key_3;
    bit<KEY_REGWIDTH> key_4;
    bit<KEY_REGWIDTH> key_5;
    bit<KEY_REGWIDTH> key_6;
    bit<KEY_REGWIDTH> key_7;
    bit<KEY_REGWIDTH> key_8;
    bit<KEY_REGWIDTH> key_9;
    bit<KEY_REGWIDTH> key_10;
    bit<KEY_REGWIDTH> key_11;
    bit<KEY_REGWIDTH> key_12;
    bit<KEY_REGWIDTH> key_13;
    bit<KEY_REGWIDTH> key_14;
    bit<KEY_REGWIDTH> key_15;
    bit<KEY_REGWIDTH> key_16;
    bit<KEY_REGWIDTH> key_17;
    bit<KEY_REGWIDTH> key_18;
    bit<KEY_REGWIDTH> key_19;
    bit<KEY_REGWIDTH> key_20;
    bit<KEY_REGWIDTH> key_21;
    bit<KEY_REGWIDTH> key_22;
    bit<KEY_REGWIDTH> key_23;
}

header kv_meta_t
{
    bit<KEY_STAGENUM4COL> col_hits;
    bit<KEY_REGLENWIDTH> hash_index;
    bit<KEY_REGLENWIDTH> colhit_index;
    bit<2> padding;
}

header kv_data_header_t
{
    bit<8>  valid_len;
    bit<VALUE_REGWIDTH> value_0;
    bit<VALUE_REGWIDTH> value_1;
    bit<VALUE_REGWIDTH> value_2;
    bit<VALUE_REGWIDTH> value_3;
    bit<VALUE_REGWIDTH> value_4;
    bit<VALUE_REGWIDTH> value_5;
    bit<VALUE_REGWIDTH> value_6;
    bit<VALUE_REGWIDTH> value_7;
    bit<VALUE_REGWIDTH> value_8;
    bit<VALUE_REGWIDTH> value_9;
    bit<VALUE_REGWIDTH> value_10;
    bit<VALUE_REGWIDTH> value_11;
    bit<VALUE_REGWIDTH> value_12;
    bit<VALUE_REGWIDTH> value_13;
    bit<VALUE_REGWIDTH> value_14;
    bit<VALUE_REGWIDTH> value_15;
    bit<VALUE_REGWIDTH> value_16;
    bit<VALUE_REGWIDTH> value_17;
    bit<VALUE_REGWIDTH> value_18;
    bit<VALUE_REGWIDTH> value_19;
    bit<VALUE_REGWIDTH> value_20;
    bit<VALUE_REGWIDTH> value_21;
    bit<VALUE_REGWIDTH> value_22;
    bit<VALUE_REGWIDTH> value_23;
}

struct my_ingress_headers_t
{
    bridge_h   bridge;

    ethernet_h ethernet;
    arp_h      arp;
    ipv4_h     ipv4;
    tcp_h      tcp;
    udp_t      udp;

    kv_header_t      kv;
    kv_data_header_t kv_data;

    // ctrl_header_t    ctrl;
    ctrl_opHashInfo_header_t ctrl_opHashInfo;
    ctrl_keyInfo_header_t ctrl_keyInfo;
    ctrl_valueInfo_header_t ctrl_valueInfo;
    ctrl_ack_header_t ctrl_ack;
}

/******  G L O B A L   I N G R E S S   M E T A D A T A  *********/

struct my_ingress_metadata_t
{
    bit<32> dst_ipv4;
    kv_meta_t      kv;
    // bit<4>  op_type; 

    // bridge_md_t bridge_md;
}

/***********************  H E A D E R S  ************************/

struct my_egress_headers_t
{
    ethernet_h ethernet;
    arp_h      arp;
    ipv4_h     ipv4;
    tcp_h      tcp;
    udp_t      udp;
    
    kv_header_t      kv;
    kv_data_header_t kv_data;

    // ctrl_header_t    ctrl;
    ctrl_opHashInfo_header_t ctrl_opHashInfo;
    // ctrl_keyInfo_header_t ctrl_keyInfo;
    ctrl_valueInfo_header_t ctrl_valueInfo;
    ctrl_ack_header_t ctrl_ack;
}

/********  G L O B A L   E G R E S S   M E T A D A T A  *********/

struct my_egress_metadata_t
{
    MirrorId_t      mirror_session;
    egress_mirror_h egress_mirror;

    bit<32> dst_ipv4;
    kv_meta_t       kv;
}



#endif //_HEADER_P4_
