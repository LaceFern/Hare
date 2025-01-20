#ifndef _LOCK_QUEUE_NODE_P4_
#define _LOCK_QUEUE_NODE_P4_


#include "config.p4"

#define NODE_INSTANCE(i, obj, reg_width, str_field, ldr_field)                          \
    Register<bit<reg_width>, bit<IDX_WIDTH>>(SLOT_SIZE) lock_queue_##obj##_reg_##i##;   \
    RegisterAction<                                                                     \
        bit<reg_width>,                                                                 \
        bit<IDX_WIDTH>,                                                                 \
        bit<reg_width>                                                                  \
    >(lock_queue_##obj##_reg_##i) set_##obj##_reg_##i = {                               \
        void apply(inout bit<reg_width> register_data)                                  \
        {                                                                               \
            register_data = str_field;                                                  \
        }                                                                               \
    };                                                                                  \
    RegisterAction<                                                                     \
        bit<reg_width>,                                                                 \
        bit<IDX_WIDTH>,                                                                 \
        bit<reg_width>                                                                  \
    >(lock_queue_##obj##_reg_##i) rst_##obj##_reg_##i = {                               \
        void apply(inout bit<reg_width> register_data)                                  \
        {                                                                               \
            register_data = 0;                                                          \
        }                                                                               \
    };                                                                                  \
    RegisterAction<                                                                     \
        bit<reg_width>,                                                                 \
        bit<IDX_WIDTH>,                                                                 \
        bit<reg_width>                                                                  \
    >(lock_queue_##obj##_reg_##i) get_##obj##_reg_##i = {                               \
        void apply(inout bit<reg_width> register_data, out bit<reg_width> result)       \
        {                                                                               \
            result = register_data;                                                     \
        }                                                                               \
    };                                                                                  \
    action set_##obj##_##i()                                                            \
    {                                                                                   \
        set_##obj##_reg_##i.execute(index);                                             \
    }                                                                                   \
    action rst_##obj##_##i()                                                            \
    {                                                                                   \
        rst_##obj##_reg_##i.execute(index);                                             \
    }                                                                                   \
    action get_##obj##_##i()                                                            \
    {                                                                                   \
        ldr_field = get_##obj##_reg_##i.execute(index);                                 \
    }                                                                                   \
    table lock_queue_##obj##_table_##i                                                  \
    {                                                                                   \
        key = {                                                                         \
            op_code    : exact;                                                         \
            queue_head : ternary;                                                       \
            queue_tail : ternary;                                                       \
        }                                                                               \
        actions = {                                                                     \
            set_##obj##_##i;                                                            \
            rst_##obj##_##i;                                                            \
            get_##obj##_##i;                                                            \
            NoAction;                                                                   \
        }                                                                               \
        size = 32;                                                                      \
        const entries = {                                                               \
            (GRANT_LOCK,                      _, i &&& QUEUE_MASK) : set_##obj##_##i(); \
            (RELEA_LOCK, (i + 1) &&& QUEUE_MASK,                _) : rst_##obj##_##i(); \
            (RELEA_LOCK,       i &&& QUEUE_MASK,                _) : get_##obj##_##i(); \
            (CHECK_HEAD,       i &&& QUEUE_MASK,                _) : get_##obj##_##i(); \
        }                                                                               \
        default_action = NoAction;                                                      \
    }

// #define NODE_TYPE_INSTANCE(i)                                                        \
//     Register<bit<8>, bit<IDX_WIDTH>>(SLOT_SIZE) lock_queue_type_reg_##i;             \
//     RegisterAction<                                                                  \
//         bit<8>,                                                                      \
//         bit<IDX_WIDTH>,                                                              \
//         bit<8>                                                                       \
//     >(lock_queue_type_reg_##i) acquire_type_##i = {                                  \
//         void apply(inout bit<8> register_data)                                       \
//         {                                                                            \
//             register_data = lock_type;                                               \
//         }                                                                            \
//     };                                                                               \
//     RegisterAction<                                                                  \
//         bit<8>,                                                                      \
//         bit<IDX_WIDTH>,                                                              \
//         bit<8>                                                                       \
//     >(lock_queue_type_reg_##i) release_type_##i = {                                  \
//         void apply(inout bit<8> register_data)                                       \
//         {                                                                            \
//             register_data = 0;                                                       \
//         }                                                                            \
//     };                                                                               \
//     RegisterAction<                                                                  \
//         bit<8>,                                                                      \
//         bit<IDX_WIDTH>,                                                              \
//         bit<8>                                                                       \
//     >(lock_queue_type_reg_##i) read_type_##i = {                                     \
//         void apply(inout bit<8> register_data, out bit<8> result)                    \
//         {                                                                            \
//             result = register_data;                                                  \
//         }                                                                            \
//     };                                                                               \
//     action set_type_##i()                                                            \
//     {                                                                                \
//         acquire_type_##i.execute(index);                                             \
//     }                                                                                \
//     action rst_type_##i()                                                            \
//     {                                                                                \
//         release_type_##i.execute(index);                                             \
//     }                                                                                \
//     action get_type_##i()                                                            \
//     {                                                                                \
//         type_##i = read_type_##i.execute(index);                                     \
//     }                                                                                \
//     table lock_queue_type_table_##i                                                  \
//     {                                                                                \
//         key = {                                                                      \
//             op_code    : exact;                                                      \
//             queue_head : ternary;                                                    \
//             queue_tail : ternary;                                                    \
//         }                                                                            \
//         actions = {                                                                  \
//             set_type_##i;                                                            \
//             rst_type_##i;                                                            \
//             get_type_##i;                                                            \
//             NoAction;                                                                \
//         }                                                                            \
//         size = 32;                                                                   \
//         const entries = {                                                            \
//             (GRANT_LOCK,                      _, i &&& QUEUE_MASK) : set_type_##i(); \
//             (RELEA_LOCK, (i + 1) &&& QUEUE_MASK,                _) : rst_type_##i(); \
//             (RELEA_LOCK,       i &&& QUEUE_MASK,                _) : get_type_##i(); \
//             (CHECK_HEAD,       i &&& QUEUE_MASK,                _) : get_type_##i(); \
//         }                                                                            \
//         default_action = NoAction;                                                   \
//     }

