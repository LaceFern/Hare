
#include "kvs.p4"

/***************** H E A D E R *********************/
/* USER_REGION begins*/
header values{
    bit<VALSEG_WIDTH> value_0;
    bit<VALSEG_WIDTH> value_1;
    bit<VALSEG_WIDTH> value_2;
    bit<VALSEG_WIDTH> value_3;
    bit<VALSEG_WIDTH> value_4;
    bit<VALSEG_WIDTH> value_5;
    bit<VALSEG_WIDTH> value_6;
    bit<VALSEG_WIDTH> value_7;
    bit<VALSEG_WIDTH> value_8;
    bit<VALSEG_WIDTH> value_9;
    bit<VALSEG_WIDTH> value_10;
    bit<VALSEG_WIDTH> value_11;
    bit<VALSEG_WIDTH> value_12;
    bit<VALSEG_WIDTH> value_13;
    bit<VALSEG_WIDTH> value_14;
    bit<VALSEG_WIDTH> value_15;
    bit<VALSEG_WIDTH> value_16;
    bit<VALSEG_WIDTH> value_17;
    bit<VALSEG_WIDTH> value_18;
    bit<VALSEG_WIDTH> value_19;
}
/* USER_REGION ends*/

struct my_ingress_headers_t{
    /* USER_REGION ends*/
    bit<8> op;
    bit<OBJSEG_WIDTH> obj_0;
    app_header_extension_t values;    
    /* USER_REGION ends*/
}

struct my_ingress_metadata_t{
    bit<OBJIDX_WIDTH> hit_index;
    bit<8> hit_flag;
}


/***************** P A R S E R *********************/
parser IngressParser(packet_in       pkt,
    out my_ingress_headers_t         hdr,
    out my_ingress_metadata_t        meta,
    out ingress_intrinsic_metadata_t ig_intr_md){

    /* USER_REGION begins*/
    state parse_op{
        pkt.extract(hdr.op);
        transition select(hdr.op){
            OP_GET: parse_obj;
            OP_PUT: parse_index_and_value;
            default: accept;
        }
    }
    state parse_obj{
        pkt.extract(hdr.obj_0);
        transition accept;
    }
    state parse_index_and_value{
        pkt.extract(meta.hit_index);
        pkt.extract(hdr.values);
        transition accept;
    }
    /* USER_REGION ends*/
}


/***************** I N G R E S S *********************/
control Ingress(
    inout my_ingress_headers_t                      hdr,
    inout my_ingress_metadata_t                     meta,
    in    ingress_intrinsic_metadata_t              ig_intr_md,
    in    ingress_intrinsic_metadata_from_parser_t  ig_prsr_md,
    inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
    inout ingress_intrinsic_metadata_for_tm_t       ig_tm_md){

    /* USER_REGION begins*/
    KVS(hdr.op, meta.hit_index, hdr.values.value)
    apply{
        IF_QUERY_HIT{
            if(hdr.op == OP_GET){
                reflect();
                hdr.values.setValid();
                hdr.op = OP_GETSUCC;
                KVS_GET_VALUE(meta.hit_index, hdr.values.value)
            }
        }
        ELSE_IF_CTRL{
            if(hdr.op == OP_PUT){
                drop();
                KVS_PUT_VALUE(meta.hit_index, hdr.values.value)
            }
        }
    }
    /* USER_REGION ends*/
}


/***************** D E P A R S E R *********************/
control IngressDeparser(packet_out                  pkt,
    inout my_ingress_headers_t                      hdr,
    in    my_ingress_metadata_t                     meta,
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

