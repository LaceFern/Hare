#ifndef _KVS_P4_
#define _KVS_P4_


#define KVS_REG(i, value)                                                         \
    Register<bit<VAL_WIDTH>, bit<IDX_WIDTH>>(SLOT_SIZE) kvs_reg_##i;              \
    RegisterAction<                                                               \
        bit<VAL_WIDTH>,                                                           \
        bit<IDX_WIDTH>,                                                           \
        bit<VAL_WIDTH>                                                            \
    >(kvs_reg_##i) value_read_##i = {                                             \
        void apply(inout bit<VAL_WIDTH> register_data, out bit<VAL_WIDTH> result) \
        {                                                                         \
            result = register_data;                                               \
        }                                                                         \
    };                                                                            \
    RegisterAction<                                                               \
        bit<VAL_WIDTH>,                                                           \
        bit<IDX_WIDTH>,                                                           \
        bit<VAL_WIDTH>                                                            \
    >(kvs_reg_##i) value_write_##i = {                                            \
        void apply(inout bit<VAL_WIDTH> register_data)                            \
        {                                                                         \
            register_data = value;                                                \
        }                                                                         \
    }

#define KVS_ACTION(i, index, value)            \
    action read_value_##i()                    \
    {                                          \
        value = value_read_##i.execute(index); \
    }                                          \
    action write_value_##i()                   \
    {                                          \
        value_write_##i.execute(index);        \
    }

#define KVS_TABLE(i, op_code, valid_len)     \
    table kvs_table_##i                      \
    {                                        \
        key = {                              \
            op_code   : exact;               \
            valid_len : ternary;             \
        }                                    \
        actions = {                          \
            read_value_##i;                  \
            write_value_##i;                 \
            NoAction;                        \
        }                                    \
        size = 16;                           \
        const entries = {                    \
            (OP_GET, 0) : NoAction;          \
            (OP_GET, _) : read_value_##i();  \
            (OP_PUT, _) : write_value_##i(); \
        }                                    \
        default_action = NoAction;           \
    }

