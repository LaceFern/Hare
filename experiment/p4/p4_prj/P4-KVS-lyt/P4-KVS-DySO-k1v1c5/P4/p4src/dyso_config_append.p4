#ifndef _DYSO_CONFIG_APPEND_P4_
#define _DYSO_CONFIG_APPEND_P4_

//******************************** K E Y ********************************
// for one key
#define KEY_KEYWIDTH KEY_WIDTH // the bit width of one key
#define KEY_REGNUMINSTAGE 4 // the number of regs that one key crosses in one stage(max:4)
#define KEY_STAGENUM 1  // the number of stages that one key crosses (KEY_REGNUMINSTAGE * KEY_STAGENUM) must be an interger
#define KEY_REGWIDTH (KEY_KEYWIDTH / KEY_REGNUMINSTAGE / KEY_STAGENUM) // the bit width of one reg that stores the key (segment)
#define KEY_REGNUM (KEY_REGNUMINSTAGE * KEY_STAGENUM) // the number of regs that one key crosses in total

// for all keys
#define KEY_KEYNUM SLOT_SIZE // the size of the r-mat (row * col)
#define KEY_COLNUM 5 // 2 (col = 2) // the column of the r-mat
#define KEY_REGLEN (KEY_KEYNUM / KEY_COLNUM) // the row of the r-mat
#define KEY_STAGENUM4COL (KEY_STAGENUM * KEY_COLNUM) // the number of stages that an r-mat requires
// the number of KEY_REGs is KEY_REGNUMINSTAGE * KEY_STAGENUM4COL = 4 * (1 * 4) = 16
#define KEY_REGLENWIDTH 32
#define KEY_REGLENLENWIDTH 14 // 12b(4096)-2b

#define KEY_CTRLPKTBITWIDTH (KEY_KEYWIDTH * KEY_COLNUM) //

#define INF 0xffffffff

//--------------------------------key rmat reg----------------------------------
// regIndex in [0, KEY_REGNUM - 1]
// segIndex in [0, KEY_REGNUMINSTAGE * KEY_STAGENUM - 1] 
// ctrlIndex in [0, KEY_COLNUM - 1]
#define KEY_REG(regIndex, segIndex, ctrlIndex) \
    Register<bit<KEY_REGWIDTH>, _>(KEY_REGLEN) key_##regIndex; \
    RegisterAction<bit<KEY_REGWIDTH>, _, bit<1>>(key_##regIndex) key_##regIndex##_read = {\
        void apply(inout bit<KEY_REGWIDTH> val, out bit<1> rv){\
            if(hdr.kv.key_##segIndex == val) {\
                rv = 1;\
            } else {\
                rv = 0;\
            }\
        }\
    };\
    RegisterAction<bit<KEY_REGWIDTH>, _, bit<KEY_REGWIDTH>>(key_##regIndex) key_##regIndex##_update = {\
        void apply(inout bit<KEY_REGWIDTH> val){\
            val = hdr.ctrl_keyInfo.key_##segIndex##_##ctrlIndex;\
        }\
    };

// the number of lines in KEY_RMAT_REG equals (KEY_REGNUM * KEY_COLNUM)
#define KEY_RMAT_REG()\
    KEY_REG(0, 0, 0)\
    KEY_REG(1, 1, 0)\
    KEY_REG(2, 2, 0)\
    KEY_REG(3, 3, 0)\
    KEY_REG(4, 0, 1)\
    KEY_REG(5, 1, 1)\
    KEY_REG(6, 2, 1)\
    KEY_REG(7, 3, 1)\
    KEY_REG(8, 0, 2)\
    KEY_REG(9, 1, 2)\
    KEY_REG(10, 2, 2)\
    KEY_REG(11, 3, 2)\
    KEY_REG(12, 0, 3)\
    KEY_REG(13, 1, 3)\
    KEY_REG(14, 2, 3)\
    KEY_REG(15, 3, 3)\
    KEY_REG(16, 0, 4)\
    KEY_REG(17, 1, 4)\
    KEY_REG(18, 2, 4)\
    KEY_REG(19, 3, 4)  


