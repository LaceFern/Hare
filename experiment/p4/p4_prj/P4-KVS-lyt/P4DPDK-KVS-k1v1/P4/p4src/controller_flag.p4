#ifndef _CONTROLLER_FLAG_P4_
#define _CONTROLLER_FLAG_P4_

#define SUSPEND_SIZE SLOT_SIZE //SLOT_SIZE

#define STATISTIC_FLAG()                                          \
    Register<bit<1>, bit<1>>(1) statistic_flag_reg;               \
    RegisterAction<                                               \
        bit<1>,                                                   \
        bit<1>,                                                   \
        bit<1>                                                    \
    >(statistic_flag_reg) statistic_flag_get = {                  \
        void apply(inout bit<1> register_data, out bit<1> result) \
        {                                                         \
            result = register_data;                               \
        }                                                         \
    };                                                            \
    RegisterAction<                                               \
        bit<1>,                                                   \
        bit<1>,                                                   \
        bit<1>                                                    \
    >(statistic_flag_reg) statistic_flag_set = {                  \
        void apply(inout bit<1> register_data)                    \
        {                                                         \
            register_data = 1;                                    \
        }                                                         \
    };                                                            \
    RegisterAction<                                               \
        bit<1>,                                                   \
        bit<1>,                                                   \
        bit<1>                                                    \
    >(statistic_flag_reg) statistic_flag_rst = {                  \
        void apply(inout bit<1> register_data)                    \
        {                                                         \
            register_data = 0;                                    \
        }                                                         \
    };                                                            \
    action get_statistic_flag(out bit<1> flag)                    \
    {                                                             \
        flag = statistic_flag_get.execute(0);                     \
    }                                                             \
    action set_statistic_flag()                                   \
    {                                                             \
        statistic_flag_set.execute(0);                            \
    }                                                             \
    action rst_statistic_flag()                                   \
    {                                                             \
        statistic_flag_rst.execute(0);                            \
    }

#define STATISTIC_FLAG_GET(flag) \
    get_statistic_flag(flag)

#define STATISTIC_FLAG_SET() \
    set_statistic_flag()

#define STATISTIC_FLAG_RST() \
    rst_statistic_flag()

#define SUSPEND_FLAG()                                                     \
    Register<bit<1>, bit<SUSPEND_IDXWIDTH>>(SUSPEND_SIZE) suspend_flag_reg;          \
    RegisterAction<                                                        \
        bit<1>,                                                            \
        bit<SUSPEND_IDXWIDTH>,                                                    \
        bit<1>                                                             \
    >(suspend_flag_reg) suspend_flag_get = {                               \
        void apply(inout bit<1> register_data, out bit<1> result)          \
        {                                                                  \
            result = register_data;                                        \
        }                                                                  \
    };                                                                     \
    RegisterAction<                                                        \
        bit<1>,                                                            \
        bit<SUSPEND_IDXWIDTH>,                                                    \
        bit<1>                                                             \
    >(suspend_flag_reg) suspend_flag_set = {                               \
        void apply(inout bit<1> register_data)                             \
        {                                                                  \
            register_data = 1;                                             \
        }                                                                  \
    };                                                                     \
    RegisterAction<                                                        \
        bit<1>,                                                            \
        bit<SUSPEND_IDXWIDTH>,                                                    \
        bit<1>                                                             \
    >(suspend_flag_reg) suspend_flag_rst = {                               \
        void apply(inout bit<1> register_data)                             \
        {                                                                  \
            register_data = 0;                                             \
        }                                                                  \
    };                                                                     \
    action get_suspend_flag(out bit<1> flag, bit<SUSPEND_IDXWIDTH> index)         \
    {                                                                      \
        flag = suspend_flag_get.execute(index);                            \
    }                                                                      \
    action set_suspend_flag(bit<SUSPEND_IDXWIDTH> index)                          \
    {                                                                      \
        suspend_flag_set.execute(index);                                   \
    }                                                                      \
    action rst_suspend_flag(bit<SUSPEND_IDXWIDTH> index)                          \
    {                                                                      \
        suspend_flag_rst.execute(index);                                   \
    }

#define SUSPEND_FLAG_GET(flag, index) \
    get_suspend_flag(flag, (bit<SUSPEND_IDXWIDTH>)index)

#define SUSPEND_FLAG_SET(index) \
    set_suspend_flag((bit<SUSPEND_IDXWIDTH>)index)

#define SUSPEND_FLAG_RST(index) \
    rst_suspend_flag((bit<SUSPEND_IDXWIDTH>)index)


#endif //_CONTROLLER_FLAG_P4_
