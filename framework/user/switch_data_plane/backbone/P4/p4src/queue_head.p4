#ifndef _QUEUE_HEAD_P4_
#define _QUEUE_HEAD_P4_


control QueueHead(
    in    oper_t op_code,
    in    bit<8> lock_type,
    in    bit<IDX_WIDTH> index,
    in    bit<CNT_WIDTH> queue_counter,
    inout bit<QUEUE_ITER_WIDTH> queue_head 
)
{
    Register<bit<QUEUE_ITER_WIDTH>, bit<IDX_WIDTH>>(SLOT_SIZE) queue_head_reg;

    RegisterAction<
        bit<QUEUE_ITER_WIDTH>,
        bit<IDX_WIDTH>,
        bit<QUEUE_ITER_WIDTH>
    >(queue_head_reg) shift = {
        void apply(inout bit<QUEUE_ITER_WIDTH> register_data, out bit<QUEUE_ITER_WIDTH> result)
        {
            register_data = register_data + 1;
            result = register_data;
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
    >(queue_head_reg) get = {
        void apply(inout bit<QUEUE_ITER_WIDTH> register_data, out bit<QUEUE_ITER_WIDTH> result)
        {
            result = register_data;
        }
    };

    action pop_head()
    {
        queue_head = shift.execute(index);
    }

    action get_head()
    {
        queue_head = get.execute(index);
    }

    action invalid()
    {
        queue_head = 0;
    }

    action check_head()
    {
        queue_head = queue_head + 1;
    }

    table queue_head_table
    {
        key = {
            op_code       : exact;
            queue_counter : range;
        }

        actions = {
            pop_head;
            get_head;
            invalid;
            check_head;
            NoAction;
        }

        size = 16;
        const entries = {
            (GRANT_LOCK, 1 .. QUEUE_SIZE) : get_head();
            (RELEA_LOCK, 1 .. QUEUE_SIZE) : pop_head();
            (GRANT_LOCK,               _) : invalid();
            (RELEA_LOCK,               _) : invalid();
            (CHECK_HEAD,               _) : check_head();
        }

        default_action = NoAction;
    }

    apply
    {
        queue_head_table.apply();
    }
}


#endif //_QUEUE_HEAD_P4_