//--------------------------------key rmat hitcheck----------------------------------
#define KEY_CHECKREGCOLHIT_ACTIONandMAT(regIndex)\
    bit<1> tmp_hit_##regIndex = 0;\
    action key_read_action_##regIndex(){\
        tmp_hit_##regIndex = key_##regIndex##_read.execute(meta.kv.hash_index);\
    }\
    action key_update_action_##regIndex(){\
        key_##regIndex##_update.execute(meta.kv.hash_index);\
    }\
    table key_checkRegColHit_mat_##regIndex{\
        key = {\
            hdr.ethernet.ether_type : exact;\
            hdr.kv.op_code : ternary;\
        }\
        actions = {\
            key_read_action_##regIndex;\
            key_update_action_##regIndex;\
            @defaultonly NoAction;\
        }\
        default_action = NoAction;\
        size           = 32;\
        const entries = {\
            (ETHERTYPE_IPV4, OP_GET) : key_read_action_##regIndex(); \
            (0x2333, _) : key_update_action_##regIndex(); \
        }\
    }

#define KEY_GETCOLHITINDEX_ACTIONandMAT()\
    action key_colHit_action_0(){\
        hdr.bridge.egress_bridge_md.colhit_index = 0;\
    }\
    action key_colHit_action_1(){\
        hdr.bridge.egress_bridge_md.colhit_index = 1;\
    }\
    action key_colHit_action_2(){\
        hdr.bridge.egress_bridge_md.colhit_index = 2;\
    }\
    action key_colHit_action_3(){\
        hdr.bridge.egress_bridge_md.colhit_index = 3;\
    }\
    action key_colHit_action_4(){\
        hdr.bridge.egress_bridge_md.colhit_index = 4;\
    }\
    table key_getColHitIndex_mat{\
        key = {\
            hdr.ethernet.ether_type : exact;\
            hdr.kv.op_code : exact;\
            tmp_hit_0 : ternary;\
            tmp_hit_1 : ternary;\
            tmp_hit_2 : ternary;\
            tmp_hit_3 : ternary;\
            tmp_hit_4 : ternary;\
            tmp_hit_5 : ternary;\
            tmp_hit_6 : ternary;\
            tmp_hit_7 : ternary;\
            tmp_hit_8 : ternary;\
            tmp_hit_9 : ternary;\
            tmp_hit_10 : ternary;\
            tmp_hit_11 : ternary;\
            tmp_hit_12 : ternary;\
            tmp_hit_13 : ternary;\
            tmp_hit_14 : ternary;\
            tmp_hit_15 : ternary;\
            tmp_hit_16 : ternary;\
            tmp_hit_17 : ternary;\
            tmp_hit_18 : ternary;\
            tmp_hit_19 : ternary;\           
        }\
        actions = {\
            key_colHit_action_0;\
            key_colHit_action_1;\
            key_colHit_action_2;\
            key_colHit_action_3;\
            key_colHit_action_4;\
            @defaultonly NoAction;\
        }\
        default_action = NoAction;\
        size           = 32;\
        const entries = {\
            (ETHERTYPE_IPV4, OP_GET, 1, 1, 1, 1, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _) : key_colHit_action_0(); \
            (ETHERTYPE_IPV4, OP_GET, _, _, _, _, 1, 1, 1, 1, _, _, _, _, _, _, _, _, _, _, _, _) : key_colHit_action_1(); \
            (ETHERTYPE_IPV4, OP_GET, _, _, _, _, _, _, _, _, 1, 1, 1, 1, _, _, _, _, _, _, _, _) : key_colHit_action_2(); \
            (ETHERTYPE_IPV4, OP_GET, _, _, _, _, _, _, _, _, _, _, _, _, 1, 1, 1, 1, _, _, _, _) : key_colHit_action_3(); \
            (ETHERTYPE_IPV4, OP_GET, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, _, 1, 1, 1, 1) : key_colHit_action_4(); \
        }\
    }

