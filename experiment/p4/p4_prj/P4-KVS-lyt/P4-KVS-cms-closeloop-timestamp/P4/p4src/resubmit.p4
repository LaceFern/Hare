#ifndef _RESUBMIT_P4_
#define _RESUBMIT_P4_


const bit<3> INGRESS_MIRROR = 1;
const bit<3> EGRESS_MIRROR = 2;

typedef bit<8> header_type_t;

const header_type_t HEADER_TYPE_BRIDGE = 0xB;
const header_type_t HEADER_TYPE_MIRROR = 0xC;
const header_type_t HEADER_TYPE_NORMAL = 0xD;

struct bridge_md_t
{
    // add data you need
    // e.g.
    // @padding bit<1> _pad;
    bit<IDX_WIDTH> index;
    
    bit<7> _pad;
    bit<1> suspend_flag;

    // @padding bit<6> _pad1;
    // MirrorId_t mirror_session;
}

#define INTERNAL_HEADER \
        header_type_t header_type

header internal_h
{
    INTERNAL_HEADER;
}

header bridge_h
{
    INTERNAL_HEADER;
    bridge_md_t egress_bridge_md;
}

header egress_mirror_h
{
    INTERNAL_HEADER;
    // add data you need
    // e.g.
    // @padding bit<1> _pad;
    // bit<IDX_WIDTH> index;
    bridge_md_t bridge_md;
}


#endif //_RESUBMIT_P4_
