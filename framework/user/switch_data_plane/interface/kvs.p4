#ifndef _KVS_P4_
#define _KVS_P4_

#define OP_GET 1
#define OP_PUT 2
#define VALSEG_WIDTH 32 //bit
#define OBJSEG_WIDTH 128 //bit

#define KVS_REG(i, value)                                                         \
    Register<bit<VALSEG_WIDTH>, bit<OBJIDX_WIDTH>>(CACHE_SIZE) kvs_reg_##i;              \
    RegisterAction<                                                               \
        bit<VALSEG_WIDTH>,                                                           \
        bit<OBJIDX_WIDTH>,                                                           \
        bit<VALSEG_WIDTH>                                                            \
    >(kvs_reg_##i) value_read_##i = {                                             \
        void apply(inout bit<VALSEG_WIDTH> register_data, out bit<VALSEG_WIDTH> result) \
        {                                                                         \
            result = register_data;                                               \
        }                                                                         \
    };                                                                            \
    RegisterAction<                                                               \
        bit<VALSEG_WIDTH>,                                                           \
        bit<OBJIDX_WIDTH>,                                                           \
        bit<VALSEG_WIDTH>                                                            \
    >(kvs_reg_##i) value_write_##i = {                                            \
        void apply(inout bit<VALSEG_WIDTH> register_data)                            \
        {                                                                         \
            register_data = value;                                                \
        }                                                                         \
    };

#define KVS_ACTION(i, index, value)            \
    action read_value_##i()                    \
    {                                          \
        value = value_read_##i.execute(index); \
    }                                          \
    action write_value_##i()                   \
    {                                          \
        value_write_##i.execute(index);        \
    }

#define KVS_SLICE(i, op_code, index, value)\
    KVS_REG(i, value##_##i)\
    KVS_ACTION(i, index, value##_##i)

#define KVS(op_code, index, value)\
    KVS_SLICE(0, op_code, index, value)\
    KVS_SLICE(1, op_code, index, value)\
    KVS_SLICE(2, op_code, index, value)\
    KVS_SLICE(3, op_code, index, value)\
    KVS_SLICE(4, op_code, index, value)\
    KVS_SLICE(5, op_code, index, value)\
    KVS_SLICE(6, op_code, index, value)\
    KVS_SLICE(7, op_code, index, value)\
    KVS_SLICE(8, op_code, index, value)\
    KVS_SLICE(9, op_code, index, value)\
    KVS_SLICE(10, op_code, index, value)\
    KVS_SLICE(11, op_code, index, value)\
    KVS_SLICE(12, op_code, index, value)\
    KVS_SLICE(13, op_code, index, value)\
    KVS_SLICE(14, op_code, index, value)\
    KVS_SLICE(15, op_code, index, value)\
    KVS_SLICE(16, op_code, index, value) \
    KVS_SLICE(17, op_code, index, value) \
    KVS_SLICE(18, op_code, index, value) \
    KVS_SLICE(19, op_code, index, value) \
    KVS_SLICE(20, op_code, index, value)

#define GET(i, index, value)\
    value##_##i = value_read##_##i.execute(index);

#define PUT(i, index, value)\
    value_write##_##i.execute(index);

KVS_GET_VALUE(index, value)\
    GET(0, index, value)\
    GET(1, index, value)\
    GET(2, index, value)\
    GET(3, index, value)\
    GET(4, index, value)\
    GET(5, index, value)\
    GET(6, index, value)\
    GET(7, index, value)\
    GET(8, index, value)\
    GET(9, index, value)\
    GET(10, index, value)\
    GET(11, index, value)\
    GET(12, index, value)\
    GET(13, index, value)\
    GET(14, index, value)\
    GET(15, index, value)\
    GET(16, index, value)\
    GET(17, index, value)\
    GET(18, index, value)\
    GET(19, index, value)

KVS_PUT_VALUE(index, value)
    PUT(0, index, value)\
    PUT(1, index, value)\
    PUT(2, index, value)\
    PUT(3, index, value)\
    PUT(4, index, value)\
    PUT(5, index, value)\
    PUT(6, index, value)\
    PUT(7, index, value)\
    PUT(8, index, value)\
    PUT(9, index, value)\
    PUT(10, index, value)\
    PUT(11, index, value)\
    PUT(12, index, value)\
    PUT(13, index, value)\
    PUT(14, index, value)\
    PUT(15, index, value)\
    PUT(16, index, value)\
    PUT(17, index, value)\
    PUT(18, index, value)\
    PUT(19, index, value)

#endif //_KVS_P4_
