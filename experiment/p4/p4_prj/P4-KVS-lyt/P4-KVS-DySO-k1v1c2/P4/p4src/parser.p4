#ifndef _PARSER_P4_
#define _PARSER_P4_


#include "types.p4"
#include "header.p4"
#include "resubmit.p4"

/***********************  P A R S E R  **************************/

// see includes/parsers.p4
parser IngressParser(packet_in       pkt,
    /* User */
    out my_ingress_headers_t         hdr,
    out my_ingress_metadata_t        meta,
    /* Intrinsic */
    out ingress_intrinsic_metadata_t ig_intr_md)
{
    /* This is a mandatory state, required by Tofino Architecture */
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
            0x2333        : parse_ctrl;
            default       : accept;
        }
    }

    state parse_ctrl{
        pkt.extract(hdr.ctrl);
        transition accept;
    }

    state parse_arp{
        pkt.extract(hdr.arp);
        meta.dst_ipv4 = hdr.arp.dst_proto_addr;
        transition accept;
    }

    state parse_ipv4{
        pkt.extract(hdr.ipv4);
        meta.dst_ipv4 = hdr.ipv4.dst_addr;
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
            // UDP_LOCK_PORT: parse_kv;
            // default      : accept;
            default      : parse_kv;
        }
    }

    state parse_kv{
        pkt.extract(hdr.kv);
        transition select(hdr.kv.op_code){
            OP_PUT : parse_kv_data;
            default: accept;
        }
    }

    state parse_kv_data{
        pkt.extract(hdr.kv_data);
        transition accept;
    }
}

/*********************  D E P A R S E R  ************************/

control IngressDeparser(packet_out                  pkt,
    /* User */
    inout my_ingress_headers_t                      hdr,
    in    my_ingress_metadata_t                     meta,
    /* Intrinsic */
    in    ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md)
{
    Checksum() ipv4_checksum;

    apply
    {
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
    out egress_intrinsic_metadata_t eg_intr_md)
{
    internal_h internal;
    bridge_h bridge;

    state start{
        pkt.extract(eg_intr_md);

        internal = pkt.lookahead<internal_h>();
        transition select(internal.header_type)
        {
            HEADER_TYPE_BRIDGE: parse_bridge;
            HEADER_TYPE_NORMAL: parse_normal;
            HEADER_TYPE_MIRROR: parse_egress_mirror;
            default           : accept;
        }
    }

    state parse_bridge{
        pkt.extract(bridge);
        meta.egress_mirror.bridge_md = bridge.egress_bridge_md;
        transition parse_ethernet;
    }

    state parse_normal{
        pkt.extract(bridge);
        meta.kv.hash_index = bridge.egress_bridge_md.hash_index;
        meta.kv.colhit_index = bridge.egress_bridge_md.colhit_index;
        transition parse_ethernet;
    }

    state parse_egress_mirror{
        pkt.extract(meta.egress_mirror);
        transition parse_ethernet;
    }

    //----------------------------------------
    state parse_ethernet{
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.ether_type){
            ETHERTYPE_IPV4: parse_ipv4;
            ETHERTYPE_ARP : parse_arp;
            0x2333        : parse_ctrl;
            default       : accept;
        }
    }

    state parse_ctrl{
        pkt.extract(hdr.ctrl);
        transition accept;
    }

    state parse_arp{
        pkt.extract(hdr.arp);
        meta.dst_ipv4 = hdr.arp.dst_proto_addr;
        transition accept;
    }

    state parse_ipv4{
        pkt.extract(hdr.ipv4);
        meta.dst_ipv4 = hdr.ipv4.dst_addr;
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
            // UDP_LOCK_PORT: parse_kv;
            // default      : accept;
            default      : parse_kv;
        }
    }

    state parse_kv{
        pkt.extract(hdr.kv);
        transition select(hdr.kv.op_code){
            OP_PUT : parse_kv_data;
            default: accept;
        }
    }

    state parse_kv_data{
        pkt.extract(hdr.kv_data);
        transition accept;
    }
}

/*********************  D E P A R S E R  ************************/

control EgressDeparser(packet_out                  pkt,
    /* User */
    inout my_egress_headers_t                      hdr,
    in    my_egress_metadata_t                     meta,
    /* Intrinsic */
    in    egress_intrinsic_metadata_for_deparser_t eg_dprsr_md)
{
    Mirror() mirror;

    apply
    {
        if(eg_dprsr_md.mirror_type == EGRESS_MIRROR)
        {
            mirror.emit<egress_mirror_h>(
                meta.mirror_session, // mirror_session_id
                meta.egress_mirror
            );
        }

        pkt.emit(hdr);
    }
}


#endif //_PARSER_P4_