#define KEY_COLS_ACTIONandMAT()\
    KEY_CHECKREGCOLHIT_ACTIONandMAT(0)\
    KEY_CHECKREGCOLHIT_ACTIONandMAT(1)\
    KEY_CHECKREGCOLHIT_ACTIONandMAT(2)\
    KEY_CHECKREGCOLHIT_ACTIONandMAT(3)\
    KEY_CHECKREGCOLHIT_ACTIONandMAT(4)\
    KEY_CHECKREGCOLHIT_ACTIONandMAT(5)\
    KEY_CHECKREGCOLHIT_ACTIONandMAT(6)\
    KEY_CHECKREGCOLHIT_ACTIONandMAT(7)\
    KEY_CHECKREGCOLHIT_ACTIONandMAT(8)\
    KEY_CHECKREGCOLHIT_ACTIONandMAT(9)\
    KEY_CHECKREGCOLHIT_ACTIONandMAT(10)\
    KEY_CHECKREGCOLHIT_ACTIONandMAT(11)\
    KEY_CHECKREGCOLHIT_ACTIONandMAT(12)\
    KEY_CHECKREGCOLHIT_ACTIONandMAT(13)\
    KEY_CHECKREGCOLHIT_ACTIONandMAT(14)\
    KEY_CHECKREGCOLHIT_ACTIONandMAT(15)\
    KEY_CHECKREGCOLHIT_ACTIONandMAT(16)\
    KEY_CHECKREGCOLHIT_ACTIONandMAT(17)\
    KEY_CHECKREGCOLHIT_ACTIONandMAT(18)\
    KEY_CHECKREGCOLHIT_ACTIONandMAT(19)\
    KEY_GETCOLHITINDEX_ACTIONandMAT()


#define KEY_COLS_APPLY()\
    key_checkRegColHit_mat_0.apply();\
    key_checkRegColHit_mat_1.apply();\
    key_checkRegColHit_mat_2.apply();\
    key_checkRegColHit_mat_3.apply();\
    key_checkRegColHit_mat_4.apply();\
    key_checkRegColHit_mat_5.apply();\
    key_checkRegColHit_mat_6.apply();\
    key_checkRegColHit_mat_7.apply();\
    key_checkRegColHit_mat_8.apply();\
    key_checkRegColHit_mat_9.apply();\
    key_checkRegColHit_mat_10.apply();\
    key_checkRegColHit_mat_11.apply();\
    key_checkRegColHit_mat_12.apply();\
    key_checkRegColHit_mat_13.apply();\
    key_checkRegColHit_mat_14.apply();\
    key_checkRegColHit_mat_15.apply();\
    key_checkRegColHit_mat_16.apply();\
    key_checkRegColHit_mat_17.apply();\
    key_checkRegColHit_mat_18.apply();\
    key_checkRegColHit_mat_19.apply();\
    key_getColHitIndex_mat.apply();

//--------------------------------hash----------------------------------
// the key (hash input) is the whole key, key segs should be intergrated
#define HASH_KEY2INDEX_ACTION()\
    Hash<bit<32>>(HashAlgorithm_t.CRC32) cal_hash;\
    bit<32> hash_value = 0;\
    action hash_key2index(){\
        hash_value = cal_hash.get(hdr.kv.key_0 ++ hdr.kv.key_1 ++ hdr.kv.key_2 ++ hdr.kv.key_3);\
        hdr.bridge.egress_bridge_md.hash_index = (bit<KEY_REGLENWIDTH>)(hash_value[KEY_REGLENLENWIDTH - 1 : 0]);\
    }

#define HASH_KEY2INDEX_APPLY()\
    hash_key2index();\
    if(hdr.kv.isValid()){\
        meta.kv.hash_index = hdr.bridge.egress_bridge_md.hash_index;\
        hdr.bridge.egress_bridge_md.colhit_index = INF;\
    }\
    else{\
        meta.kv.hash_index = hdr.ctrl_opHashInfo.hash_index;\
    }

//******************************** V A L U E ********************************
// for one value
#define VALUE_VALUEWIDTH VALUE_WIDTH // the bit width of one value
#define VALUE_REGNUMINSTAGE 4 // the number of regs that one value crosses in one stage(max:4)
#define VALUE_STAGENUM 1  // the number of stages that one value crosses (VALUE_REGNUMINSTAGE * VALUE_STAGENUM) must be an interger
#define VALUE_REGWIDTH (VALUE_VALUEWIDTH / VALUE_REGNUMINSTAGE / VALUE_STAGENUM) // the bit width of one reg that stores the value (segment)
#define VALUE_REGNUM (VALUE_REGNUMINSTAGE * VALUE_STAGENUM) // the number of regs that one value crosses in total

// for all values
#define VALUE_VALUENUM SLOT_SIZE // the size of the r-mat (row * col)
#define VALUE_COLNUM 5 // the column of the r-mat (must equal to VALUE_COLNUM)
#define VALUE_REGLEN (VALUE_VALUENUM / VALUE_COLNUM) // the row of the r-mat ???????????????????????????????
#define VALUE_STAGENUM4COL (VALUE_STAGENUM * VALUE_COLNUM) // the number of stages that an r-mat requires
// the number of VALUE_REGs is VALUE_REGNUMINSTAGE * VALUE_STAGENUM4COL = 4 * (1 * 4) = 16
#define VALUE_REGLENWIDTH 32

