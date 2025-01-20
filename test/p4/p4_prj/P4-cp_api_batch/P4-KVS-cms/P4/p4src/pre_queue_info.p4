#ifndef _PRE_QUEUE_INFO_P4_
#define _PRE_QUEUE_INFO_P4_


#include "queue_head.p4"
#include "queue_tail.p4"
#include "queue_counter.p4"

control PreQueueInfo(
    in    oper_t op_code,
    in    bit<8> lock_type,
    in    bit<IDX_WIDTH> index,
    inout bit<CNT_WIDTH> queue_counter,
    inout bit<QUEUE_ITER_WIDTH> queue_head,
    inout bit<QUEUE_ITER_WIDTH> queue_tail
)
{
    // QueueCounter() counter;
    QueueHead() head;
    QueueTail() tail;

    apply
    {
        // counter.apply(op_code, lock_type, index, queue_counter);
        head.apply(op_code, lock_type, index, queue_counter, queue_head);
        tail.apply(op_code, lock_type, index, queue_counter, queue_tail);
    }
}

#endif //_PRE_QUEUE_INFO_P4_
