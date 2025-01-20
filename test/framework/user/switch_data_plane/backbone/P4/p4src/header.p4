#ifndef _HEADER_P4_
#define _HEADER_P4_


#include "types.p4"
#include "config.p4"

/***********************  H E A D E R S  ************************/

header ethernet_h{
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> ether_type;
}

header arp_h{
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

header ipv4_h{
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

header tcp_h{
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

header udp_t{
    bit<16> src_port;
    bit<16> dst_port;
    bit<16> len;
    bit<16> checksum;
}

/* Customized struct */

header ctrl_header_t{
    bit<8>  op_code;
    bit<OBJIDX_WIDTH> index;
    bit<COUNTER_WIDTH> counter_0;
//    bit<COUNTER_WIDTH> counter_1;
//    bit<COUNTER_WIDTH> counter_2;
//    bit<COUNTER_WIDTH> counter_3;

}

/* USER_REGION_1 begins */
/* USER_REGION_1 ends */

header app_meta_t{
    bit<OBJIDX_WIDTH> segidx_0;
//    bit<OBJIDX_WIDTH> segidx_1;
//    bit<OBJIDX_WIDTH> segidx_2;
    bit<OBJIDX_WIDTH> index; //not segidx_3, but (final) index
    bit<1> statistic_flag;
    bit<STATE_WIDTH> state;
    bit<7> padding;
}

struct my_ingress_headers_t{

    ethernet_h ethernet;
    arp_h      arp;
    ipv4_h     ipv4;
    tcp_h      tcp;
    udp_t      udp;

    ctrl_header_t    ctrl;
    
    app_header_t    app;    
    app_header_extension_t    app_ex;    
}

/******  G L O B A L   I N G R E S S   M E T A D A T A  *********/

struct my_ingress_metadata_t{
    app_meta_t app;
}

/***********************  H E A D E R S  ************************/

struct my_egress_headers_t
{
    ethernet_h ethernet;
    arp_h      arp;
    ipv4_h     ipv4;
    tcp_h      tcp;
    udp_t      udp;
}

/********  G L O B A L   E G R E S S   M E T A D A T A  *********/

struct my_egress_metadata_t
{
    app_meta_t app;
}


#endif //_HEADER_P4_
