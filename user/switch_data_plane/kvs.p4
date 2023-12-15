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




#endif //_KVS_P4_
