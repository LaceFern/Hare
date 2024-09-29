#ifndef _LOCK_QUEUE_P4_
#define _LOCK_QUEUE_P4_


#include "lock_queue_node.p4"

control LockQueue(
    in    oper_t  op_code,
    in    bit<8>  lock_type,
    in    bit<32> client_id,
    in    bit<IDX_WIDTH> index,
    in    bit<CNT_WIDTH> queue_counter,
    in    bit<QUEUE_ITER_WIDTH> queue_head,
    in    bit<QUEUE_ITER_WIDTH> queue_tail,
    out   bit<1>  op_result,
    out   oper_t  op_output,
    // mirror
    inout bit<1>  do_mirror,
    inout oper_t  mirror_op,
    inout bit<8>  mirror_tp,
    inout bit<32> mirror_id,
    inout bit<8>  release_type
)
{
    // queue node
    NODE(0)
    NODE(1)
    NODE(2)
    NODE(3)
    NODE(4)
    NODE(5)
    NODE(6)
    NODE(7)

    // NODE(8)
    // NODE(9)
    // NODE(10)
    // NODE(11)
    // NODE(12)
    // NODE(13)
    // NODE(14)
    // NODE(15)

    // NODE(16)
    // NODE(17)
    // NODE(18)
    // NODE(19)
    // NODE(20)
    // NODE(21)
    // NODE(22)
    // NODE(23)
    // NODE(24)
    // NODE(25)
    // NODE(26)
    // NODE(27)
    // NODE(28)
    // NODE(29)
    // NODE(30)
    // NODE(31)

    table query_valid_table // is this operation valid? which data should be used to reply?
    {
        key = {
            op_code       : exact;
            queue_counter : range;
        }

        actions = {
            NoAction;
        }

        size = 64;
        const entries = {
            (GRANT_LOCK,               1) : NoAction; // client_id grant
            (GRANT_LOCK, 2 .. QUEUE_SIZE) : NoAction; // client_id queue
            (RELEA_LOCK,               1) : NoAction; // no reply
            (RELEA_LOCK, 2 .. QUEUE_SIZE) : NoAction; // head + 1
            (CHECK_HEAD, 1 .. QUEUE_SIZE) : NoAction; // check
        }

        default_action = NoAction;
    }

    // next step
    Register<bit<CNT_WIDTH>, bit<IDX_WIDTH>>(SLOT_SIZE) queue_state_reg;

    RegisterAction<
        bit<CNT_WIDTH>,
        bit<IDX_WIDTH>,
        bit<8>
    >(queue_state_reg) count = {
        void apply(inout bit<CNT_WIDTH> register_data)
        {
            register_data = register_data + 1;
        }
    };

    RegisterAction<
        bit<CNT_WIDTH>,
        bit<IDX_WIDTH>,
        bit<8>
    >(queue_state_reg) minus = {
        void apply(inout bit<CNT_WIDTH> register_data)
        {
            if(register_data > 0)
            {
                register_data = register_data - 1;
            }
        }
    };

    RegisterAction<
        bit<CNT_WIDTH>,
        bit<IDX_WIDTH>,
        bit<8>
    >(queue_state_reg) check = {
        void apply(inout bit<CNT_WIDTH> register_data, out bit<8> result)
        {
            if(register_data == 0)
            {
                result = LOCK_GRANT_SUCC;
            }
            else
            {
                register_data = register_data + 1;
                result = LOCK_QUEUE_SUCC;
            }
        }
    };

    bit<8> queue_opid = 0;

    action mirror_exclu_begin()
    {
        queue_opid = RE_LOCK_RELEA_SUCC;
        release_type = lock_type; // EXCLU_LOCK;

        minus.execute(index);
    }

    action mirror_share_begin()
    {
        queue_opid = RE_LOCK_RELEA_SUCC;
        release_type = lock_type; // SHARE_LOCK;
    }

    action mirror_exclu_check()
    {
        queue_opid = RE_LOCK_GRANT_SUCC;
    }

    action mirror_share_check()
    {
        queue_opid = RE_LOCK_GRANT_SUCC;

        minus.execute(index);
    }

    action grant_exclu_succ()
    {
        queue_opid = LOCK_GRANT_SUCC;

        count.execute(index);
    }
    
    action grant_share_succ()
    {
        queue_opid = LOCK_GRANT_SUCC;
    }

    action queue_exclu_succ()
    {
        queue_opid = LOCK_QUEUE_SUCC;

        count.execute(index);
    }

    action queue_share_succ()
    {
        queue_opid = check.execute(index);
    }
        
    action grant_lock_fail()
    {
        queue_opid = LOCK_GRANT_FAIL;
    }

    action release_exclu_succ()
    {
        queue_opid = 2w0b11 ++ (oper_t) LOCK_RELEA_SUCC; // 0xc5 // LOCK_RELEA_SUCC;
        // release_type = lock_type; // EXCLU_LOCK;

        minus.execute(index);
    }

    action release_share_succ()
    {
        queue_opid = 2w0b11 ++ (oper_t) LOCK_RELEA_SUCC; // 0xc5 // LOCK_RELEA_SUCC;
        // release_type = lock_type; // SHARE_LOCK;
    }

    action release_lock_fail()
    {
        queue_opid = LOCK_RELEA_FAIL;
    }

    table queue_operation_table
    {
        key = {
            op_code       : exact;
            lock_type     : ternary;
            queue_counter : range;
        }

        actions = {
            mirror_exclu_begin;
            mirror_share_begin;
            mirror_exclu_check;
            mirror_share_check;
            grant_exclu_succ;   // client_id grant succ
            grant_share_succ;   // client_id grant succ
            queue_exclu_succ;   // client_id queue succ
            queue_share_succ;   // client_id queue succ
            grant_lock_fail;    // client_id fail
            release_exclu_succ; // no reply or client_id release succ
            release_share_succ; // no reply or client_id release succ
            release_lock_fail;  // client_id release succ
            NoAction;
        }

        size = 64;
        const entries = {
            /* grant */
            (GRANT_LOCK, EXCLU_LOCK,               1) : grant_exclu_succ();   // set_nexthop(1); // client_id grant succ
            (GRANT_LOCK, EXCLU_LOCK, 2 .. QUEUE_SIZE) : queue_exclu_succ();   // waiting list ++ // set_nexthop(2); // client_id queue succ
            (GRANT_LOCK, SHARE_LOCK,               1) : grant_share_succ();   // set_nexthop(1); // client_id grant succ
            (GRANT_LOCK, SHARE_LOCK, 2 .. QUEUE_SIZE) : queue_share_succ();   // waiting list ++ // set_nexthop(2); // client_id queue succ // may be modified later
            (GRANT_LOCK,          _,               _) : grant_lock_fail();    // set_nexthop(5); // client_id grant fail
            /* release */
            (RELEA_LOCK, EXCLU_LOCK,               1) : release_exclu_succ(); // set_nexthop(3); // no reply or client_id release succ // notify controller
            (RELEA_LOCK, EXCLU_LOCK, 2 .. QUEUE_SIZE) : mirror_exclu_begin(); // head + 1 grant
            (RELEA_LOCK, SHARE_LOCK,               1) : release_share_succ(); // set_nexthop(3); // no reply or client_id release succ // notify controller
            (RELEA_LOCK, SHARE_LOCK, 2 .. QUEUE_SIZE) : mirror_share_begin(); // head + 1 grant
            (RELEA_LOCK,          _,               _) : release_lock_fail();  // set_nexthop(6); // client_id release fail
            /* mirroring */
            (CHECK_HEAD, EXCLU_LOCK, 1 .. QUEUE_SIZE) : mirror_exclu_check(); // waiting list -- // forward and check next
            (CHECK_HEAD, SHARE_LOCK, 1 .. QUEUE_SIZE) : mirror_share_check(); // waiting list -- // forward and check next
            (CHECK_HEAD,          _,               _) : NoAction;             // something bad happend
        }

        const default_action = NoAction; // default nexthop id 0
    }

    // currhop
    action grant_succ()
    {
        op_result = 1;
        op_output = LOCK_GRANT_SUCC;
    }

    action queue_succ()
    {
        op_result = 1;
        op_output = LOCK_QUEUE_SUCC;
    }

    action grant_fail()
    {
        op_result = 1;
        op_output = LOCK_GRANT_FAIL;
    }

    action release_succ()
    {
        op_result = 1;
        op_output = LOCK_RELEA_SUCC;
    }

    action release_fail()
    {
        op_result = 1;
        op_output = LOCK_RELEA_FAIL;
    }

    action invalid()
    {
        op_result = 0;
        // op_output = 0;
        op_output = op_code;
    }

    table forward_data_table
    {
        key = {
            queue_opid : ternary;
        }

        actions = {
            grant_succ;
            queue_succ;
            grant_fail;
            release_succ;
            release_fail;
            invalid;
        }

        size = 64;
        const entries = {
            (3 &&& OPER_MUSK) : grant_succ();
            (4 &&& OPER_MUSK) : grant_fail();
            (5 &&& OPER_MUSK) : release_succ();
            (6 &&& OPER_MUSK) : release_fail();
            (7 &&& OPER_MUSK) : queue_succ();
        }

        default_action = invalid();
    }

    // next hop
    action empty_callback()
    {
        do_mirror = 1;
        mirror_op = EMPTY_CALL;
        mirror_tp = 0;
        mirror_id = 0;
    }

    action invalid_mirror()
    {
        do_mirror = 0;
    }

    table nexthop_data_table
    {
        key = {
            queue_opid : ternary;
            queue_head : ternary;
        }

        actions = {
            empty_callback;
            /*  8 */
            grant_succ_0;  // head + 1 grant
            grant_succ_1;  // head + 1 grant
            grant_succ_2;  // head + 1 grant
            grant_succ_3;  // head + 1 grant
            grant_succ_4;  // head + 1 grant
            grant_succ_5;  // head + 1 grant
            grant_succ_6;  // head + 1 grant
            grant_succ_7;  // head + 1 grant
            /* 16 */
            // grant_succ_8;  // head + 1 grant
            // grant_succ_9;  // head + 1 grant
            // grant_succ_10; // head + 1 grant
            // grant_succ_11; // head + 1 grant
            // grant_succ_12; // head + 1 grant
            // grant_succ_13; // head + 1 grant
            // grant_succ_14; // head + 1 grant
            // grant_succ_15; // head + 1 grant
            /* 32 */
            // grant_succ_16; // head + 1 grant
            // grant_succ_17; // head + 1 grant
            // grant_succ_18; // head + 1 grant
            // grant_succ_19; // head + 1 grant
            // grant_succ_20; // head + 1 grant
            // grant_succ_21; // head + 1 grant
            // grant_succ_22; // head + 1 grant
            // grant_succ_23; // head + 1 grant
            // grant_succ_24; // head + 1 grant
            // grant_succ_25; // head + 1 grant
            // grant_succ_26; // head + 1 grant
            // grant_succ_27; // head + 1 grant
            // grant_succ_28; // head + 1 grant
            // grant_succ_29; // head + 1 grant
            // grant_succ_30; // head + 1 grant
            // grant_succ_31; // head + 1 grant
            invalid_mirror;
        }

        size = 64;
        const entries = {
            (0xc0 &&& FLAG_MUSK,                 _) : empty_callback(); // mirror to notify controller queue is empty
            /*  8 */
            (0x80 &&& FLAG_MUSK,  0 &&& QUEUE_MASK) : grant_succ_0();   // head + 1 grant
            (0x80 &&& FLAG_MUSK,  1 &&& QUEUE_MASK) : grant_succ_1();   // head + 1 grant
            (0x80 &&& FLAG_MUSK,  2 &&& QUEUE_MASK) : grant_succ_2();   // head + 1 grant
            (0x80 &&& FLAG_MUSK,  3 &&& QUEUE_MASK) : grant_succ_3();   // head + 1 grant
            (0x80 &&& FLAG_MUSK,  4 &&& QUEUE_MASK) : grant_succ_4();   // head + 1 grant
            (0x80 &&& FLAG_MUSK,  5 &&& QUEUE_MASK) : grant_succ_5();   // head + 1 grant
            (0x80 &&& FLAG_MUSK,  6 &&& QUEUE_MASK) : grant_succ_6();   // head + 1 grant
            (0x80 &&& FLAG_MUSK,  7 &&& QUEUE_MASK) : grant_succ_7();   // head + 1 grant
            /* 16 */
            // (0x80 &&& 0xf0,  8 &&& QUEUE_MASK) : grant_succ_9();   // head + 1 grant
            // (0x80 &&& 0xf0,  9 &&& QUEUE_MASK) : grant_succ_10();  // head + 1 grant
            // (0x80 &&& 0xf0, 10 &&& QUEUE_MASK) : grant_succ_11();  // head + 1 grant
            // (0x80 &&& 0xf0, 11 &&& QUEUE_MASK) : grant_succ_12();  // head + 1 grant
            // (0x80 &&& 0xf0, 12 &&& QUEUE_MASK) : grant_succ_13();  // head + 1 grant
            // (0x80 &&& 0xf0, 13 &&& QUEUE_MASK) : grant_succ_14();  // head + 1 grant
            // (0x80 &&& 0xf0, 14 &&& QUEUE_MASK) : grant_succ_15();  // head + 1 grant
            // (0x80 &&& 0xf0, 15 &&& QUEUE_MASK) : grant_succ_16();  // head + 1 grant
            /* 32 */
            // (0x80 &&& 0xf0, 16 &&& QUEUE_MASK) : grant_succ_17();  // head + 1 grant
            // (0x80 &&& 0xf0, 17 &&& QUEUE_MASK) : grant_succ_18();  // head + 1 grant
            // (0x80 &&& 0xf0, 18 &&& QUEUE_MASK) : grant_succ_19();  // head + 1 grant
            // (0x80 &&& 0xf0, 19 &&& QUEUE_MASK) : grant_succ_20();  // head + 1 grant
            // (0x80 &&& 0xf0, 20 &&& QUEUE_MASK) : grant_succ_21();  // head + 1 grant
            // (0x80 &&& 0xf0, 21 &&& QUEUE_MASK) : grant_succ_22();  // head + 1 grant
            // (0x80 &&& 0xf0, 22 &&& QUEUE_MASK) : grant_succ_23();  // head + 1 grant
            // (0x80 &&& 0xf0, 23 &&& QUEUE_MASK) : grant_succ_24();  // head + 1 grant
            // (0x80 &&& 0xf0, 24 &&& QUEUE_MASK) : grant_succ_25();  // head + 1 grant
            // (0x80 &&& 0xf0, 25 &&& QUEUE_MASK) : grant_succ_26();  // head + 1 grant
            // (0x80 &&& 0xf0, 26 &&& QUEUE_MASK) : grant_succ_27();  // head + 1 grant
            // (0x80 &&& 0xf0, 27 &&& QUEUE_MASK) : grant_succ_28();  // head + 1 grant
            // (0x80 &&& 0xf0, 28 &&& QUEUE_MASK) : grant_succ_29();  // head + 1 grant
            // (0x80 &&& 0xf0, 29 &&& QUEUE_MASK) : grant_succ_30();  // head + 1 grant
            // (0x80 &&& 0xf0, 30 &&& QUEUE_MASK) : grant_succ_31();  // head + 1 grant
         }

        default_action = invalid_mirror();
    }

    apply
    {
        queue_operation_table.apply();
        if(query_valid_table.apply().hit)
        {
            NODE_APPLY(0);
            NODE_APPLY(1);
            NODE_APPLY(2);
            NODE_APPLY(3);
            NODE_APPLY(4);
            NODE_APPLY(5);
            NODE_APPLY(6);
            NODE_APPLY(7);

            // NODE_APPLY(8);
            // NODE_APPLY(9);
            // NODE_APPLY(10);
            // NODE_APPLY(11);
            // NODE_APPLY(12);
            // NODE_APPLY(13);
            // NODE_APPLY(14);
            // NODE_APPLY(15);

            // NODE_APPLY(16);
            // NODE_APPLY(17);
            // NODE_APPLY(18);
            // NODE_APPLY(19);
            // NODE_APPLY(20);
            // NODE_APPLY(21);
            // NODE_APPLY(22);
            // NODE_APPLY(23);
            // NODE_APPLY(24);
            // NODE_APPLY(25);
            // NODE_APPLY(26);
            // NODE_APPLY(27);
            // NODE_APPLY(28);
            // NODE_APPLY(29);
            // NODE_APPLY(30);
            // NODE_APPLY(31);
        }
        forward_data_table.apply();
        nexthop_data_table.apply();
    }
}


#endif //_LOCK_QUEUE_P4_
