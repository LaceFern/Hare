#ifndef _SKETCH_P4_
#define _SKETCH_P4_


#include "types.p4"
#include "config.p4"

#define SKETCH(i, clr_index)                                        \
    Register<bit<32>, bit<16>>(SKETCH_DEPTH) sketch_reg_##i;        \
    bit<32> diff_##i = 0;                                           \
    bit<16> hash_value_##i = 0;                                     \
    bit<32> sketch_count_##i = 0xffffffff;                          \
    RegisterAction<                                                 \
        bit<32>,                                                    \
        bit<16>,                                                    \
        bit<32>                                                     \
    >(sketch_reg_##i) count_##i = {                                 \
        void apply(inout bit<32> register_data, out bit<32> result) \
        {                                                           \
            register_data = register_data + 1;                      \
            result = register_data;                                 \
        }                                                           \
    };                                                              \
    RegisterAction<                                                 \
        bit<32>,                                                    \
        bit<16>,                                                    \
        bit<32>                                                     \
    >(sketch_reg_##i) clean_##i = {                                 \
        void apply(inout bit<32> register_data)                     \
        {                                                           \
            register_data = 0;                                      \
        }                                                           \
    };                                                              \
    action count_sketch_##i()                                       \
    {                                                               \
        sketch_count_##i = count_##i.execute(hash_value_##i);       \
    }                                                               \
    action clean_sketch_##i()                                       \
    {                                                               \
        clean_##i.execute(clr_index[IDX_WIDTH - 1:0]);              \
    }

#define LOCK_SKETCH(op_code, lock_id, clr_index, lock_count)    \
    Hash<bit<64>>(HashAlgorithm_t.CRC64) count_min_sketch_hash; \
    bit<64> hash_value = 0;                                     \
    SKETCH(0, clr_index)                                        \
    SKETCH(1, clr_index)                                        \
    SKETCH(2, clr_index)                                        \
    SKETCH(3, clr_index)                                        \
    action calculate_hash()                                     \
    {                                                           \
        hash_value = count_min_sketch_hash.get(lock_id);        \
        hash_value_0 = hash_value[15:0];                        \
        hash_value_1 = hash_value[31:16];                       \
        hash_value_2 = hash_value[47:32];                       \
        hash_value_3 = hash_value[63:48];                       \
    }

#define SKETCH_FUNC(op_code, lock_id, lock_count) \
    calculate_hash();                             \
    count_sketch_0();                             \
    count_sketch_1();                             \
    count_sketch_2();                             \
    count_sketch_3();                             \
    bit<32> n11 = 0;                              \
    bit<32> n12 = 0;                              \
    if (sketch_count_0 < sketch_count_1)          \
    {                                             \
        n11 = sketch_count_0;                     \
    }                                             \
    else                                          \
    {                                             \
        n11 = sketch_count_1;                     \
    }                                             \
    if (sketch_count_2 < sketch_count_3)          \
    {                                             \
        n12 = sketch_count_2;                     \
    }                                             \
    else                                          \
    {                                             \
        n12 = sketch_count_3;                     \
    }                                             \
    if (n11 < n12)                                \
    {                                             \
        lock_count = n11;                         \
    }                                             \
    else                                          \
    {                                             \
        lock_count = n12;                         \
    }

#define SKETCH_READ(op_code, lock_id, lock_count) \
    clean_sketch_0();                             \
    clean_sketch_1();                             \
    clean_sketch_2();                             \
    clean_sketch_3();

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
