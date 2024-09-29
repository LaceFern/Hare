#include <core.p4>
#include <tna.p4>

#include "types.p4"
#include "config.p4"
#include "header.p4"
#include "parser.p4"

#include "control.p4"
#include "precise_counter.p4"
#include "sketch.p4"

#include "kvs.p4"


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
    // control
    action mapping(bit<IDX_WIDTH> index){
        hdr.bridge.egress_bridge_md.index = index;
    }
    table index_mapping_table{
        key = {
            hdr.kv.key : exact;
        }
        actions = {
            mapping;
            NoAction;
        }
        size = SLOT_SIZE + 500;
        default_action = NoAction;
    }

    // control flag: statistic & application
    STATISTIC_FLAG()
    SUSPEND_FLAG()

    // direct counter
    PRECISE_COUNTER(hdr.bridge.egress_bridge_md.index)

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

    action modify_control_op(oper_t op_code){
        hdr.ctrl.op_code = op_code;
    }

    action ack_to_controller(){
        hdr.ctrl.ack = 1;
    }

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

    // L2 forward
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

    KeyValueStore() kvs;

    apply{
        // packet should have a bridge header set
        set_bridge(HEADER_TYPE_NORMAL);

        if(hdr.kv.isValid()){

            bit<1> statistic_flag = 0;
            if(index_mapping_table.apply().hit){
                STATISTIC_FLAG_GET(statistic_flag);
                if(statistic_flag == 0){
                    COUNTER_ADD();
                }

                SUSPEND_FLAG_GET(hdr.bridge.egress_bridge_md.suspend_flag, hdr.bridge.egress_bridge_md.index);

                if(hdr.kv.op_code == OP_GET){
                    reflect();
                    set_bridge(HEADER_TYPE_BRIDGE);
                    kvs.apply(
                        hdr.kv.op_code,
                        hdr.bridge.egress_bridge_md.index,
                        hdr.kv_data
                    );
                    if(hdr.kv_data.valid_len != 0 && hdr.bridge.egress_bridge_md.suspend_flag == 0){
                        modify_service_op(OP_GETSUCC);
                    }
                    else{
                        modify_service_op(OP_GETFAIL);
                    }
                }
                else{
                    mac_forward.apply();
                }
            }
            else{
                mac_forward.apply();
            }
        }
        else if(hdr.ctrl.isValid()){
            if(hdr.ctrl.op_code == STAT_FLAG_SET){
                STATISTIC_FLAG_SET();
                reflect();
            }
            else if(hdr.ctrl.op_code == STAT_FLAG_RST){
                STATISTIC_FLAG_RST();
                reflect();
            }
            else if(hdr.ctrl.op_code == SUSP_FLAG_SET){
                SUSPEND_FLAG_SET(hdr.ctrl.index[IDX_WIDTH - 1:0]);
                reflect();
            }
            else if(hdr.ctrl.op_code == SUSP_FLAG_RST){
                SUSPEND_FLAG_RST(hdr.ctrl.index[IDX_WIDTH - 1:0]);
                reflect();
            }
            else if(hdr.ctrl.op_code == OP_CNT_GET){
                COUNTER_GET(hdr.ctrl.counter, (bit<IDX_WIDTH>) hdr.ctrl.index);
                reflect();
            }
            else if(hdr.ctrl.op_code == OP_CNT_CLR){
                COUNTER_CLR((bit<IDX_WIDTH>) hdr.ctrl.index);
                drop();//reflect();
            }
            else if(hdr.ctrl.op_code == HOT_REPORT){
                send(CONTROL_PLANE);
            }
            else if(hdr.ctrl.op_code == REPLACE_SUCC){
                send(CPU_CONTROLLER);//send(CONTROL_PLANE);//send(CPU_CONTROLLER);//send(FPGA_CONTROLLER);
            }
            else{
                send(CONTROL_PLANE);
            }
        }
        else{
            mac_forward.apply();
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
    // action modify_service_op(oper_t op_code){
    //     hdr.kv.op_code = op_code;
    // }

    // action modify_control_op(oper_t op_code){
    //     hdr.ctrl.op_code = op_code;
    // }
    // KeyValueStore() kvs;
    // apply{
    //     if(hdr.kv.op_code == OP_GET){
    //         kvs.apply(
    //             hdr.kv.op_code,
    //             meta.bridge.egress_bridge_md.index,
    //             hdr.kv_data
    //         );
    //         if(hdr.kv_data.valid_len != 0 && meta.bridge.egress_bridge_md.suspend_flag == 0){
    //             modify_service_op(OP_GETSUCC);
    //         }
    //         else{
    //             modify_service_op(OP_GETFAIL);
    //         }
    //     }
    // }
    apply{}
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
