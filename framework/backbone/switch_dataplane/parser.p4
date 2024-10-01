#ifndef _PARSER_P4_
#define _PARSER_P4_

#include "types.p4"
#include "header.p4"

/***********************  P A R S E R  **************************/

parser IngressParser(packet_in       pkt,
    out my_ingress_headers_t         hdr,
    out my_ingress_metadata_t        meta,
    out ingress_intrinsic_metadata_t ig_intr_md){
    state start{
        pkt.extract(ig_intr_md);
        pkt.advance(PORT_METADATA_SIZE);
        transition parse_ethernet;
    }

    state parse_ethernet{
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.ether_type){
            ETHERTYPE_IPV4: parse_ipv4;
            ETHERTYPE_ARP : parse_arp;
            default       : accept;
        }
    }

    state parse_arp{
        pkt.extract(hdr.arp);
        transition accept;
    }

    state parse_ipv4{
        pkt.extract(hdr.ipv4);
        transition select(hdr.ipv4.protocol){
            IPV4_PROTOCOL_TCP: parse_tcp;
            IPV4_PROTOCOL_UDP: parse_udp;
            default          : accept;
        }
    }

    state parse_tcp{
        pkt.extract(hdr.tcp);
        transition accept;
    }

    state parse_udp{
        pkt.extract(hdr.udp);
        transition select(hdr.udp.dst_port){
            UDP_CTRL_PORT: parse_ctrl;
            default      : parse_app; // parse_app;
        }
    }

    state parse_ctrl{
        pkt.extract(hdr.ctrl);
        transition accept;
    }

    /* USER_REGION parser begins */
    state parse_app{
        pkt.extract(hdr.app);
        meta.app.index = (bit<OBJIDX_WIDTH>) INVALID_4B;
        transition select(hdr.app.op_code){
            default: accept;
        }
    }
    /* USER_REGION parser ends */
}

/*********************  D E P A R S E R  ************************/

control IngressDeparser(packet_out                  pkt,
    /* User */
    inout my_ingress_headers_t                      hdr,
    in    my_ingress_metadata_t                     meta,
    /* Intrinsic */
    in    ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md){

    Checksum() ipv4_checksum;
    apply{
        hdr.ipv4.hdr_checksum = ipv4_checksum.update({
                hdr.ipv4.version,
                hdr.ipv4.ihl,
                hdr.ipv4.diffserv,
                hdr.ipv4.total_len,
                hdr.ipv4.identification,
                hdr.ipv4.flags,
                hdr.ipv4.frag_offset,
                hdr.ipv4.ttl,
                hdr.ipv4.protocol,
                hdr.ipv4.src_addr,
                hdr.ipv4.dst_addr
        });

        pkt.emit(hdr);
    }
}

/***********************  P A R S E R  **************************/

parser EgressParser(packet_in       pkt,
    /* User */
    out my_egress_headers_t         hdr,
    out my_egress_metadata_t        meta,
    /* Intrinsic */
    out egress_intrinsic_metadata_t eg_intr_md){

    state start{
        pkt.extract(eg_intr_md);
        transition accept;
    }
}

/*********************  D E P A R S E R  ************************/

control EgressDeparser(packet_out                  pkt,
    /* User */
    inout my_egress_headers_t                      hdr,
    in    my_egress_metadata_t                     meta,
    /* Intrinsic */
    in    egress_intrinsic_metadata_for_deparser_t eg_dprsr_md){

    apply{
        pkt.emit(hdr);
    }
}


#endif //_PARSER_P4_
