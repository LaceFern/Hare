#ifndef _GRANTED_LOCK_P4_
#define _GRANTED_LOCK_P4_


#include "types.p4"
#include "config.p4"

struct lock_register_value_t
{
    bit<16> data;
    bit<16> type;
}

control GrantedLock(
    in  bit<7> op_code,
    in  bit<8> lock_type,
    in  bit<16> client_id,
    in  bit<IDX_WIDTH> index,
    out bit<32> op_result)
{
    // type
    // Register<bit<32>, bit<IDX_WIDTH>>(SLOT_SIZE) lock_type_reg;

    // RegisterAction<
    //     bit<32>,
    //     bit<IDX_WIDTH>,
    //     bit<32>
    // >(lock_type_reg) try_grant = {
    //     void apply(inout bit<32> register_data, out bit<32> result)
    //     {
    //         if(register_data != 0)
    //         {
    //             register_data = lock_type;
    //             result = 1;
    //         }
    //         else
    //         {
    //             result = 0;
    //         }
    //     }
    // };

    // RegisterAction<
    //     bit<32>,
    //     bit<IDX_WIDTH>,
    //     bit<32>
    // >(lock_type_reg) try_release = {
    //     void apply(inout bit<32> register_data, out bit<32> result)
    //     {
    //         if(register_data == lock_type)
    //         {
    //             register_data = 0;
    //             result = 1;
    //         }
    //         else
    //         {
    //             result = 0;
    //         }
    //     }
    // };

    // data
    // Register<bit<32>, bit<IDX_WIDTH>>(SLOT_SIZE) lock_data_reg;

    // RegisterAction<
    //     bit<32>,
    //     bit<IDX_WIDTH>,
    //     bit<32>
    // >(lock_data_reg) grant_exclu = {
    //     void apply(inout bit<32> register_data, out bit<32> result)
    //     {
    //         if(register_data == 0)
    //         {
    //             register_data = client_id;
    //             result = 1;
    //         }
    //         else
    //         {
    //             result = 0;
    //         }
    //     }
    // };

    // RegisterAction<
    //     bit<32>,
    //     bit<IDX_WIDTH>,
    //     bit<32>
    // >(lock_data_reg) grant_share = {
    //     void apply(inout bit<32> register_data, out bit<32> result)
    //     {
    //         register_data = register_data + 1;
    //         result = register_data;
    //     }
    // };

    // RegisterAction<
    //     bit<32>,
    //     bit<IDX_WIDTH>,
    //     bit<32>
    // >(lock_data_reg) release_exclu = {
    //     void apply(inout bit<32> register_data, out bit<32> result)
    //     {
    //         result = register_data;
    //         register_data = 0;
    //     }
    // };

    // RegisterAction<
    //     bit<32>,
    //     bit<IDX_WIDTH>,
    //     bit<32>
    // >(lock_data_reg) release_share = {
    //     void apply(inout bit<32> register_data, out bit<32> result)
    //     {
    //         if(register_data > 0)
    //             register_data = register_data - 1;
    //         result = register_data;
    //     }
    // };

    // type + data
    Register<lock_register_value_t, bit<IDX_WIDTH>>(SLOT_SIZE) lock_reg;

    RegisterAction<
        lock_register_value_t,
        bit<IDX_WIDTH>,
        bit<32>
    >(lock_reg) grant_exclu = {
        void apply(inout lock_register_value_t register_data, out bit<32> result)
        {
            if(register_data.data == client_id && register_data.type == EXCLU_LOCK)
            {
                result = 1;
            }
            
            if(register_data.data == 0 && client_id != 0)
            {
                register_data.data = client_id;
                register_data.type = EXCLU_LOCK;
                result = 1;
            }
            else
            {
                result = 0;
            }
        }
    };

    RegisterAction<
        lock_register_value_t,
        bit<IDX_WIDTH>,
        bit<32>
    >(lock_reg) grant_share = {
        void apply(inout lock_register_value_t register_data, out bit<32> result)
        {
            if(register_data.type != EXCLU_LOCK)
            {
                register_data.data = register_data.data + 1;
                register_data.type = SHARE_LOCK;
                result = 1;
            }
            else
            {
                result = 0;
            }
        }
    };

    RegisterAction<
        lock_register_value_t,
        bit<IDX_WIDTH>,
        bit<32>
    >(lock_reg) release_exclu = {
        void apply(inout lock_register_value_t register_data, out bit<32> result)
        {
            if(register_data.type == EXCLU_LOCK && register_data.data == client_id)
            {
                register_data.data = 0;
                register_data.type = 0;
                result = 1;
            }
            else
            {
                result = 0;
            }
        }
    };

    RegisterAction<
        lock_register_value_t,
        bit<IDX_WIDTH>,
        bit<32>
    >(lock_reg) release_share = {
        void apply(inout lock_register_value_t register_data, out bit<32> result)
        {
            if(register_data.type == SHARE_LOCK && register_data.data > 0)
            {
                register_data.data = register_data.data - 1;
                result = 1;
            }
            else
            {
                result = 0;
            }
        }
    };

    action grant_exclusive_lock()
    {
        op_result = grant_exclu.execute(index);
    }

    action grant_shared_lock()
    {
        op_result = grant_share.execute(index);
    }

    action release_exclusive_lock()
    {
        op_result = release_exclu.execute(index);
    }

    action release_shared_lock()
    {
        op_result = release_share.execute(index);
    }

    table granted_lock_table
    {
        key = {
            lock_type : exact;
            op_code   : exact;
        }

        actions = {
            grant_exclusive_lock;
            grant_shared_lock;
            release_exclusive_lock;
            release_shared_lock;
            @defaultonly NoAction;
        }

        size = 32;
        const entries = {
            (EXCLU_LOCK, GRANT_LOCK) : grant_exclusive_lock();
            (EXCLU_LOCK, RELEA_LOCK) : release_exclusive_lock();
            (SHARE_LOCK, GRANT_LOCK) : grant_shared_lock();
            (SHARE_LOCK, RELEA_LOCK) : release_shared_lock();
        }

        default_action = NoAction;
    }

    apply
    {
        granted_lock_table.apply();
    }
}


#endif //_GRANTED_LOCK_P4_
