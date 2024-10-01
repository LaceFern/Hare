#ifndef _KVS_P4_
#define _KVS_P4_


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

#define KVS_TABLE(i, op_code)     \
    table kvs_table_##i                      \
    {                                        \
        key = {                              \
            op_code   : exact;               \
        }                                    \
        actions = {                          \
            read_value_##i;                  \
            write_value_##i;                 \
            NoAction;                        \
        }                                    \
        size = 16;                           \
        const entries = {                    \
            OP_GET : read_value_##i();  \
        }                                    \
        default_action = NoAction;           \
    }

#define KVS_SLICE(i, op_code, index, value)\
    KVS_REG(i, value##_##i)\
    KVS_ACTION(i, index, value##_##i)\
    KVS_TABLE(i, op_code)

#define KVS(op_code, index, value)\
    KVS_SLICE(0, op_code, index, value)
//    KVS_SLICE(1, op_code, index, value)\
//    KVS_SLICE(2, op_code, index, value)\
//    KVS_SLICE(3, op_code, index, value)\
//    KVS_SLICE(4, op_code, index, value)\
//    KVS_SLICE(5, op_code, index, value)\
//    KVS_SLICE(6, op_code, index, value)\
//    KVS_SLICE(7, op_code, index, value)\
//    KVS_SLICE(8, op_code, index, value)\
//    KVS_SLICE(9, op_code, index, value)\
//    KVS_SLICE(10, op_code, index, value)\
//    KVS_SLICE(11, op_code, index, value)\
//    KVS_SLICE(12, op_code, index, value)\
//    KVS_SLICE(13, op_code, index, value)\
//    KVS_SLICE(14, op_code, index, value)\
//    KVS_SLICE(15, op_code, index, value)
    // KVS_SLICE(16, op_code, index, value) \
    // KVS_SLICE(17, op_code, index, value) \
    // KVS_SLICE(18, op_code, index, value) \
    // KVS_SLICE(19, op_code, index, value) \
    // KVS_SLICE(20, op_code, index, value) \
    // KVS_SLICE(21, op_code, index, value) \
    // KVS_SLICE(22, op_code, index, value) \
    // KVS_SLICE(23, op_code, index, value) \
    // KVS_SLICE(24, op_code, index, value) \
    // KVS_SLICE(25, op_code, index, value) \
    // KVS_SLICE(26, op_code, index, value) \
    // KVS_SLICE(27, op_code, index, value) \
    // KVS_SLICE(28, op_code, index, value) \
    // KVS_SLICE(29, op_code, index, value) \
    // KVS_SLICE(30, op_code, index, value) \
    // KVS_SLICE(31, op_code, index, value)

#define KVS_TABLE_APPLY()  \
    kvs_table_0.apply();
//    kvs_table_1.apply();   \
//    kvs_table_2.apply();   \
//    kvs_table_3.apply();   \
//    kvs_table_4.apply();   \
//    kvs_table_5.apply();   \
//    kvs_table_6.apply();   \
//    kvs_table_7.apply();   \
//    kvs_table_8.apply();   \
//    kvs_table_9.apply();   \
//    kvs_table_10.apply();  \
//    kvs_table_11.apply();  \
//    kvs_table_12.apply();  \
//    kvs_table_13.apply();  \
//    kvs_table_14.apply();  \
//    kvs_table_15.apply();
    // kvs_table_16.apply();  \
    // kvs_table_17.apply();  \
    // kvs_table_18.apply();  \
    // kvs_table_19.apply();  \
    // kvs_table_20.apply();  \
    // kvs_table_21.apply();  \
    // kvs_table_22.apply();  \
    // kvs_table_23.apply();  \
    // kvs_table_24.apply();  \
    // kvs_table_25.apply();  \
    // kvs_table_26.apply();  \
    // kvs_table_27.apply();  \
    // kvs_table_28.apply();  \
    // kvs_table_29.apply();  \
    // kvs_table_30.apply();  \
    // kvs_table_31.apply()


#endif //_KVS_P4_