#define VALUE_REG(regIndex, segIndex, ctrlIndex)\
Register<bit<VALUE_REGWIDTH>, _>(VALUE_REGLEN) value_##regIndex;\
RegisterAction<bit<VALUE_REGWIDTH>, _, bit<VALUE_REGWIDTH>>(value_##regIndex) value_##regIndex##_read = {\
    void apply(inout bit<VALUE_REGWIDTH> val, out bit<VALUE_REGWIDTH> result){\
        result = val;\
    }\
};\
RegisterAction<bit<VALUE_REGWIDTH>, _, bit<VALUE_REGWIDTH>>(value_##regIndex) value_##regIndex##_update = {\
    void apply(inout bit<VALUE_REGWIDTH> val){\
        val = hdr.ctrl_valueInfo.value_##segIndex##_##ctrlIndex;\
    }\
};

//--------------------------------data modify----------------------------------
// the number of lines in VALUE_RMAT_NEW equals (VALUE_REGNUM * VALUE_COLNUM)
#define VALUE_RMAT_NEW()\
    VALUE_REG(0, 0, 0)\
    VALUE_REG(1, 1, 0)\
    VALUE_REG(2, 2, 0)\
    VALUE_REG(3, 3, 0)\
    VALUE_REG(4, 0, 1)\
    VALUE_REG(5, 1, 1)\
    VALUE_REG(6, 2, 1)\
    VALUE_REG(7, 3, 1)\
    VALUE_REG(8, 0, 2)\
    VALUE_REG(9, 1, 2)\
    VALUE_REG(10, 2, 2)\
    VALUE_REG(11, 3, 2)\
    VALUE_REG(12, 0, 3)\
    VALUE_REG(13, 1, 3)\
    VALUE_REG(14, 2, 3)\
    VALUE_REG(15, 3, 3)\
    VALUE_REG(16, 0, 4)\
    VALUE_REG(17, 1, 4)\
    VALUE_REG(18, 2, 4)\
    VALUE_REG(19, 3, 4)  

// the number of value_##...##_read equals (VALUE_REGNUM)
#define VALUE_READ_COL_RUN(colIndex, regIndex0, regIndex1, regIndex2, regIndex3)\
    if(colIndex == meta.kv.colhit_index){\
        hdr.kv_data.value_0 = value_##regIndex0##_read.execute(meta.kv.hash_index);\
        hdr.kv_data.value_1 = value_##regIndex1##_read.execute(meta.kv.hash_index);\
        hdr.kv_data.value_2 = value_##regIndex2##_read.execute(meta.kv.hash_index);\
        hdr.kv_data.value_3 = value_##regIndex3##_read.execute(meta.kv.hash_index);\
    }

// the number of VALUE_READ_COL_RUN equals (VALUE_COLNUM)
#define VALUE_READ_COLS_RUN()\
    VALUE_READ_COL_RUN(0, 0, 1, 2, 3)\
    VALUE_READ_COL_RUN(1, 4, 5, 6, 7)\
    VALUE_READ_COL_RUN(2, 8, 9, 10, 11)\
    VALUE_READ_COL_RUN(3, 12, 13, 14, 15)\
    VALUE_READ_COL_RUN(4, 16, 17, 18, 19)

// the number of value_##...##_update equals (VALUE_REGNUM)
#define VALUE_MODIFY_COL_RUN(colIndex, regIndex0, regIndex1, regIndex2, regIndex3)\
    value_##regIndex0##_update.execute(hdr.ctrl_opHashInfo.hash_index);\
    value_##regIndex1##_update.execute(hdr.ctrl_opHashInfo.hash_index);\
    value_##regIndex2##_update.execute(hdr.ctrl_opHashInfo.hash_index);\
    value_##regIndex3##_update.execute(hdr.ctrl_opHashInfo.hash_index);

#define VALUE_MODIFY_COLS_RUN()\
    VALUE_MODIFY_COL_RUN(0, 0, 1, 2, 3)\
    VALUE_MODIFY_COL_RUN(1, 4, 5, 6, 7)\
    VALUE_MODIFY_COL_RUN(2, 8, 9, 10, 11)\
    VALUE_MODIFY_COL_RUN(3, 12, 13, 14, 15)\
    VALUE_MODIFY_COL_RUN(4, 16, 17, 18, 19)


// #define CPU_CONTROLLER 24

#endif