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
    action mapping(bit<IDX_WIDTH> index)
    {
        hdr.bridge.egress_bridge_md.index = index;
        meta.lockId = hdr.kv.key[31:0];
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

        size = SLOT_SIZE;
        default_action = NoAction;//hard_code_mapping(); // NoAction;
    }

    // control flag: statistic & application
    STATISTIC_FLAG()
    SUSPEND_FLAG()
    CLEAN_FLAG()

    // direct counter
    PRECISE_COUNTER(hdr.bridge.egress_bridge_md.index)
    
    // sketch
    LOCK_SKETCH(hdr.kv.key)
    LOCK_FILTER_SKETCH(hdr.kv.key)

    // forward
    action send(PortId_t port)
    {
        ig_tm_md.ucast_egress_port = port;
    }

    action reflect()
    {
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
        size           = 32;
    }

    KeyValueStore() kvs;

    //bloon filter debug
    bool toCP = false;
    Register<bit<32>, bit<32>>(1) count_reg;
    RegisterAction<
        bit<32>,
        bit<32>,
        bit<32>
    >(count_reg) bf_count = {
        void apply(inout bit<32> register_data, out bit<32> result)
        {
            if(toCP == true){
                register_data = register_data + 1;
            }
        }
    };



    apply
    {
        // packet should have a bridge header set
        // set_bridge(HEADER_TYPE_NORMAL);

        // if(hdr.kv.isValid())
        // {
            bit<1> statistic_flag = 0;

            if(index_mapping_table.apply().hit)
            {
                // // get_statistic_flag();
                // STATISTIC_FLAG_GET(statistic_flag);
                // if(statistic_flag == 0)
                // {
                //     // count direct_counter
                //     // counter_add();
                //     // p_counter.apply(); // add
                //     COUNTER_ADD();
                // }

                // // get_suspend_flag(hdr.bridge.egress_bridge_md.index);
                // SUSPEND_FLAG_GET(hdr.bridge.egress_bridge_md.suspend_flag, hdr.bridge.egress_bridge_md.index);

                if(hdr.kv.op_code == OP_GET){
                    reflect();
                    // set_bridge(HEADER_TYPE_BRIDGE);
                    hdr.kv_data.valid_len = 1;
                    kvs.apply(
                        hdr.kv.op_code,
                        hdr.bridge.egress_bridge_md.index,
                        hdr.kv_data
                    );
                    modify_service_op(OP_GETSUCC);
                    // if(hdr.kv_data.valid_len != 0 && hdr.bridge.egress_bridge_md.suspend_flag == 0){
                    //     modify_service_op(OP_GETSUCC);
                    // }
                    // else{
                    //     modify_service_op(OP_GETFAIL);
                    // }
                }
                else{
                    mac_forward.apply();
                }
            }
            else
            {

                // sample
                bit<1> clean_flag = 0;
                // CLEAN_FLAG_GET(clean_flag);
                if(ig_intr_md.ingress_port == SAMPLE_PORT && clean_flag == 0){
                    bool hot = false;
                    SKETCH_FUNC(hot)
                    // bool toCP = false;
                    if(hot == true){
                        // FILTER_FUNC(toCP)
                        // multicast(2);
                        FILTER_FUNC(toCP)
                        if(toCP){
                            multicast(2);
                        }
                        bf_count.execute(0);  
                    }
                }

                // send to fpga data server
                mac_forward.apply();// hash_partition_forward.apply();
            }
        // }
        // else
        // {
        //     mac_forward.apply();
        // }
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
    apply
    {}
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
