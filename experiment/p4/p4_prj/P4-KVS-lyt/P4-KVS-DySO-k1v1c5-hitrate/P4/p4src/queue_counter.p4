#ifndef _QUEUE_COUNTER_P4_
#define _QUEUE_COUNTER_P4_


#include "types.p4"
#include "config.p4"

#define QUEUE_COUNTER(op_code, lock_type, index, queue_counter)                    \
    Register<bit<CNT_WIDTH>, bit<IDX_WIDTH>>(SLOT_SIZE) queue_counter_reg;         \
    RegisterAction<                                                                \
        bit<CNT_WIDTH>,                                                            \
        bit<IDX_WIDTH>,                                                            \
        bit<CNT_WIDTH>                                                             \
    >(queue_counter_reg) queue_counter_count = {                                   \
        void apply(inout bit<CNT_WIDTH> register_data, out bit<CNT_WIDTH> result)  \
        {                                                                          \
            if(register_data < QUEUE_SIZE)                                         \
            {                                                                      \
                register_data = register_data + 1;                                 \
                result = register_data;                                            \
            }                                                                      \
            else                                                                   \
            {                                                                      \
                result = 0;                                                        \
            }                                                                      \
        }                                                                          \
    };                                                                             \
    RegisterAction<                                                                \
        bit<CNT_WIDTH>,                                                            \
        bit<IDX_WIDTH>,                                                            \
        bit<CNT_WIDTH>                                                             \
    >(queue_counter_reg) queue_counter_minus = {                                   \
        void apply(inout bit<CNT_WIDTH> register_data, out bit<CNT_WIDTH> result)  \
        {                                                                          \
            if(register_data > 0)                                                  \
            {                                                                      \
                result = register_data;                                            \
                register_data = register_data - 1;                                 \
            }                                                                      \
            else                                                                   \
            {                                                                      \
                result = 0;                                                        \
            }                                                                      \
        }                                                                          \
    };                                                                             \
    RegisterAction<                                                                \
        bit<CNT_WIDTH>,                                                            \
        bit<IDX_WIDTH>,                                                            \
        bit<CNT_WIDTH>                                                             \
    >(queue_counter_reg) queue_counter_check = {                                   \
        void apply(inout bit<CNT_WIDTH> register_data, out bit<CNT_WIDTH> result)  \
        {                                                                          \
            result = register_data;                                                \
        }                                                                          \
    };                                                                             \
    action count_queue_counter()                                                   \
    {                                                                              \
        queue_counter = queue_counter_count.execute(index);                        \
    }                                                                              \
    action minus_queue_counter()                                                   \
    {                                                                              \
        queue_counter = queue_counter_minus.execute(index);                        \
    }                                                                              \
    action check_queue_counter()                                                   \
    {                                                                              \
        queue_counter = queue_counter_check.execute(index);                        \
    }                                                                              \
    action fetch_queue_counter(out bit<CNT_WIDTH> value, bit<IDX_WIDTH> index_ctl) \
    {                                                                              \
        value = queue_counter_check.execute(index_ctl);                            \
    }                                                                              \
    table queue_counter_table                                                      \
    {                                                                              \
        key = {                                                                    \
            op_code : exact;                                                       \
        }                                                                          \
        actions = {                                                                \
            count_queue_counter;                                                   \
            minus_queue_counter;                                                   \
            check_queue_counter;                                                   \
            @defaultonly NoAction;                                                 \
        }                                                                          \
        size = 16;                                                                 \
        const entries = {                                                          \
            GRANT_LOCK : count_queue_counter();                                    \
            RELEA_LOCK : minus_queue_counter();                                    \
            CHECK_HEAD : check_queue_counter();                                    \
        }                                                                          \
        default_action = NoAction;                                                 \
    }

#define QUEUE_COUNTER_APPLY() \
    queue_counter_table.apply()

#define QUEUE_COUNTER_CHECK(counter, index) \
    fetch_queue_counter(counter, index)

control QueueCounter(
    in    oper_t op_code,
    in    bit<8> lock_type,
    in    bit<IDX_WIDTH> index,
    inout bit<CNT_WIDTH> queue_counter
)
{
    // Register<bit<CNT_WIDTH>, bit<IDX_WIDTH>>(SLOT_SIZE) queue_counter_reg;

    // RegisterAction<
    //     bit<CNT_WIDTH>,
    //     bit<IDX_WIDTH>,
    //     bit<CNT_WIDTH>
    // >(queue_counter_reg) count = {
    //     void apply(inout bit<CNT_WIDTH> register_data, out bit<CNT_WIDTH> result)
    //     {
    //         if(register_data < QUEUE_SIZE)
    //         {
    //             register_data = register_data + 1;
    //             result = register_data;
    //         }
    //         else
    //         {
    //             result = 0;
    //         }
    //     }
    // };

    // RegisterAction<
    //     bit<CNT_WIDTH>,
    //     bit<IDX_WIDTH>,
    //     bit<CNT_WIDTH>
    // >(queue_counter_reg) minus = {
    //     void apply(inout bit<CNT_WIDTH> register_data, out bit<CNT_WIDTH> result)
    //     {
    //         if(register_data > 0)
    //         {
    //             result = register_data;
    //             register_data = register_data - 1;
    //         }
    //         else
    //         {
    //             result = 0;
    //         }
    //     }
    // };

    // RegisterAction<
    //     bit<CNT_WIDTH>,
    //     bit<IDX_WIDTH>,
    //     bit<CNT_WIDTH>
    // >(queue_counter_reg) check = {
    //     void apply(inout bit<CNT_WIDTH> register_data, out bit<CNT_WIDTH> result)
    //     {
    //         result = register_data;
    //     }
    // };

    // action acquire()
    // {
    //     queue_counter = count.execute(index);
    // }

    // action release()
    // {
    //     queue_counter = minus.execute(index);
    // }

    // action getonly()
    // {
    //     queue_counter = check.execute(index);
    // }

    // table queue_counter_table
    // {
    //     key = {
    //         op_code   : exact;
    //     }

    //     actions = {
    //         acquire;
    //         release;
    //         getonly;
    //         @defaultonly NoAction;
    //     }

    //     size = 16;
    //     const entries = {
    //         GRANT_LOCK : acquire();
    //         RELEA_LOCK : release();
    //         CHECK_HEAD : getonly();
    //     }

    //     default_action = NoAction;
    // }

    QUEUE_COUNTER(op_code, lock_type, index, queue_counter)

    apply
    {
        // queue_counter_table.apply();
        QUEUE_COUNTER_APPLY();
    }
}


#endif //_QUEUE_COUNTER_P4_
