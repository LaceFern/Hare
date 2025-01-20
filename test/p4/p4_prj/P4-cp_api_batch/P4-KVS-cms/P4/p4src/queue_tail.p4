#ifndef _QUEUE_TAIL_P4_
#define _QUEUE_TAIL_P4_


control QueueTail(
    in    oper_t op_code,
    in    bit<8> lock_type,
    in    bit<IDX_WIDTH> index,
    in    bit<CNT_WIDTH> queue_counter,
    inout bit<QUEUE_ITER_WIDTH> queue_tail
)
{
    Register<bit<QUEUE_ITER_WIDTH>, bit<IDX_WIDTH>>(SLOT_SIZE) queue_tail_reg;

    RegisterAction<
        bit<QUEUE_ITER_WIDTH>,
        bit<IDX_WIDTH>,
        bit<QUEUE_ITER_WIDTH>
    >(queue_tail_reg) shift = {
        void apply(inout bit<QUEUE_ITER_WIDTH> register_data, out bit<QUEUE_ITER_WIDTH> result)
        {
            result = register_data;
            register_data = register_data + 1;
            // if(register_data == QUEUE_SIZE - 1)
            // {
            //     register_data = 0;
            // }
            // else
            // {
            //     register_data = register_data + 1;
            // }
        }
    };

    RegisterAction<
        bit<QUEUE_ITER_WIDTH>,
        bit<IDX_WIDTH>,
        bit<QUEUE_ITER_WIDTH>
    >(queue_tail_reg) get = {
        void apply(inout bit<QUEUE_ITER_WIDTH> register_data, out bit<QUEUE_ITER_WIDTH> result)
        {
            result = register_data;
        }
    };

    action push_tail()
    {
        queue_tail = shift.execute(index);
    }

    action get_tail()
    {
        queue_tail = get.execute(index);
    }

    action invalid()
    {
        queue_tail = 0;
    }

    table queue_tail_table
    {
        key = {
            op_code       : exact;
            queue_counter : range;
        }

        actions = {
            push_tail;
            get_tail;
            invalid;
            NoAction;
        }

        size = 16;
        const entries = {
            (GRANT_LOCK, 1 .. QUEUE_SIZE) : push_tail();
            (RELEA_LOCK, 1 .. QUEUE_SIZE) : get_tail();
            (GRANT_LOCK,               _) : invalid();
            (RELEA_LOCK,               _) : invalid();
            (CHECK_HEAD,               _) : get_tail();
        }

        default_action = NoAction;
    }

    apply
    {
        queue_tail_table.apply();
    }
}


#endif //_QUEUE_TAIL_P4_
