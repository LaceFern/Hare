#ifndef _PRECISE_COUNTER_P4_
#define _PRECISE_COUNTER_P4_


#define BUCKET_SIZE 8192 // SLOT_SIZE / (2 ^ BUCKET_WIDTH)
#define BUCKET_SIZE_WIDTH 13 // SLOT_SIZE / (2 ^ BUCKET_WIDTH)
#define BUCKET_MASK 0x3 //0x7
#define BUCKET_MASK_WIDTH 2 //3


#define COUNTER_BUCKET(i, index)                                                          \
    Register<bit<COUNTER_WIDTH>, bit<OBJIDX_WIDTH>>(BUCKET_SIZE) counter_bucket_reg_##i;                \
    RegisterAction<                                                                       \
        bit<COUNTER_WIDTH>,                                                                          \
        bit<OBJIDX_WIDTH>,                                                                   \
        bit<COUNTER_WIDTH>                                                                           \
    >(counter_bucket_reg_##i) counter_add_##i = {                                         \
        void apply(inout bit<COUNTER_WIDTH> register_data)                                           \
        {                                                                                 \
            register_data = register_data + 1;                                            \
        }                                                                                 \
    };                                                                                    \
    RegisterAction<                                                                       \
        bit<COUNTER_WIDTH>,                                                                          \
        bit<OBJIDX_WIDTH>,                                                                   \
        bit<COUNTER_WIDTH>                                                                           \
    >(counter_bucket_reg_##i) counter_get_##i = {                                         \
        void apply(inout bit<COUNTER_WIDTH> register_data, out bit<COUNTER_WIDTH> result)                       \
        {                                                                                 \
            result = register_data;                                                       \
        }                                                                                 \
    };                                                                                    \
    RegisterAction<                                                                       \
        bit<COUNTER_WIDTH>,                                                                          \
        bit<OBJIDX_WIDTH>,                                                                   \
        bit<COUNTER_WIDTH>                                                                           \
    >(counter_bucket_reg_##i) counter_clr_##i = {                                         \
        void apply(inout bit<COUNTER_WIDTH> register_data)                                           \
        {                                                                                 \
            register_data = 0;                                                            \
        }                                                                                 \
    };                                                                                    \
    action add_counter_##i()                                                              \
    {                                                                                     \
        counter_add_##i.execute((bit<OBJIDX_WIDTH>) index[OBJIDX_WIDTH - 1:BUCKET_MASK_WIDTH]); \
    }                                                                                     \
    action get_counter_##i(out bit<COUNTER_WIDTH> value, bit<OBJIDX_WIDTH> index_ctl)                   \
    {                                                                                     \
        value = counter_get_##i.execute(index_ctl);                                       \
    }                                                                                     \
    action clr_counter_##i(bit<OBJIDX_WIDTH> index_ctl)                                      \
    {                                                                                     \
        counter_clr_##i.execute(index_ctl);                                               \
    }                                                                                     \
    table precise_counter_table_##i                                                       \
    {                                                                                     \
        key = {                                                                           \
            index : ternary;                                                              \
        }                                                                                 \
        actions = {                                                                       \
            add_counter_##i;                                                              \
            NoAction;                                                                     \
        }                                                                                 \
        size = 16;                                                                        \
        const entries = {                                                                 \
            (i &&& BUCKET_MASK) : add_counter_##i();                                      \
        }                                                                                 \
        default_action = NoAction;                                                        \
    }

#define PRECISE_COUNTER(index) \
    COUNTER_BUCKET(0, index)   \
    COUNTER_BUCKET(1, index)   \
    COUNTER_BUCKET(2, index)   \
    COUNTER_BUCKET(3, index)   
    // COUNTER_BUCKET(4, index)   \
    // COUNTER_BUCKET(5, index)   \
    // COUNTER_BUCKET(6, index)   \
    // COUNTER_BUCKET(7, index)

#define COUNTER_ADD()                \
    precise_counter_table_0.apply(); \
    precise_counter_table_1.apply(); \
    precise_counter_table_2.apply(); \
    precise_counter_table_3.apply(); 
    // precise_counter_table_4.apply(); \
    // precise_counter_table_5.apply(); \
    // precise_counter_table_6.apply(); \
    // precise_counter_table_7.apply()

#define COUNTER_GET(counter, index_ctl) \
    get_counter_0(counter##_0, index_ctl);  \
    get_counter_1(counter##_1, index_ctl);  \
    get_counter_2(counter##_2, index_ctl);  \
    get_counter_3(counter##_3, index_ctl);  
    // get_counter_4(counter##_4, index_ctl);  \
    // get_counter_5(counter##_5, index_ctl);  \
    // get_counter_6(counter##_6, index_ctl);  \
    // get_counter_7(counter##_7, index_ctl)

#define COUNTER_CLR(index_ctl) \
    clr_counter_0(index_ctl);  \
    clr_counter_1(index_ctl);  \
    clr_counter_2(index_ctl);  \
    clr_counter_3(index_ctl);  
    // clr_counter_4(index_ctl);  \
    // clr_counter_5(index_ctl);  \
    // clr_counter_6(index_ctl);  \
    // clr_counter_7(index_ctl)


#endif //_PRECISE_COUNTER_P4_