#define KVS_SLICE(i, op_code, index, valid_len, value) \
    KVS_REG(i, value##_##i);                           \
    KVS_ACTION(i, index, value##_##i)                  \
    KVS_TABLE(i, op_code, valid_len)

#define KVS(op_code, index, valid_len, value)       \
    KVS_SLICE(0, op_code, index, valid_len, value)  \ 
    KVS_SLICE(1, op_code, index, valid_len, value)  \
    KVS_SLICE(2, op_code, index, valid_len, value)  \
    KVS_SLICE(3, op_code, index, valid_len, value)  \
    KVS_SLICE(4, op_code, index, valid_len, value)  \
    KVS_SLICE(5, op_code, index, valid_len, value)  \
    KVS_SLICE(6, op_code, index, valid_len, value)  \
    KVS_SLICE(7, op_code, index, valid_len, value)  \
    KVS_SLICE(8, op_code, index, valid_len, value)  \
    KVS_SLICE(9, op_code, index, valid_len, value)  \
    KVS_SLICE(10, op_code, index, valid_len, value) \
    KVS_SLICE(11, op_code, index, valid_len, value) \
    KVS_SLICE(12, op_code, index, valid_len, value) \
    KVS_SLICE(13, op_code, index, valid_len, value) \
    KVS_SLICE(14, op_code, index, valid_len, value) \
    KVS_SLICE(15, op_code, index, valid_len, value)  \
    KVS_SLICE(16, op_code, index, valid_len, value) \
    KVS_SLICE(17, op_code, index, valid_len, value) \
    KVS_SLICE(18, op_code, index, valid_len, value) \
    KVS_SLICE(19, op_code, index, valid_len, value) \
    KVS_SLICE(20, op_code, index, valid_len, value) \
    KVS_SLICE(21, op_code, index, valid_len, value) \
    KVS_SLICE(22, op_code, index, valid_len, value) \
    KVS_SLICE(23, op_code, index, valid_len, value) \
    KVS_SLICE(24, op_code, index, valid_len, value) \
    KVS_SLICE(25, op_code, index, valid_len, value) \
    KVS_SLICE(26, op_code, index, valid_len, value) \
    KVS_SLICE(27, op_code, index, valid_len, value) \
    KVS_SLICE(28, op_code, index, valid_len, value) \
    KVS_SLICE(29, op_code, index, valid_len, value) \
    KVS_SLICE(30, op_code, index, valid_len, value) \
    KVS_SLICE(31, op_code, index, valid_len, value)

#define KVS_TABLE_APPLY()  \
    kvs_table_0.apply();   \
    kvs_table_1.apply();   \
    kvs_table_2.apply();   \
    kvs_table_3.apply();   \
    kvs_table_4.apply();   \
    kvs_table_5.apply();   \
    kvs_table_6.apply();   \
    kvs_table_7.apply();   \
    kvs_table_8.apply();   \
    kvs_table_9.apply();   \
    kvs_table_10.apply();  \
    kvs_table_11.apply();  \
    kvs_table_12.apply();  \
    kvs_table_13.apply();  \
    kvs_table_14.apply();  \
    kvs_table_15.apply();  \
    kvs_table_16.apply();  \
    kvs_table_17.apply();  \
    kvs_table_18.apply();  \
    kvs_table_19.apply();  \
    kvs_table_20.apply();  \
    kvs_table_21.apply();  \
    kvs_table_22.apply();  \
    kvs_table_23.apply();  \
    kvs_table_24.apply();  \
    kvs_table_25.apply();  \
    kvs_table_26.apply();  \
    kvs_table_27.apply();  \
    kvs_table_28.apply();  \
    kvs_table_29.apply();  \
    kvs_table_30.apply();  \
    kvs_table_31.apply()

#define VALID_LENGTH_REG(valid_len)                                 \
    Register<bit<8>, bit<IDX_WIDTH>>(SLOT_SIZE) valid_length_reg;   \
    RegisterAction<                                                 \
        bit<8>,                                                     \
        bit<IDX_WIDTH>,                                             \
        bit<8>                                                      \
    >(valid_length_reg) get_length = {                              \
        void apply(inout bit<8> register_data, out bit<8> result)   \
        {                                                           \
            result = register_data;                                 \
        }                                                           \
    };                                                              \
    RegisterAction<                                                 \
        bit<8>,                                                     \
        bit<IDX_WIDTH>,                                             \
        bit<8>                                                      \
    >(valid_length_reg) set_length = {                              \
        void apply(inout bit<8> register_data)                      \
        {                                                           \
            register_data = valid_len;                              \
        }                                                           \
    };                                                              \
    RegisterAction<                                                 \
        bit<8>,                                                     \
        bit<IDX_WIDTH>,                                             \
        bit<8>                                                      \
    >(valid_length_reg) rst_length = {                              \
        void apply(inout bit<8> register_data)                      \
        {                                                           \
            register_data = 0;                                      \
        }                                                           \
    }

#define VALID_LENGTH_ACTION(index, valid_len)  \
    action length_get()                        \
    {                                          \
        valid_len = get_length.execute(index); \
    }                                          \
    action length_set()                        \
    {                                          \
        set_length.execute(index);             \
    }                                          \
    action length_rst()                        \
    {                                          \
        rst_length.execute(index);             \
    }

#define VALID_LENGTH_TABLE(op_code)  \
    table valid_length_table         \
    {                                \
        key = {                      \
            op_code : exact;         \
        }                            \
        actions = {                  \
            length_get;              \
            length_set;              \
            length_rst;              \
            NoAction;                \
        }                            \
        size = 16;                   \
        const entries = {            \
            (OP_GET) : length_get(); \
            (OP_PUT) : length_set(); \
            (OP_DEL) : length_rst(); \
        }                            \
        default_action = NoAction;   \
    }

#define VALID_LENGTH(op_code, index, valid_len) \
    VALID_LENGTH_REG(valid_len);                \
    VALID_LENGTH_ACTION(index, valid_len)       \
    VALID_LENGTH_TABLE(op_code)

#define VALID_LENGTH_TABLE_APPLY() \
    valid_length_table.apply()

control KeyValueStore(
    in    oper_t           op_code,
    in    bit<IDX_WIDTH>   index,
    inout kv_data_header_t data
)
{
    // VALID_LENGTH(op_code, index, data.valid_len)
    KVS(op_code, index, data.valid_len, data.value)

    apply
    {
        // VALID_LENGTH_TABLE_APPLY();
        KVS_TABLE_APPLY();
    }
}

#endif //_KVS_P4_
