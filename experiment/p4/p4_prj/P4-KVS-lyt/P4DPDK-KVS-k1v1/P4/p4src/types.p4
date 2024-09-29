#ifndef _TYPES_P4_
#define _TYPES_P4_


const bit<16> ETHERTYPE_IPV4 = 0x0800;
const bit<16> ETHERTYPE_ARP  = 0x0806;
const bit<16> ETHERTYPE_CTL  = 0x2333;
const bit<16> ETHERTYPE_CTL_ = 0x2334;

const bit<8> IPV4_PROTOCOL_TCP = 6;
const bit<8> IPV4_PROTOCOL_UDP = 17;

const bit<16> UDP_LOCK_PORT = 5001;

const int IPV4_HOST_SIZE = 1024;
const int IPV4_LPM_SIZE  = 12288;


#endif //_TYPES_P4_