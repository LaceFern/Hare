// #include <core.p4>
// #include <tna.p4>

// #include "types.p4"
// #include "config.p4"
// #include "header.p4"
// #include "parser.p4"

// #include "control.p4"
// #include "precise_counter.p4"
// #include "sketch.p4"

// #include "kvs.p4"

// //------------------------------DySO--------------------------------
// const ether_type_t ETHERTYPE_IPV4 = 0x0800;     // ipv4
// const ether_type_t ETHERTYPE_PGEN_0 = 0xBFBF;   // pktgen pipe0 - query
// const ether_type_t ETHERTYPE_PGEN_1 = 0xFBFB;   // pktgen pipe1 - control
// const ether_type_t ETHERTYPE_CTRL = 0xDEAD;      // dyso control

// #define FR_CTRL_PLANE 50 // 64 
// #define TO_CTRL_PLANE 51 // 66 

// #define PGEN_PORT_0 68
// #define PGEN_PORT_1 196

// #define REG_WIDTH 32

// #define RECIRC_PORT_0 128 // 31/0
// #define RECIRC_PORT_1 132 // 1/0

// #define REG_LEN 131072
// #define REG_LEN_KEY_IDX 14
// #define REG_LEN_KEY (1<<14)

// #define REG_LEN_REC_IDX 8
// #define REG_LEN_REC (1<<8)
// const bit<32> RoundRobinTheta = 0xFF;

// #define REG_KEY(num) \
// Register<bit<REG_WIDTH>, _>(REG_LEN_KEY) key##num##; \
// RegisterAction<bit<REG_WIDTH>, _, bit<1>>(key##num) key##num##_read = {\
//     void apply(inout bit<REG_WIDTH> val, out bit<1> rv){\
//         if(meta.match_key == val) {\
//             rv = 1;\
//         } else { \
//             rv = 0; \
//         }\
//     }\
// };\
// RegisterAction<bit<REG_WIDTH>, _, bit<REG_WIDTH>>(key##num##) key##num##_update = {\
//     void apply(inout bit<REG_WIDTH> val){\
//         val = meta.update_key##num##;\
//     }\
// };

// #define REG_REC(num) \
// Register<bit<REG_WIDTH>, _>(REG_LEN_REC) rec##num##; \
// RegisterAction<bit<REG_WIDTH>, _, bit<REG_WIDTH>>(rec##num##) rec##num##_read_and_clear = {\
//     void apply(inout bit<REG_WIDTH> val, out bit<REG_WIDTH> rv){\
//         rv = val; \
//         val = 0; \
//     }\
// };\
// RegisterAction<bit<REG_WIDTH>, _, bit<1>>(rec##num##) rec##num##_update = {\
//     void apply(inout bit<REG_WIDTH> val, out bit<1> rv){\
//         if(val == 0) {  \
//             val = meta.packet_sig; \
//             rv = 1; \
//         } else {    \
//             rv = 0; \
//         }   \
//     }\
// };
// //------------------------------DySO--------------------------------

// /*************************************************************************
//  **************  I N G R E S S   P R O C E S S I N G   *******************
//  *************************************************************************/

// /***************** M A T C H - A C T I O N  *********************/

// control Ingress(
//     /* User */
//     inout my_ingress_headers_t                      hdr,
//     inout my_ingress_metadata_t                     meta,
//     /* Intrinsic */
//     in    ingress_intrinsic_metadata_t              ig_intr_md,
//     in    ingress_intrinsic_metadata_from_parser_t  ig_prsr_md,
//     inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
//     inout ingress_intrinsic_metadata_for_tm_t       ig_tm_md)
// {
//     // forward
//     action send(PortId_t port)
//     {
//         ig_tm_md.ucast_egress_port = port;
//     }

//     action reflect()
//     {
//         // ig_tm_md.ucast_egress_port = ig_intr_md.ingress_port;
//         send(ig_intr_md.ingress_port);

//         bit<48> dmac = hdr.ethernet.dst_addr;
//         hdr.ethernet.dst_addr = hdr.ethernet.src_addr;
//         hdr.ethernet.src_addr = dmac;

//         bit<32> dip = hdr.ipv4.dst_addr;
//         hdr.ipv4.dst_addr = hdr.ipv4.src_addr;
//         hdr.ipv4.src_addr = dip;

//         bit<16> dport = hdr.udp.dst_port;
//         hdr.udp.dst_port = hdr.udp.src_port;
//         hdr.udp.src_port = dport;
//     }

//     action modify_service_op(oper_t op_code)
//     {
//         hdr.kv.op_code = op_code;
//     }

//     action modify_control_op(oper_t op_code)
//     {
//         hdr.ctrl.op_code = op_code;
//     }

//     action ack_to_controller()
//     {
//         hdr.ctrl.ack = 1;
//     }

//     action set_bridge(header_type_t header_type)
//     {
//         hdr.bridge.setValid();
//         hdr.bridge.header_type = header_type;
//     }

//     action drop()
//     {
//         ig_dprsr_md.drop_ctl = 1;
//     }

