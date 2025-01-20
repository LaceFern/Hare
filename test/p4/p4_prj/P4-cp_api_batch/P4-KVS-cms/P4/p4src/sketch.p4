#ifndef _SKETCH_P4_
#define _SKETCH_P4_


#include "types.p4"
#include "config.p4" 

#define SKETCH(i)                                                \
    Register<bit<32>, bit<16>>(SKETCH_DEPTH) sketch_reg_##i;     \
    RegisterAction<                                              \
        bit<32>,                                                 \
        bit<16>,                                                 \
        bool                                                     \
    >(sketch_reg_##i) sketch_count_##i = {                       \
        void apply(inout bit<32> register_data, out bool result) \
        {                                                        \
            register_data = register_data + 1;                   \
            if(register_data > THRESHOLD)                        \
                result = true;                                   \
            else                                                 \
                result = false;                                  \
        }                                                        \
    };                                                           \
    RegisterAction<                                              \
        bit<32>,                                                 \
        bit<16>,                                                 \
        bit<32>                                                  \
    >(sketch_reg_##i) sketch_clean_##i = {                       \
        void apply(inout bit<32> register_data)                  \
        {                                                        \
            register_data = 0;                                   \
        }                                                        \
    };                                                           \
    action count_sketch_##i(out bool flag, bit<16> hash)         \
    {                                                            \
        flag = sketch_count_##i.execute(hash);                   \
    }                                                            \
    action clean_sketch_##i(bit<16> index)                       \
    {                                                            \
        sketch_clean_##i.execute(index);                         \
    }

//--------------------bak begins---------------------

#define LOCK_SKETCH(lock_id)                                        \
    Hash<bit<32>>(HashAlgorithm_t.CRC32) count_min_sketch_hash_0;   \
    Hash<bit<32>>(HashAlgorithm_t.CRC64) count_min_sketch_hash_1;   \
    bit<32> sketch_hash_value_0 = 0;                                \
    bit<32> sketch_hash_value_1 = 0;                                \
    bit<16> sketch_index_0 = 0;                                \
    bit<16> sketch_index_1 = 0;                                \
    bit<16> sketch_index_2 = 0;                                \
    bit<16> sketch_index_3 = 0;                                \
    bit<18> filter_sketch_index_0 = 0;                                \
    bit<18> filter_sketch_index_1 = 0;                                \
    bit<18> filter_sketch_index_2 = 0;                                \
    SKETCH(0)                                                       \
    SKETCH(1)                                                       \
    SKETCH(2)                                                       \
    SKETCH(3)                                                       \
    action calculate_hash_0()                                       \
    {                                                               \
        sketch_hash_value_0 = count_min_sketch_hash_0.get(lock_id[31:0]); \
        sketch_index_0 = (bit<16>)sketch_hash_value_0[15:0]; \
        sketch_index_1 = (bit<16>)sketch_hash_value_0[31:16]; \
    }                                                               \
    action calculate_hash_1()                                       \
    {                                                               \
        sketch_hash_value_1 = count_min_sketch_hash_1.get(lock_id[31:0]); \
        sketch_index_2 = (bit<16>)sketch_hash_value_1[15:0]; \
        sketch_index_3 = (bit<16>)sketch_hash_value_1[31:16]; \
        filter_sketch_index_0 = (bit<18>)sketch_hash_value_0[17:0]; \
        filter_sketch_index_1 = (bit<18>)sketch_hash_value_0[31:14]; \ 
        filter_sketch_index_2 = (bit<18>)sketch_hash_value_1[17:0]; \
    }

#define SKETCH_FUNC(result)                                                 \
    bool sketch_count_flag_0 = false;                                       \
    bool sketch_count_flag_1 = false;                                       \
    bool sketch_count_flag_2 = false;                                       \
    bool sketch_count_flag_3 = false;                                       \
    result = false;                                                         \
    calculate_hash_0();                                                     \
    calculate_hash_1();                                                     \
    count_sketch_0(sketch_count_flag_0, sketch_index_0); \
    count_sketch_1(sketch_count_flag_1, sketch_index_1); \
    count_sketch_2(sketch_count_flag_2, sketch_index_2);  \
    count_sketch_3(sketch_count_flag_3, sketch_index_3); \
    if(                                                                     \
        sketch_count_flag_0 &&                                              \
        sketch_count_flag_1 &&                                              \
        sketch_count_flag_2 &&                                              \
        sketch_count_flag_3                                                 \
    )                                                                       \
        result = true;

// #define LOCK_SKETCH(lock_id)                                        \
//     Hash<bit<32>>(HashAlgorithm_t.CRC32) count_min_sketch_hash_0;   \
//     Hash<bit<32>>(HashAlgorithm_t.CRC64) count_min_sketch_hash_1;   \
//     bit<32> sketch_hash_value_0 = 0;                                \
//     bit<32> sketch_hash_value_1 = 0;                                \
//     SKETCH(0)                                                       \
//     SKETCH(1)                                                       \
//     SKETCH(2)                                                       \
//     SKETCH(3)                                                       \
//     action calculate_hash_0()                                       \
//     {                                                               \
//         sketch_hash_value_0 = 5; \
//     }                                                               \
//     action calculate_hash_1()                                       \
//     {                                                               \
//         sketch_hash_value_1 = count_min_sketch_hash_1.get(lock_id[31:0]); \
//     }

