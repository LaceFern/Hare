
/* USER_REGION_0 begins */
#include "kvs.p4"
/* USER_REGION_0 ends */

/***************** H E A D E R *********************/
/* USER_REGION_1 begins */
header app_header_extension_t{
    bit<VALSEG_WIDTH> value_0;
//    bit<VALSEG_WIDTH> value_1;
//    bit<VALSEG_WIDTH> value_2;
//    bit<VALSEG_WIDTH> value_3;
}
header app_header_t{
    bit<8>         op_code;
    bit<OBJSEG_WIDTH> objseg_0;
//    bit<OBJSEG_WIDTH> objseg_1;
//    bit<OBJSEG_WIDTH> objseg_2;
//    bit<OBJSEG_WIDTH> objseg_3;
}
/* USER_REGION_1 ends */


/***************** P A R S E R *********************/
parser IngressParser(packet_in       pkt,
    out my_ingress_headers_t         hdr,
    out my_ingress_metadata_t        meta,
    out ingress_intrinsic_metadata_t ig_intr_md){

    /* USER_REGION_3 begins */
    state parse_app{
        pkt.extract(hdr.app.op_code);
        meta.app.index = (bit<OBJIDX_WIDTH>) INVALID_4B;
        transition select(hdr.app.op_code){
            OP_GET: parse_query;
            OP_PUT: parse_datamv;
            default: accept;
        }
    }
    state parse_query{
        pkt.extract(hdr.app.objseg_0);
        transition accept;
    }
    state parse_datamv{
        pkt.extract(meta.app.index);
        pkt.extract(hdr.app_ex.value_0);
        transition accept;
    }
    /* USER_REGION_3 ends */
}


/***************** I N G R E S S *********************/
control Ingress(
    inout my_ingress_headers_t                      hdr,
    inout my_ingress_metadata_t                     meta,
    in    ingress_intrinsic_metadata_t              ig_intr_md,
    in    ingress_intrinsic_metadata_from_parser_t  ig_prsr_md,
    inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
    inout ingress_intrinsic_metadata_for_tm_t       ig_tm_md){

    /* USER_REGION_4 begins */
    /* define_appdata{ */
    KVS(hdr.app.op_code, meta.app.index, hdr.app_ex.value)
    /* } */
    /* USER_REGION_4 ends */
    
    /* USER_REGION_5 begins */
    /* apply_queryhit{*/
        if(hdr.app.op_code == OP_GET){
            reflect();
            KVS_TABLE_APPLY()
            hdr.app_ex.setValid();
            if(meta.app.state == STATE_UPD){
                hdr.app.op_code = OP_GETFAIL;
            }
            else{
                hdr.app.op_code = OP_GETSUCC;
            }
        }
        else{
            mac_forward.apply();
        }
    /* }*/
    /* USER_REGION_5 ends */

    /* USER_REGION_6 begins */
    /* apply_datamv{*/
        if(hdr.app.op_code == OP_PUT){
            drop();
            KVS_PUT_VALUE(meta.hit_index, hdr.values.value)
        }
    /* }*/
    /* USER_REGION_6 ends */
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