//     action multicast(MulticastGroupId_t mcast_grp)
//     {
//         ig_tm_md.mcast_grp_a = mcast_grp;
//     }

//     // L2 forward
//     table mac_forward
//     {
//         key = {
//             hdr.ethernet.dst_addr : exact;
//         }

//         actions = {
//             send;
//             drop;
//             multicast;
//             @defaultonly NoAction;
//         }

//         default_action = NoAction;
//         size           = IPV4_HOST_SIZE;
//     }

// //------------------------------------------------------------------------
//     /* hash functions */
//     CRCPolynomial<bit<32>>(32w0x04C11DB7, 
//                            false, 
//                            false, 
//                            false, 
//                            32w0xFFFFFFFF,
//                            32w0x00000000
//                            ) CRC32_MPEG;

//     Hash<bit<32>>(HashAlgorithm_t.CRC32) hash_crc32;
//     Hash<bit<32>>(HashAlgorithm_t.CUSTOM, CRC32_MPEG) hash_crc32_mpeg;
//     action get_crc32() {
//         meta.hash_crc32 = hash_crc32.get({ hdr.ipv4.src_addr });
//     }
//     action get_crc32_mpeg() {
//         meta.hash_crc32_mpeg = hash_crc32_mpeg.get({ hdr.ipv4.src_addr });
//     }
//     action get_indices() {
//         meta.key_idx = meta.hash_crc32_mpeg[13:0];
//         meta.rec_idx = meta.hash_crc32_mpeg[13:6];
//     }
//     action get_packet_sig() {
//         meta.packet_sig = meta.hash_crc32_mpeg[5:0] ++ meta.hash_crc32[25:0];
//     }
//     action copy_match() {
//         meta.match_key = hdr.ipv4.src_addr;
//     }
//     action copy_update() {
//         meta.update_idx = hdr.ctrl.index_update; // idx to update keys in RAs
//         meta.update_key0 = hdr.ctrl.key0;
//         meta.update_key1 = hdr.ctrl.key1;
//         meta.update_key2 = hdr.ctrl.key2;
//         meta.update_key3 = hdr.ctrl.key3;
//     }
//     // round-robin of probing
//     Register<bit<32>, bit<1>>(1) reg_idx_to_probe;
//     RegisterAction<bit<32>, bit<1>, bit<32>>(reg_idx_to_probe) reg_idx_to_probe_action = {
//         void apply(inout bit<32> value, out bit<32> result) {
//             /* if value >= nRepoEntry - 1 */
//             if (value >= RoundRobinTheta) {
//                 value = 0;
//             } else {
//                 value = value + 1; // round-robin, modular to nEntry
//             }
//             result = value;
//         }
//     };
//     action copy_probe() {
//         meta.probe_idx = reg_idx_to_probe_action.execute(0);
//     }
//     action copy_probe_idx_to_hdr() {
//         hdr.ctrl.index_probe = meta.probe_idx;
//     }

//     REG_KEY(0)
//     REG_KEY(1)
//     REG_KEY(2)
//     REG_KEY(3)

//     action action_key0_update() {
//         key0_update.execute(meta.update_idx);
//     }
//     action action_key1_update() {
//         key1_update.execute(meta.update_idx);
//     }
//     action action_key2_update() {
//         key2_update.execute(meta.update_idx);
//     }
//     action action_key3_update() {
//         key3_update.execute(meta.update_idx);
//     }

//     Register<bit<32>, bit<1>>(1) reg_hit_number;
//     RegisterAction<bit<32>, bit<1>, bit<32>>(reg_hit_number) reg_hit_number_action = {
//         void apply(inout bit<32> value){
//             value = value + 1;
//         }
//     };

//     Register<bit<32>, bit<1>>(1) reg_total_number;
//     RegisterAction<bit<32>, bit<1>, bit<32>>(reg_total_number) reg_total_number_action = {
//         void apply(inout bit<32> value){
//             value = value + 1;
//         }
//     };

//     apply {
//         // from pipe0 traffic generator
//         // todo: if need to scale up the traffic, then we need to do multicast - check whether the packet was a multicast packet
//         if(ig_intr_md.ingress_port == RECIRC_PORT_0 && hdr.ipv4.isValid()) {    
//             // todo: read the registers, and store signature
//             get_crc32();
//             get_crc32_mpeg();
//             get_indices();
//             get_packet_sig();
//             reg_total_number_action.execute(0); /* debugging */
            
//             copy_match();
//             // cache matching here
//             // first stage - always try
//             // return 0 if miss, 1 if hit 
//             meta.key0_hit = key0_read.execute(meta.key_idx);
//             if(meta.key0_hit == 0) {
//                 meta.key1_hit = key1_read.execute(meta.key_idx);
//             } else if(meta.key0_hit == 1)  {
//                 meta.cache_hit = 1;
//             }

//             if(meta.key1_hit == 0) {
//                 meta.key2_hit = key2_read.execute(meta.key_idx);
//             } else if(meta.key1_hit == 1) {
//                 meta.cache_hit = 1;
//             }

