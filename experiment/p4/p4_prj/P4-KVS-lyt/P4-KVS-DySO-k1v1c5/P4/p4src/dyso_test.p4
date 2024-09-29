#include <core.p4>
#include <tna.p4>

#include "types.p4"
#include "config.p4"
#include "dyso_config_append.p4"
#include "dyso_header.p4"
// #include "header.p4"
#include "parser.p4"

// #include "control.p4"
// #include "precise_counter.p4"
// #include "sketch.p4"

// #include "kvs.p4"


/*************************************************************************
 **************  I N G R E S S   P R O C E S S I N G   *******************
 *************************************************************************/

/***************** M A T C H - A C T I O N  *********************/

control Ingress(
    /* User */
    inout my_ingress_headers_t                      hdr,
    inout my_ingress_metadata_t                     meta,
    /* Intrinsic */
    in    ingress_intrinsic_metadata_t              ig_intr_md,
    in    ingress_intrinsic_metadata_from_parser_t  ig_prsr_md,
    inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
    inout ingress_intrinsic_metadata_for_tm_t       ig_tm_md)
{
    // forward
    action send(PortId_t port){
        ig_tm_md.ucast_egress_port = port;
    }
    action reflect(){
        send(ig_intr_md.ingress_port);
        bit<48> dmac = hdr.ethernet.dst_addr;
        hdr.ethernet.dst_addr = hdr.ethernet.src_addr;
        hdr.ethernet.src_addr = dmac;
        bit<32> dip = hdr.ipv4.dst_addr;
        hdr.ipv4.dst_addr = hdr.ipv4.src_addr;
        hdr.ipv4.src_addr = dip;
        bit<16> dport = hdr.udp.dst_port;
        hdr.udp.dst_port = hdr.udp.src_port;
        hdr.udp.src_port = dport;
    }
    action modify_service_op(oper_t op_code){
        hdr.kv.op_code = op_code;
    }
    // action modify_control_op(oper_t op_code){
    //     hdr.ctrl.op_code = op_code;
    // }
    // action ack_to_controller(){
    //     hdr.ctrl.ack = 1;
    // }
    action set_bridge(header_type_t header_type){
        hdr.bridge.setValid();
        hdr.bridge.header_type = header_type;
    }
    action drop(){
        ig_dprsr_md.drop_ctl = 1;
    }
    action multicast(MulticastGroupId_t mcast_grp){
        ig_tm_md.mcast_grp_a = mcast_grp;
    }
    table mac_forward{
        key = {
            hdr.ethernet.dst_addr : exact;
        }
        actions = {
            send;
            drop;
            multicast;
            @defaultonly NoAction;
        }
        default_action = NoAction;
        size           = IPV4_HOST_SIZE;
    }
//------------------------------------------------------------------------
    // define action: calculate hash index and assign it to meta
    HASH_KEY2INDEX_ACTION()
    // define hit-check r-mat
    KEY_RMAT_REG()
    // define hit-check action & mat
    KEY_COLS_ACTIONandMAT()

    // // define record r-mat
    // RECORD_RMAT_NEW()
    // // define action: check record for each col
    // RECORD_CHECK_NEW()

    apply {
        // set bridge
        set_bridge(HEADER_TYPE_NORMAL); //attention!!!!!!!!

        // get hash index
        HASH_KEY2INDEX_APPLY()

        // hit check || record || ctrl
        KEY_COLS_APPLY()

        if(hdr.kv.isValid()){
            if(hdr.bridge.egress_bridge_md.colhit_index != INF){
                if(hdr.kv.op_code == OP_GET){
                    reflect();
                    set_bridge(HEADER_TYPE_BRIDGE); //attention!!!!!!!!
                    modify_service_op(OP_GETSUCC);
                }
            }

            // multicast to control node
            multicast(3);
            mac_forward.apply();
        }
        else if(hdr.ctrl_opHashInfo.isValid()){
            // get record
            // ...
            send(CONTROL_PLANE_PCIE); //debug
        }
        else{
            mac_forward.apply();
            // send(CONTROL_PLANE_PCIE); //debug
        }
    }
}


/*************************************************************************
 ****************  E G R E S S   P R O C E S S I N G   *******************
 *************************************************************************/

/***************** M A T C H - A C T I O N  *********************/

control Egress(
    /* User */
    inout my_egress_headers_t                         hdr,
    inout my_egress_metadata_t                        meta,
    /* Intrinsic */
    in    egress_intrinsic_metadata_t                 eg_intr_md,
    in    egress_intrinsic_metadata_from_parser_t     eg_prsr_md,
    inout egress_intrinsic_metadata_for_deparser_t    eg_dprsr_md,
    inout egress_intrinsic_metadata_for_output_port_t eg_oport_md)
{
    // define value r-mat
    VALUE_RMAT_NEW()

    apply{
        if(hdr.kv.isValid()){
            if(hdr.kv.op_code == OP_GET){
                VALUE_READ_COLS_RUN()
                hdr.kv_data.setValid();
            }
            if(eg_intr_md.egress_port == CONTROL_PLANE_ETH0){
                hdr.udp.src_port = (bit<16>)meta.kv.hash_index;
            }
        }
        else if(hdr.ctrl_valueInfo.isValid()){
            VALUE_MODIFY_COLS_RUN()
        }
    }
}


/************ F I N A L   P A C K A G E ******************************/

Pipeline(
    IngressParser(),
    Ingress(),
    IngressDeparser(),
    EgressParser(),
    Egress(),
    EgressDeparser()
) pipe;

Switch(pipe) main;