// #define SKETCH_FUNC(result)                                                 \
//     bool sketch_count_flag_0 = false;                                       \
//     bool sketch_count_flag_1 = false;                                       \
//     bool sketch_count_flag_2 = false;                                       \
//     bool sketch_count_flag_3 = false;                                       \
//     result = false;                                                         \
//     calculate_hash_0();                                                     \
//     calculate_hash_1();                                                     \
//     count_sketch_0(sketch_count_flag_0, 2w0 ++ sketch_hash_value_0[13:0]); \
//     count_sketch_1(sketch_count_flag_1, 2w0 ++ sketch_hash_value_0[29:16]); \
//     count_sketch_2(sketch_count_flag_2, 2w0 ++ sketch_hash_value_1[13:0]);  \
//     count_sketch_3(sketch_count_flag_3, 2w0 ++ sketch_hash_value_1[29:16]); \
//     if(                                                                     \
//         sketch_count_flag_0 &&                                              \
//         sketch_count_flag_1 &&                                              \
//         sketch_count_flag_2 &&                                              \
//         sketch_count_flag_3                                                 \
//     )                                                                       \
//         result = true;
//--------------------bak ends---------------------


//-----------------------------------------------------------------------------------

#define FILTER_SKETCH(i)                                                        \
    Register<bit<8>, bit<18>>(FILTER_SKETCH_DEPTH) filter_sketch_reg_##i;       \
    RegisterAction<                                                             \
        bit<8>,                                                                 \
        bit<18>,                                                                \
        bool                                                                    \
    >(filter_sketch_reg_##i) filter_sketch_update_##i = {                       \
        void apply(inout bit<8> register_data, out bool result) \
        {                                                       \
            if(register_data == 0){                             \
                register_data = 1;                              \
                result = true;                                  \
            }                                                   \
            else                                                \
                result = false;                                 \
        }                                                       \
    };                                                          \
    RegisterAction<                                                             \
        bit<8>,                                                                 \
        bit<18>,                                                                \
        bit<8>                                                                  \
    >(filter_sketch_reg_##i) filter_sketch_clean_##i = {                        \
        void apply(inout bit<8> register_data)                      \
        {                                                           \
            register_data = 0;                                      \
        }                                                           \
    };                                                              \
    action update_filter_sketch_##i(out bool flag, bit<18> hash)         \
    {                                                                   \
        flag = filter_sketch_update_##i.execute(hash);                  \
    }                                                                   \
    action clean_filter_sketch_##i(bit<18> index)                       \
    {                                                                   \
        filter_sketch_clean_##i.execute(index);                         \
    }

#define LOCK_FILTER_SKETCH(lock_id)                                     \
    FILTER_SKETCH(0)                                                    \
    FILTER_SKETCH(1)                                                    \
    FILTER_SKETCH(2)                                                    

#define FILTER_FUNC(result)                                           \
    bool filter_flag_0 = false;                                       \
    bool filter_flag_1 = false;                                       \
    bool filter_flag_2 = false;                                       \
    result = false;                                                   \
    update_filter_sketch_0(filter_flag_0, filter_sketch_index_0); \
    update_filter_sketch_1(filter_flag_1, filter_sketch_index_1);\
    update_filter_sketch_2(filter_flag_2, filter_sketch_index_2); \
    if(                                                               \
        filter_flag_0 &&                                              \
        filter_flag_1 &&                                              \
        filter_flag_2                                                 \
    )                                                                 \
        result = true;



// #define LOCK_FILTER_SKETCH(lock_id)                                     \
//     Hash<bit<32>>(HashAlgorithm_t.CRC32) filter_sketch_hash_0;          \
//     Hash<bit<32>>(HashAlgorithm_t.CRC64) filter_sketch_hash_1;          \
//     bit<32> filter_sketch_hash_value_0 = 0;                             \
//     bit<32> filter_sketch_hash_value_1 = 0;                             \
//     FILTER_SKETCH(0)                                                    \
//     FILTER_SKETCH(1)                                                    \
//     FILTER_SKETCH(2)                                                    \
//     action filter_calculate_hash_0()                                    \
//     {                                                                   \
//         filter_sketch_hash_value_0 = filter_sketch_hash_0.get(lock_id); \
//     }                                                                   \
//     action filter_calculate_hash_1()                                    \
//     {                                                                   \
//         filter_sketch_hash_value_1 = filter_sketch_hash_1.get(lock_id); \
//     }

