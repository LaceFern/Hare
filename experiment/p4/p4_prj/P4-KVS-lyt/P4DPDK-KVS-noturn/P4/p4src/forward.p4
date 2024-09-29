#ifndef _FORWARD_P4_
#define _FORWARD_P4_


#include "types.p4"
#include "config.p4"
#include "resubmit.p4"

control Forward(
    inout bit<1> op_result,
    inout bit<1> do_mirror,
    inout my_egress_headers_t                         hdr,
    inout my_egress_metadata_t                        meta,
    // Intrinsic
    inout egress_intrinsic_metadata_for_deparser_t    eg_dprsr_md
    // in    ingress_intrinsic_metadata_t              ig_intr_md,
    // in    ingress_intrinsic_metadata_from_parser_t  ig_prsr_md,
    // inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
    // inout ingress_intrinsic_metadata_for_tm_t       ig_tm_md
)
{
    action drop()
    {
        eg_dprsr_md.drop_ctl = 0x7;
    }

    action disable_unicast()
    {
        op_result = 0;
        do_mirror = 0;
    }

    action disable_mirroring()
    {
        do_mirror = 0;
    }

    // forward prepare if meta.egress_mirror.isValid()
    table forward_prepare_table
    {
        key = {
            meta.egress_mirror.release_type : ternary;
            hdr.lock.lock_mode              : exact;
            meta.egress_mirror.isValid()    : exact;
            meta.egress_mirror.mirror_tp    : ternary;
        }

        actions = {
            NoAction;
            disable_unicast;
            disable_mirroring;
        }

        size = 64;
        const entries = {
            // exclusive
            (         _, EXCLU_LOCK, false, EXCLU_LOCK) : NoAction;            // final_mirroring(); // NoAction; // final
            (         _, EXCLU_LOCK, false, SHARE_LOCK) : NoAction;            // may be final, depends on head and tail
            (         _, EXCLU_LOCK, false,          _) : NoAction;            // disable_mirroring(); // counter == 1, notify controller queue is empty
            (EXCLU_LOCK, SHARE_LOCK,  true, EXCLU_LOCK) : disable_mirroring(); // should already be final
            (EXCLU_LOCK, SHARE_LOCK,  true, SHARE_LOCK) : NoAction;            // may be final, depends on head and tail
            (EXCLU_LOCK, SHARE_LOCK,  true,          _) : disable_mirroring(); // reach the tail, should already be final
            (EXCLU_LOCK, EXCLU_LOCK,  true,          _) : disable_mirroring(); // reach the tail, should already be final
            // shared
            (         _, SHARE_LOCK, false, EXCLU_LOCK) : NoAction;            // final_mirroring();
            (         _, SHARE_LOCK, false, SHARE_LOCK) : disable_mirroring(); // should already be final
            (         _, SHARE_LOCK, false,          _) : NoAction;            // disable_mirroring(); // counter == 1, notify controller queue is empty
            (SHARE_LOCK, EXCLU_LOCK,  true,          _) : disable_mirroring(); // reach the tail, should already be final
        }

        default_action = disable_unicast();
    }

    table forward
    {
        key = {
            op_result : exact;
        }

        actions = {
            drop;
            NoAction;
        }
        
        size = 2;
        const entries = {
            0 : drop();
        }
        
        const default_action = NoAction;
    }

    action set_mirror(MirrorId_t mirror_session_id)
    {
        meta.mirror_session = mirror_session_id;
    }

    table mirror
    {
        key = {
            do_mirror                    : exact;
            meta.egress_mirror.mirror_id : ternary;
        }

        actions = {
            set_mirror;
            NoAction;
        }
        
        size = 16;
        const entries = {
            // (0,                 _) : NoAction;
            (1,                        0) : set_mirror(9); // to controller
            (1, (0 << 29) &&& 0xe0000000) : set_mirror(1);
            (1, (1 << 29) &&& 0xe0000000) : set_mirror(2);
            (1, (2 << 29) &&& 0xe0000000) : set_mirror(3);
            (1, (3 << 29) &&& 0xe0000000) : set_mirror(4);
            (1, (4 << 29) &&& 0xe0000000) : set_mirror(5);
            (1, (5 << 29) &&& 0xe0000000) : set_mirror(6);
            (1, (6 << 29) &&& 0xe0000000) : set_mirror(7);
            (1, (7 << 29) &&& 0xe0000000) : set_mirror(8);
        }
        
        const default_action = NoAction;
    }

    action do_egress_mirror()
    {
        eg_dprsr_md.mirror_type = EGRESS_MIRROR;

        meta.egress_mirror.header_type = HEADER_TYPE_MIRROR;
        
        // handle mirror data here
        // e.g.
        meta.egress_mirror.re_mirror = (bit<1>) meta.egress_mirror.isValid();
    }

    apply
    {
        if(do_mirror == 1 /* && meta.egress_mirror.head == meta.tail */)
        {
            forward_prepare_table.apply();
            if(meta.egress_mirror.head == meta.tail) // mirroring has reach the tail [when queue is full]
            {
                if(!(meta.egress_mirror.bridge_md.suspend_flag == 1 && meta.egress_mirror.mirror_op == EMPTY_CALL)) // not satisfied with this implementation
                {
                    disable_mirroring();
                }
            }
            // do mirror
        }
        forward.apply();
        // if(op_result == 0)
        // {
        //     drop();
        // }
        mirror.apply();
        // reply();
                    
        // hdr.lock.re_mirror = meta.egress_mirror.re_mirror;
        // hdr.lock.mirror_op = meta.egress_mirror.mirror_op;
        // hdr.lock.mirror_tp = meta.egress_mirror.mirror_tp;
        // hdr.lock.mirror_id = meta.egress_mirror.mirror_id;

        // hdr.lock.head = meta.egress_mirror.head;
        // hdr.lock.release_type = meta.egress_mirror.release_type;
    }
}


#endif //_FORWARD_P4_
