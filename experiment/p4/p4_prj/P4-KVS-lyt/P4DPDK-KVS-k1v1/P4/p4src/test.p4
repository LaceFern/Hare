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
    action mapping(bit<32> index)
    {
        hdr.bridge.egress_bridge_md.index = index;

        meta.bucketIndex = (bit<BUCKET_IDXWIDTH>)index;
        meta.bucketIndexL1b = (bit<1>)index[0:0];
        meta.suspendIndex = (bit<SUSPEND_IDXWIDTH>)index;

        meta.regDepthIndex = (bit<VALUE_IDXWIDTH>)index[14 : 0];
        meta.hitStageIndex = (bit<VALUE_IDXWIDTH>)index[17 : 15];
    }
    action mapping_append(bit<32> index)
    {
        hdr.bridge.egress_bridge_md.index = index;

        meta.bucketIndex = (bit<BUCKET_IDXWIDTH>)index;
        meta.bucketIndexL1b = (bit<1>)index[0:0];
        meta.suspendIndex = (bit<SUSPEND_IDXWIDTH>)index;

        meta.regDepthIndex = (bit<VALUE_IDXWIDTH>)index[14 : 0];
        meta.hitStageIndex = (bit<VALUE_IDXWIDTH>)index[17 : 15];
    }
    table index_mapping_table
    {
        key = {
            hdr.kv.key : exact;
        }

        actions = {
            mapping;
            NoAction;
        }

        size = SLOT_SIZE + 500;
        default_action = NoAction;//hard_code_mapping(); // NoAction;
    }

    // table index_mapping_table_append
    // {
    //     key = {
    //         hdr.kv.key : exact;
    //     }

    //     actions = {
    //         mapping_append;
    //         NoAction;
    //     }

    //     size = SLOT_SIZE + 500;
    //     default_action = NoAction;//hard_code_mapping(); // NoAction;
    // }

    // control flag: statistic & application
    STATISTIC_FLAG()
    SUSPEND_FLAG()

    // direct counter
    PRECISE_COUNTER(meta.bucketIndex)

    // sketch
    // LOCK_SKETCH(hdr.lock.op_type, hdr.lock.lock_id, hdr.lock_ctl.index, hdr.lock.lock_count)

    // forward
    action send(PortId_t port)
    {
        ig_tm_md.ucast_egress_port = port;
    }

    action reflect()
    {
        // ig_tm_md.ucast_egress_port = ig_intr_md.ingress_port;
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

    action modify_service_op(oper_t op_code)
    {
        hdr.kv.op_code = op_code;
    }

    action modify_control_op(oper_t op_code)
    {
        hdr.ctrl.op_code = op_code;
    }

    action ack_to_controller()
    {
        hdr.ctrl.ack = 1;
    }

    action set_bridge(header_type_t header_type)
    {
        hdr.bridge.setValid();
        hdr.bridge.header_type = header_type;
    }

    action drop()
    {
        ig_dprsr_md.drop_ctl = 1;
    }

    action multicast(MulticastGroupId_t mcast_grp)
    {
        ig_tm_md.mcast_grp_a = mcast_grp;
    }

    // L2 forward
    table mac_forward
    {
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

    // modify!!!!!!!!!
    // KeyValueStore() kvs;
    VALUE_REGS_TABLES(hdr.kv.op_code, hdr.bridge.egress_bridge_md.index, hdr.kv_data.valid_len, hdr.kv_data.value)

    apply
    {
        // packet should have a bridge header set
        // set_bridge(HEADER_TYPE_NORMAL);

        if(hdr.kv.isValid())
        {
            bit<1> statistic_flag = 0;

            if(index_mapping_table.apply().hit)
            {
                // if(index_mapping_table_append.apply().hit){
                    // meta.bucketIndex = (bit<BUCKET_IDXWIDTH>)hdr.bridge.egress_bridge_md.index;
                    // meta.bucketIndexL1b = (bit<1>)hdr.bridge.egress_bridge_md.index[0:0];
                    // meta.suspendIndex = (bit<SUSPEND_IDXWIDTH>)hdr.bridge.egress_bridge_md.index;
    
                    // meta.regDepthIndex = (bit<VALUE_IDXWIDTH>)hdr.bridge.egress_bridge_md.index[14 : 0];
                    // meta.hitStageIndex = (bit<VALUE_IDXWIDTH>)hdr.bridge.egress_bridge_md.index[17 : 15];
                    
                    STATISTIC_FLAG_GET(statistic_flag);
                    if(statistic_flag == 0)
                    {
                        COUNTER_ADD(meta.bucketIndex);
                    }
    
                    SUSPEND_FLAG_GET(hdr.bridge.egress_bridge_md.suspend_flag, meta.suspendIndex);
    
                    if(hdr.kv.op_code == OP_GET){
                        reflect();
                        set_bridge(HEADER_TYPE_BRIDGE);
    
                        // modify!!!!!!!!!
                        // kvs.apply(
                        //     hdr.kv.op_code,
                        //     hdr.bridge.egress_bridge_md.index,
                        //     hdr.kv_data
                        // );
                        VALUE_TABLES_APPLY()
    
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
                // }
            }
            else
            {
                mac_forward.apply();// hash_partition_forward.apply();///////////////////////////////////////////////////////
            }
        }
        else if(hdr.ctrl.isValid())
        {
            meta.bucketIndex = (bit<BUCKET_IDXWIDTH>)hdr.ctrl.index;
            meta.bucketIndexL1b = (bit<1>)hdr.ctrl.index[0:0];

            if(hdr.ctrl.op_code == STAT_FLAG_SET)
            {
                STATISTIC_FLAG_SET();
                reflect();
            }
            else if(hdr.ctrl.op_code == STAT_FLAG_RST)
            {
                STATISTIC_FLAG_RST();
                reflect();
            }
            else if(hdr.ctrl.op_code == SUSP_FLAG_SET)
            {
                meta.suspendIndex = hdr.ctrl.index[SUSPEND_IDXWIDTH - 1:0];
                SUSPEND_FLAG_SET(meta.suspendIndex);
                reflect();
            }
            else if(hdr.ctrl.op_code == SUSP_FLAG_RST) // may come from control plane, send to FPGA controller
            {
                meta.suspendIndex = hdr.ctrl.index[SUSPEND_IDXWIDTH - 1:0];
                SUSPEND_FLAG_RST(meta.suspendIndex);
                reflect();
            }
            else if(hdr.ctrl.op_code == OP_CNT_GET)
            {
                COUNTER_GET(hdr.ctrl.counter, meta.bucketIndex);
                reflect();
            }
            // else if(hdr.ctrl.op_code == OP_CNT_CLR)
            // {
            //     COUNTER_CLR(meta.bucketIndex);
            //     drop();//reflect();
            // }
            else if(hdr.ctrl.op_code == HOT_REPORT)
            {
                send(CONTROL_PLANE);
            }
            else if(hdr.ctrl.op_code == REPLACE_SUCC)
            {
                send(CPU_CONTROLLER);//send(CONTROL_PLANE);//send(CPU_CONTROLLER);//send(FPGA_CONTROLLER);
            }
            else
            {
                send(CONTROL_PLANE);
            }
        }
        else
        {
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
    // VALUE_REGS_TABLES(hdr.kv.op_code, meta.egress_mirror.bridge_md.index, hdr.kv_data.valid_len, hdr.kv_data.value)

    apply
    {
        // if(hdr.kv.isValid())
        // {
        //     meta.regDepthIndex = (bit<VALUE_IDXWIDTH>)meta.egress_mirror.bridge_md.index[14 : 0];
        //     meta.hitStageIndex = (bit<VALUE_IDXWIDTH>)meta.egress_mirror.bridge_md.index[17 : 15];
        //     VALUE_TABLES_APPLY()
        // }
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