// #define FILTER_FUNC(result)                                           \
//     bool filter_flag_0 = false;                                       \
//     bool filter_flag_1 = false;                                       \
//     bool filter_flag_2 = false;                                       \
//     result = false;                                                   \
//     filter_calculate_hash_0();                                        \
//     filter_calculate_hash_1();                                        \
//     update_filter_sketch_0(filter_flag_0, 2w0 ++ filter_sketch_hash_value_0[13:0]); \
//     update_filter_sketch_1(filter_flag_1, 2w0 ++ filter_sketch_hash_value_0[29:16]);\
//     update_filter_sketch_2(filter_flag_2, 2w0 ++ filter_sketch_hash_value_1[13:0]); \
//     if(                                                               \
//         filter_flag_0 &&                                              \
//         filter_flag_1 &&                                              \
//         filter_flag_2                                                 \
//     )                                                                 \
//         result = true;



// #define FILTER(i)                                                                 \
//     Register<bit<1>, bit<16>>(FILTER_DEPTH) filter_reg_##i;                       \
//     bit<16> filter_hash_value_##i = 0;                                            \
//     bit<1> filter_count_value_##i = 0xffffffff;                                   \
//     RegisterAction<                                                               \
//         bit<1>,                                                                   \
//         bit<16>,                                                                  \
//         bit<1>                                                                    \
//     >(filter_reg_##i) filter_set_##i = {                                          \
//         void apply(inout bit<1> register_data, out bit<1> result)                 \
//         {                                                                         \
//             register_data = 1;                                                    \
//             result = register_data;                                               \
//         }                                                                         \
//     };                                                                            \
//     RegisterAction<                                                               \
//         bit<32>,                                                                  \
//         bit<16>,                                                                  \
//         bit<32>                                                                   \
//     >(filter_reg_##i) filter_rst_##i = {                                          \
//         void apply(inout bit<1> register_data)                                    \
//         {                                                                         \
//             register_data = 0;                                                    \
//         }                                                                         \
//     };                                                                            \
//     action count_sketch_##i()                                                     \
//     {                                                                             \
//         filter_count_value_##i = filter_set_##i.execute(sketch_hash_value_##i);   \
//     }                                                                             \
//     action clean_sketch_##i(bit<IDX_WIDTH> index)                                 \
//     {                                                                             \
//         filter_rst_##i.execute(index);                                            \
//     }

// control LockSketch(
//     in  bit<4>  op_code,
//     in  bit<32> lock_id,
//     // in  bit<32> count_threshold,
//     // out bit<1>  exceed.
//     out bit<32> lock_count
// )
// {
//     // Hash<bit<64>>(HashAlgorithm_t.CRC64) count_min_sketch_hash;
//     // bit<64> hash_value = 0;

//     // SKETCH(0)
//     // SKETCH(1)
//     // SKETCH(2)
//     // SKETCH(3)

//     // action calculate_hash()
//     // {
//     //     hash_value = count_min_sketch_hash.get(lock_id);

//     //     hash_value_0 = hash_value[15:0];
//     //     hash_value_1 = hash_value[31:16];
//     //     hash_value_2 = hash_value[47:32];
//     //     hash_value_3 = hash_value[63:48];
//     // }

//     LOCK_SKETCH(op_code, lock_id, lock_count)

//     // action do_substract()
//     // {
//     //     diff_0 = hash_value_0 - count_threshold;
//     //     diff_1 = hash_value_1 - count_threshold;
//     //     diff_2 = hash_value_2 - count_threshold;
//     //     diff_3 = hash_value_3 - count_threshold;
//     // }

//     apply
//     {
//         // calculate_hash();
//         // count_sketch_0();
//         // count_sketch_1();
//         // count_sketch_2();
//         // count_sketch_3();

//         //     lock_count = hash_value_0;
//         // if (lock_count > hash_value_1)
//         //     lock_count = hash_value_1;
//         // if (lock_count > hash_value_2)
//         //     lock_count = hash_value_2;
//         // if (lock_count > hash_value_3)
//         //     lock_count = hash_value_3;

//         // do_substract();

//         // if(diff_0 > 0 &&
//         //    diff_1 > 0 &&
//         //    diff_2 > 0 &&
//         //    diff_3 > 0)
//         // {
//         //     exceed = 1;
//         // }

//         // find min
//         // bit<32> n11 = 0;
//         // bit<32> n12 = 0;

//         // if(sketch_count_0 < sketch_count_1)
//         // {
//         //     n11 = sketch_count_0;
//         // }
//         // else
//         // {
//         //     n11 = sketch_count_1;
//         // }

//         // if(sketch_count_2 < sketch_count_3)
//         // {
//         //     n12 = sketch_count_2;
//         // }
//         // else
//         // {
//         //     n12 = sketch_count_3;
//         // }

//         // if(n11 < n12)
//         // {
//         //     lock_count = n11;
//         // }
//         // else
//         // {
//         //     lock_count = n12;
//         // }

//         SKETCH_FUNC(op_code, lock_id, lock_count)
//     }
// }


#endif //_SKETCH_P4_