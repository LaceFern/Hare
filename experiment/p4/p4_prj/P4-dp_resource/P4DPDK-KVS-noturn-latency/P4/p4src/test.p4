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

#define DEFINE_MAT_FIRST_IN_EMAT(midx)              \
    action mapping_##midx(bit<IDX_WIDTH> index){    \
        meta.hit_seg_index = index;                 \
        meta.hit_mat_index = midx;                  \
    }                                               \
    table mat_##midx{                               \
        key = {                                     \
            hdr.kv.key_##midx : exact;              \
        }                                           \
        actions = {                                 \
            mapping_##midx;                         \
            NoAction;                               \
        }                                           \
        size = SLOT_SIZE;                           \
        default_action = NoAction;                  \
    }

#define DEFINE_MAT_OTHERS_IN_EMAT(midx)             \
    action mapping_##midx(bit<IDX_WIDTH> index){    \
        meta.hit_seg_index = index;                 \
        meta.hit_mat_index = midx;                  \
    }                                               \
    table mat_##midx{                               \
        key = {                                     \
            meta.hit_seg_index: exact;              \
            hdr.kv.key_##midx : exact;              \
        }                                           \
        actions = {                                 \
            mapping_##midx;                         \
            NoAction;                               \
        }                                           \
        size = SLOT_SIZE;                           \
        default_action = NoAction;                  \
    }

#define DEFINE_MAT_FAKEHEADER_IN_EMAT(midx)         \
    action mapping_##midx(bit<IDX_WIDTH> index){    \
        meta.hit_seg_index = index;                 \
        meta.hit_mat_index = midx;                  \
    }                                               \
    table mat_##midx{                               \
        key = {                                     \
            meta.hit_seg_index: exact;              \
            hdr.kv.key_1 : exact;                   \
        }                                           \
        actions = {                                 \
            mapping_##midx;                         \
            NoAction;                               \
        }                                           \
        size = SLOT_SIZE;                           \
        default_action = NoAction;                  \
    }    

#define APPLY_MAT_IN_EMAT(midx)                     \
    mat_##midx.apply();


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
    action send(PortId_t port)
    {
        ig_tm_md.ucast_egress_port = port;
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

    //------------------------------------------

    DEFINE_MAT_FIRST_IN_EMAT(0)
    DEFINE_MAT_OTHERS_IN_EMAT(1)
    DEFINE_MAT_OTHERS_IN_EMAT(2)
    DEFINE_MAT_FAKEHEADER_IN_EMAT(3)
    DEFINE_MAT_FAKEHEADER_IN_EMAT(4)
    DEFINE_MAT_FAKEHEADER_IN_EMAT(5)
    DEFINE_MAT_FAKEHEADER_IN_EMAT(6)
    DEFINE_MAT_FAKEHEADER_IN_EMAT(7)
    DEFINE_MAT_FAKEHEADER_IN_EMAT(8)
    DEFINE_MAT_FAKEHEADER_IN_EMAT(9)

    apply
    {   
        
        APPLY_MAT_IN_EMAT(0)
        APPLY_MAT_IN_EMAT(1)
        APPLY_MAT_IN_EMAT(2)
        APPLY_MAT_IN_EMAT(3)
        APPLY_MAT_IN_EMAT(4)
        APPLY_MAT_IN_EMAT(5)
        APPLY_MAT_IN_EMAT(6)
        APPLY_MAT_IN_EMAT(7)
        APPLY_MAT_IN_EMAT(8)
        APPLY_MAT_IN_EMAT(9)

        // if(meta.hit_mat_index == 1){
        //     mac_forward.apply();
        // }
        // else{
        //     drop();
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
