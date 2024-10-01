#include <core.p4>
#include <tna.p4>

#include "types.p4"
#include "config.p4"

#include "mapping.p4"
#include "header.p4"
#include "parser.p4"
#include "precise_counter.p4"

/* USER_REGION_0 begins */
/* USER_REGION_0 ends */


control Ingress(
    /* User */
    inout my_ingress_headers_t                      hdr,
    inout my_ingress_metadata_t                     meta,
    /* Intrinsic */
    in    ingress_intrinsic_metadata_t              ig_intr_md,
    in    ingress_intrinsic_metadata_from_parser_t  ig_prsr_md,
    inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
    inout ingress_intrinsic_metadata_for_tm_t       ig_tm_md){


    /* obj_2_obj_index_mapping */
    MAPPING_TABLE_AND_ACTION_START_ALSO_END(0)

//    MAPPING_TABLE_AND_ACTION_START(0)
//    MAPPING_TABLE_AND_ACTION_INTER(1, 0)
//    MAPPING_TABLE_AND_ACTION_INTER(2, 1)
//    MAPPING_TABLE_AND_ACTION_END(3, 2)

//    action obj_2_obj_index_mapping_action(bit<OBJIDX_WIDTH> index){
//        meta.app.index = index;
//    }
//    table obj_2_obj_index_mapping_table_0{
//            key = {
//                    hdr.app.objseg_0 : exact;
//            }
//            actions = {
//                obj_2_obj_index_mapping_action;
//                NoAction;
//            }
//            size = CACHE_SIZE + 500;
//            default_action = NoAction;
//    }



    /* statistic_flag */
    Register<bit<1>, bit<1>>(1) statistic_flag_reg;               
    RegisterAction<bit<1>, bit<1>, bit<1>>(statistic_flag_reg) statistic_flag_get = {                  
        void apply(inout bit<1> register_data, out bit<1> result) {                                                         
            result = register_data;                               
        }                                                         
    };                                                            
    RegisterAction<bit<1>, bit<1>, bit<1>>(statistic_flag_reg) statistic_flag_set = {                  
        void apply(inout bit<1> register_data){                                                         
            register_data = 1;                                    
        }                                                         
    };                                                            
    RegisterAction<bit<1>, bit<1>, bit<1>>(statistic_flag_reg) statistic_flag_rst = {                  
        void apply(inout bit<1> register_data){                                                         
            register_data = 0;                                    
        }                                                         
    };                                                            
    action get_statistic_flag(out bit<1> flag){                                                             
        flag = statistic_flag_get.execute(0);                     
    }                                                             
    action set_statistic_flag(){                                                             
        statistic_flag_set.execute(0);                            
    }                                                             
    action rst_statistic_flag(){                                                             
        statistic_flag_rst.execute(0);                            
    }


    /* state_manager */
    Register<bit<STATE_WIDTH>, bit<OBJIDX_WIDTH>>(CACHE_SIZE) state_reg;
    RegisterAction<bit<STATE_WIDTH>, bit<OBJIDX_WIDTH>, bit<STATE_WIDTH>>(state_reg) state_get = {
        void apply(inout bit<STATE_WIDTH> register_data, out bit<STATE_WIDTH> result){
            result = register_data;
        }
    };
    RegisterAction<bit<STATE_WIDTH>, bit<OBJIDX_WIDTH>, bit<STATE_WIDTH>>(state_reg) state_set = {
        void apply(inout bit<STATE_WIDTH> register_data){
            register_data = 1;
        }
    };
    RegisterAction<bit<STATE_WIDTH>, bit<OBJIDX_WIDTH>, bit<STATE_WIDTH>>(state_reg) state_rst = {
        void apply(inout bit<STATE_WIDTH> register_data){
            register_data = 0;
        }
    };
    action get_state(out bit<STATE_WIDTH> state, bit<OBJIDX_WIDTH> index){
        state = state_get.execute(index);
    }
    action set_state(bit<OBJIDX_WIDTH> index){
        state_set.execute(index);                                   
    }                                                                      
    action rst_state(bit<OBJIDX_WIDTH> index){
        state_rst.execute(index);                                   
    }


    /* precise counter */
//    PRECISE_COUNTER(meta.app.index)
    Register<bit<COUNTER_WIDTH>, bit<OBJIDX_WIDTH>>(CACHE_SIZE) counter_reg;
    RegisterAction<bit<COUNTER_WIDTH>, bit<OBJIDX_WIDTH>, bit<COUNTER_WIDTH>>(counter_reg) counter_add = {
        void apply(inout bit<COUNTER_WIDTH> register_data) {
            register_data = register_data + 1;
        }
    };
    RegisterAction<bit<COUNTER_WIDTH>, bit<OBJIDX_WIDTH>, bit<COUNTER_WIDTH>>(counter_reg) counter_get = {
        void apply(inout bit<COUNTER_WIDTH> register_data, out bit<COUNTER_WIDTH> result){                                                                                 \
            result = register_data;
        }
    };
    RegisterAction<bit<COUNTER_WIDTH>, bit<OBJIDX_WIDTH>, bit<COUNTER_WIDTH>>(counter_reg) counter_clr = {
        void apply(inout bit<COUNTER_WIDTH> register_data) {
            register_data = 0;
        }
    };
    action add_counter(bit<OBJIDX_WIDTH> index){
        counter_add.execute(index);
    }
    action get_counter(out bit<COUNTER_WIDTH> value, bit<OBJIDX_WIDTH> index){                                                                                     \
        value = counter_get.execute(index);
    }
    action clr_counter(bit<OBJIDX_WIDTH> index){
        counter_clr.execute(index);
    }


    /* packet forward */
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


    /* USER_REGION_4 begins */
    /* USER_REGION_4 ends */

    apply{
        if(hdr.app.objseg_0.isValid()){

            mapping_table_0.apply();
//            mapping_table_1.apply();
//            mapping_table_2.apply();
//            mapping_table_3.apply();

            get_statistic_flag(meta.app.statistic_flag);

            if(meta.app.index != (bit<OBJIDX_WIDTH>) INVALID_4B){
                get_state(meta.app.state, meta.app.index);
                if(meta.app.statistic_flag == 0){
                    // COUNTER_ADD();
                    add_counter(meta.app.index);
                }
                if(meta.app.state == STATE_UPD){
                    drop();
                }

                /* USER_REGION_5 begins */
                /* USER_REGION_5 ends */
            }
            else{
                mac_forward.apply();
            }
        }
        else if(hdr.app_ex.value_0.isValid()){
            /* USER_REGION_6 begins */
            /* USER_REGION_6 ends */
        }
        else if(hdr.ctrl.isValid()){
            if(hdr.ctrl.op_code == OP_LOCK_STAT_SWITCH){
                set_statistic_flag();
//                reflect();
            }
            else if(hdr.ctrl.op_code == OP_UNLOCK_STAT_SWITCH){
                rst_statistic_flag();
//                reflect();
            }
            else if(hdr.ctrl.op_code == OP_MOD_STATE_UPD_SWITCH){
                set_state(hdr.ctrl.index);
//                reflect();
            }
            else if(hdr.ctrl.op_code == OP_MOD_STATE_NOUPD_SWITCH){
                rst_state(hdr.ctrl.index);
//                reflect();
            }
            else if(hdr.ctrl.op_code == OP_GET_COLD_COUNTER_SWITCH){
                // COUNTER_GET(hdr.ctrl.counter, hdr.ctrl.index);
                get_counter(hdr.ctrl.counter_0, hdr.ctrl.index);
//                reflect();
            }
            else if(hdr.ctrl.op_code == OP_CLN_COLD_COUNTER_SWITCH){
                // COUNTER_CLR(hdr.ctrl.index);
                clr_counter(hdr.ctrl.index);
//                reflect();
            }
//            else{
//                mac_forward.apply();
//            }
            mac_forward.apply();
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