// #define NODE_DATA_INSTANCE(i)                                                        \
//     Register<bit<32>, bit<IDX_WIDTH>>(SLOT_SIZE) lock_queue_data_reg_##i;            \
//     RegisterAction<                                                                  \
//         bit<32>,                                                                     \
//         bit<IDX_WIDTH>,                                                              \
//         bit<32>                                                                      \
//     >(lock_queue_data_reg_##i) acquire_data_##i = {                                  \
//         void apply(inout bit<32> register_data)                                      \
//         {                                                                            \
//             register_data = client_id;                                               \
//         }                                                                            \
//     };                                                                               \
//     RegisterAction<                                                                  \
//         bit<32>,                                                                     \
//         bit<IDX_WIDTH>,                                                              \
//         bit<32>                                                                      \
//     >(lock_queue_data_reg_##i) release_data_##i = {                                  \
//         void apply(inout bit<32> register_data)                                      \
//         {                                                                            \
//             register_data = 0;                                                       \
//         }                                                                            \
//     };                                                                               \
//     RegisterAction<                                                                  \
//         bit<32>,                                                                     \
//         bit<IDX_WIDTH>,                                                              \
//         bit<32>                                                                      \
//     >(lock_queue_data_reg_##i) read_data_##i = {                                     \
//         void apply(inout bit<32> register_data, out bit<32> result)                  \
//         {                                                                            \
//             result = register_data;                                                  \
//         }                                                                            \
//     };                                                                               \
//     action set_data_##i()                                                            \
//     {                                                                                \
//         acquire_data_##i.execute(index);                                             \
//     }                                                                                \
//     action rst_data_##i()                                                            \
//     {                                                                                \
//         release_data_##i.execute(index);                                             \
//     }                                                                                \
//     action get_data_##i()                                                            \
//     {                                                                                \
//         data_##i = read_data_##i.execute(index);                                     \
//     }                                                                                \
//     table lock_queue_data_table_##i                                                  \
//     {                                                                                \
//         key = {                                                                      \
//             op_code    : exact;                                                      \
//             queue_head : ternary;                                                    \
//             queue_tail : ternary;                                                    \
//         }                                                                            \
//         actions = {                                                                  \
//             set_data_##i;                                                            \
//             rst_data_##i;                                                            \
//             get_data_##i;                                                            \
//             NoAction;                                                                \
//         }                                                                            \
//         size = 32;                                                                   \
//         const entries = {                                                            \
//             (GRANT_LOCK,                      _, i &&& QUEUE_MASK) : set_data_##i(); \
//             (RELEA_LOCK, (i + 1) &&& QUEUE_MASK,                _) : rst_data_##i(); \
//             (RELEA_LOCK,       i &&& QUEUE_MASK,                _) : get_data_##i(); \
//             (CHECK_HEAD,       i &&& QUEUE_MASK,                _) : get_data_##i(); \
//         }                                                                            \
//         default_action = NoAction;                                                   \
//     }

#define NODE_SELECTOR(i)        \
    bit<8>  type_##i = 0;       \
    bit<32> data_##i = 0;       \
    action grant_succ_##i()     \
    {                           \
        do_mirror = 1;          \
        mirror_op = CHECK_HEAD; \
        mirror_tp = type_##i;   \
        mirror_id = data_##i;   \
    }

// #define NODE(i)           \
//     NODE_SELECTOR(i)      \
//     NODE_TYPE_INSTANCE(i) \
//     NODE_DATA_INSTANCE(i)

#define NODE(i)                                     \
    NODE_SELECTOR(i)                                \
    NODE_INSTANCE(i, type,  8, lock_type, type_##i) \
    NODE_INSTANCE(i, data, 32, client_id, data_##i)

#define NODE_APPLY(i)                  \
    lock_queue_type_table_##i.apply(); \
    lock_queue_data_table_##i.apply()


#endif //_LOCK_QUEUE_NODE_P4_
