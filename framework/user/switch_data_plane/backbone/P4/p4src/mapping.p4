//
// Created by Alicia on 2023/9/19.
//
#ifndef _MAPPING_P4_
#define _MAPPING_P4_


#define MAPPING_SIZE CACHE_SIZE + 500


#define MAPPING_TABLE_AND_ACTION_START(i) \
action mapping_action_##i(bit<OBJIDX_WIDTH> index){ \
    meta.app.segidx_##i = index;\
}\
table mapping_table_##i{ \
    key = {\
        hdr.app.objseg_##i : exact;\
    }\
    actions = {\
        mapping_action_##i;\
        NoAction;\
    }\
    size = MAPPING_SIZE;\
    default_action = NoAction;\
}


#define MAPPING_TABLE_AND_ACTION_INTER(i, isub1) \
action mapping_action_##i(bit<OBJIDX_WIDTH> index){ \
    meta.app.segidx_##i = index;\
}\
table mapping_table_##i{ \
    key = {\
        hdr.app.objseg_##i : exact;       \
        meta.app.segidx_##isub1 : exact;      \
    }\
    actions = {\
        mapping_action_##i;\
        NoAction;\
    }\
    size = MAPPING_SIZE;\
    default_action = NoAction;\
}


#define MAPPING_TABLE_AND_ACTION_END(i, isub1) \
action mapping_action_##i(bit<OBJIDX_WIDTH> index){ \
    meta.app.index = index;\
}\
table mapping_table_##i{ \
    key = {\
        hdr.app.objseg_##i : exact;       \
        meta.app.segidx_##isub1 : exact;      \
    }\
    actions = {\
        mapping_action_##i;\
        NoAction;\
    }\
    size = MAPPING_SIZE;\
    default_action = NoAction;\
}


#define MAPPING_TABLE_AND_ACTION_START_ALSO_END(i) \
action mapping_action_##i(bit<OBJIDX_WIDTH> index){ \
    meta.app.index = index;\
}\
table mapping_table_##i{ \
    key = {\
        hdr.app.objseg_##i : exact;\
    }\
    actions = {\
        mapping_action_##i;\
        NoAction;\
    }\
    size = MAPPING_SIZE;\
    default_action = NoAction;\
}


#endif