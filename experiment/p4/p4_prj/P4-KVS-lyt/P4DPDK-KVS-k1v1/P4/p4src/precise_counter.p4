#ifndef _PRECISE_COUNTER_P4_
#define _PRECISE_COUNTER_P4_


#define BUCKET_SIZE (SLOT_SIZE / 2) //SLOT_SIZE//4096 // SLOT_SIZE / (2 ^ BUCKET_WIDTH)
#define BUCKET_SIZE_WIDTH 32 // SLOT_SIZE / (2 ^ BUCKET_WIDTH)
#define BUCKET_MASK 0x1 //0x3 //0x7
#define BUCKET_MASK_WIDTH 1 //2 //3

#define COUNTER_BUCKET(i, index)                                                          \
    Register<bit<32>, bit<BUCKET_IDXWIDTH>>(BUCKET_SIZE) counter_bucket_reg_##i;                \
    RegisterAction<                                                                       \
        bit<32>,                                                                          \
        bit<BUCKET_IDXWIDTH>,                                                                   \
        bit<32>                                                                           \
    >(counter_bucket_reg_##i) counter_add_##i = {                                         \
        void apply(inout bit<32> register_data)                                           \
        {                                                                                 \
            register_data = register_data + 1;                                            \
        }                                                                                 \
    };                                                                                    \
    RegisterAction<                                                                       \
        bit<32>,                                                                          \
        bit<BUCKET_IDXWIDTH>,                                                                   \
        bit<32>                                                                           \
    >(counter_bucket_reg_##i) counter_get_##i = {                                         \
        void apply(inout bit<32> register_data, out bit<32> result)                       \
        {                                                                                 \
            result = register_data;                                                       \
        }                                                                                 \
    };                                                                                    \
    RegisterAction<                                                                       \
        bit<32>,                                                                          \
        bit<BUCKET_IDXWIDTH>,                                                                   \
        bit<32>                                                                           \
    >(counter_bucket_reg_##i) counter_clr_##i = {                                         \
        void apply(inout bit<32> register_data)                                           \
        {                                                                                 \
            register_data = 0;                                                            \
        }                                                                                 \
    };                                                                                    \
    action add_counter_##i()                                                              \
    {                                                                                     \
        counter_add_##i.execute((bit<BUCKET_IDXWIDTH>) index[BUCKET_IDXWIDTH - 1:BUCKET_MASK_WIDTH]); \
    }                                                                                     \
    action get_counter_##i(out bit<32> value, bit<BUCKET_IDXWIDTH> index_ctl)                   \
    {                                                                                     \
        value = counter_get_##i.execute(index_ctl);                                       \
    }                                                                                     \
    action clr_counter_##i(bit<BUCKET_IDXWIDTH> index_ctl)                                      \
    {                                                                                     \
        counter_clr_##i.execute(index_ctl);                                               \
    }                                                                                     \
    table precise_counter_table_##i                                                       \
    {                                                                                     \
        key = {                                                                           \
            index##L1b : exact;                                                              \
        }                                                                                 \
        actions = {                                                                       \
            add_counter_##i;                                                              \
            NoAction;                                                                     \
        }                                                                                 \
        size = 16;                                                                        \
        const entries = {                                                                 \
            (i) : add_counter_##i();                                      \
        }                                                                                 \
        default_action = NoAction;                                                        \
    }

// ##L8b

#define PRECISE_COUNTER(index)  \
    COUNTER_BUCKET(0, index)    \
    COUNTER_BUCKET(1, index)   
    // COUNTER_BUCKET(1, index)   \
    // COUNTER_BUCKET(2, index)   \
    // COUNTER_BUCKET(3, index)   
    // COUNTER_BUCKET(4, index)   \
    // COUNTER_BUCKET(5, index)   \
    // COUNTER_BUCKET(6, index)   \
    // COUNTER_BUCKET(7, index)

    // if(meta.bucketIndexL1b == 0){\
    //     counter_add_0.execute((bit<BUCKET_IDXWIDTH>)(index[BUCKET_IDXWIDTH - 1:BUCKET_MASK_WIDTH]));\
    // }\
    // else{\
    //     counter_add_1.execute((bit<BUCKET_IDXWIDTH>)(index[BUCKET_IDXWIDTH - 1:BUCKET_MASK_WIDTH]));\
    // }


#define COUNTER_ADD(index)\
    precise_counter_table_0.apply(); \
    precise_counter_table_1.apply(); 
    // precise_counter_table_2.apply(); \
    // precise_counter_table_3.apply(); 
    // precise_counter_table_4.apply(); \
    // precise_counter_table_5.apply(); \
    // precise_counter_table_6.apply(); \
    // precise_counter_table_7.apply()

#define COUNTER_GET(counter, index_ctl) \
    counter##_0 = counter_get_0.execute(index_ctl);\
    counter##_1 = counter_get_1.execute(index_ctl);
    
    // get_counter_0(counter##_0, index_ctl);
    // get_counter_0(counter##_0, index_ctl);  
    // get_counter_1(counter##_1, index_ctl);  \
    // get_counter_2(counter##_2, index_ctl);  \
    // get_counter_3(counter##_3, index_ctl);  
    // get_counter_4(counter##_4, index_ctl);  \
    // get_counter_5(counter##_5, index_ctl);  \
    // get_counter_6(counter##_6, index_ctl);  \
    // get_counter_7(counter##_7, index_ctl)

#define COUNTER_CLR(index_ctl) \
    counter_clr_0.execute(index_ctl);\
    counter_clr_1.execute(index_ctl);\
    
    // clr_counter_0(index_ctl);  
    // clr_counter_0(index_ctl);  
    // clr_counter_1(index_ctl);  \
    // clr_counter_2(index_ctl);  \
    // clr_counter_3(index_ctl);  
    // clr_counter_4(index_ctl);  \
    // clr_counter_5(index_ctl);  \
    // clr_counter_6(index_ctl);  \
    // clr_counter_7(index_ctl)


//----------------------------------------------------------------

// control PreciseCounter(
//     bit<4>  op_code,
//     bit<32> index,
//     bit<32> counter0,
//     bit<32> counter1,
//     bit<32> counter2,
//     bit<32> counter3,
//     bit<32> counter4,
//     bit<32> counter5,
//     bit<32> counter6,
//     bit<32> counter7
// )
// {
//     //
//     COUNTER_BUCKET(0)
//     COUNTER_BUCKET(1)
//     COUNTER_BUCKET(2)
//     COUNTER_BUCKET(3)

//     COUNTER_BUCKET(4)
//     COUNTER_BUCKET(5)
//     COUNTER_BUCKET(6)
//     COUNTER_BUCKET(7)

//     apply
//     {
//         precise_counter_table_0.apply();
//         precise_counter_table_1.apply();
//         precise_counter_table_2.apply();
//         precise_counter_table_3.apply();

//         precise_counter_table_4.apply();
//         precise_counter_table_5.apply();
//         precise_counter_table_6.apply();
//         precise_counter_table_7.apply();
//     }
// }


#endif //_PRECISE_COUNTER_P4_