//             if(meta.key2_hit == 0) {
//                 meta.key3_hit = key3_read.execute(meta.key_idx);
//             } else if(meta.key2_hit == 1) {
//                 meta.cache_hit = 1;
//             }

//             if(meta.key3_hit == 1) {
//                 meta.cache_hit = 1;
//             }

//             /* monitoring */
//             // record packet signature here
//             // first stage - always try insert
//             meta.rec0_ins = rec0_update.execute(meta.rec_idx);
//             bit<1> tmp0 = 0;
//             if(meta.rec0_ins == 0) {
//                 meta.rec1_ins = rec1_update.execute(meta.rec_idx);
//             } else {
//                 tmp0 = 1;
//             }

//             bit<1> tmp1 = 0;
//             if(meta.rec1_ins == 0 && tmp0 == 0) {
//                 meta.rec2_ins = rec2_update.execute(meta.rec_idx);
//             } else {
//                 tmp1 = 1;
//             }

//             bit<1> tmp2 = 0;
//             if(meta.rec2_ins == 0 && tmp1 == 0) {
//                 meta.rec3_ins = rec3_update.execute(meta.rec_idx);
//             } else {
//                 tmp2 = 1;
//             }

//             bit<1> tmp3 = 0;
//             if(meta.rec3_ins == 0 && tmp2 == 0) {
//                 meta.rec4_ins = rec4_update.execute(meta.rec_idx);
//             } else {
//                 tmp3 = 1;
//             }
            
//             bit<1> tmp4 = 0;
//             if(meta.rec4_ins == 0 && tmp3 == 0) {
//                 meta.rec5_ins = rec5_update.execute(meta.rec_idx);
//             } else {
//                 tmp4 = 1;
//             }

//             bit<1> tmp5 = 0;
//             if(meta.rec5_ins == 0 && tmp4 == 0) {
//                 meta.rec6_ins = rec6_update.execute(meta.rec_idx);
//             } else {
//                 tmp5 = 1;
//             }

//             bit<1> tmp6 = 0;
//             if(meta.rec6_ins == 0 && tmp5 == 0) {
//                 meta.rec7_ins = rec7_update.execute(meta.rec_idx);
//             } else {
//                 tmp6 = 1;
//             }

//             /* debugging */
//             if (meta.cache_hit == 1) {
//                 reg_hit_number_action.execute(0);
//             }

//             // drop the query packet
//             drop();
//         } 
//         // from pipe1 traffic generator
//         else if(ig_intr_md.ingress_port == PGEN_PORT_1 && hdr.ethernet.ether_type == ETHERTYPE_PGEN_1) {
//             hdr.ethernet.ether_type = ETHERTYPE_CTRL;
//             ig_tm_md.ucast_egress_port = FR_CTRL_PLANE;
//             ig_tm_md.bypass_egress = 1;
//         } 
//         // from port FR_CTRL_PLANE (control plane)
//         else if(ig_intr_md.ingress_port == RECIRC_PORT_1 && hdr.ethernet.ether_type == ETHERTYPE_CTRL) {
//             // todo: update keys and copy record
//             check_update_dummy();
//             copy_update();
//             copy_probe(); /* monitoring */
            
//             if (meta.real_update == 1) {
//                 action_key0_update();
//             }
//             if (meta.real_update == 1) {
//                 action_key1_update();
//             }
//             if (meta.real_update == 1) {
//                 action_key2_update();
//             }
//             if (meta.real_update == 1) {
//                 action_key3_update();
//             }

//             /* monitoring */
//             copy_probe_idx_to_hdr();
//             action_rec0_read_and_clear();
//             action_rec1_read_and_clear();
//             action_rec2_read_and_clear();
//             action_rec3_read_and_clear();
//             action_rec4_read_and_clear();
//             action_rec5_read_and_clear();
//             action_rec6_read_and_clear();
//             action_rec7_read_and_clear();
            
//             ig_tm_md.ucast_egress_port = TO_CTRL_PLANE;
//             ig_tm_md.bypass_egress = 1;
//         }
//     }
// }


// /*************************************************************************
//  ****************  E G R E S S   P R O C E S S I N G   *******************
//  *************************************************************************/

// /***************** M A T C H - A C T I O N  *********************/

// control Egress(
//     /* User */
//     inout my_egress_headers_t                         hdr,
//     inout my_egress_metadata_t                        meta,
//     /* Intrinsic */
//     in    egress_intrinsic_metadata_t                 eg_intr_md,
//     in    egress_intrinsic_metadata_from_parser_t     eg_prsr_md,
//     inout egress_intrinsic_metadata_for_deparser_t    eg_dprsr_md,
//     inout egress_intrinsic_metadata_for_output_port_t eg_oport_md)
// {
//     apply
//     {}
// }


// /************ F I N A L   P A C K A G E ******************************/

// Pipeline(
//     IngressParser(),
//     Ingress(),
//     IngressDeparser(),
//     EgressParser(),
//     Egress(),
//     EgressDeparser()
// ) pipe;

// Switch(pipe) main;
