#ifndef _DYSO_CONFIG_APPEND_P4_
#define _DYSO_CONFIG_APPEND_P4_

//******************************** K E Y ********************************
// for one key
#define KEY_KEYWIDTH KEY_WIDTH // the bit width of one key
#define KEY_REGNUMINSTAGE 4 // the number of regs that one key crosses in one stage(max:4)
#define KEY_STAGENUM 5  // the number of stages that one key crosses (KEY_REGNUMINSTAGE * KEY_STAGENUM) must be an interger
#define KEY_REGWIDTH (KEY_KEYWIDTH / KEY_REGNUMINSTAGE / KEY_STAGENUM) // the bit width of one reg that stores the key (segment)
#define KEY_REGNUM (KEY_REGNUMINSTAGE * KEY_STAGENUM) // the number of regs that one key crosses in total

// for all keys
#define KEY_KEYNUM SLOT_SIZE // the size of the r-mat (row * col)
#define KEY_COLNUM 1 // 2 (col = 2) // the column of the r-mat
#define KEY_REGLEN (KEY_KEYNUM / KEY_COLNUM) // the row of the r-mat
#define KEY_STAGENUM4COL (KEY_STAGENUM * KEY_COLNUM) // the number of stages that an r-mat requires
// the number of KEY_REGs is KEY_REGNUMINSTAGE * KEY_STAGENUM4COL = 4 * (1 * 4) = 16
#define KEY_REGLENWIDTH 32
#define KEY_REGLENLENWIDTH 15 // 12b(4096)-2b

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
    KEY_REG(4, 4, 0)\
    KEY_REG(5, 5, 0)\
    KEY_REG(6, 6, 0)\
    KEY_REG(7, 7, 0)\
    KEY_REG(8, 8, 0)\
    KEY_REG(9, 9, 0)\
    KEY_REG(10, 10, 0)\
    KEY_REG(11, 11, 0)\
    KEY_REG(12, 12, 0)\
    KEY_REG(13, 13, 0)\
    KEY_REG(14, 14, 0)\
    KEY_REG(15, 15, 0)\
    KEY_REG(16, 16, 0)\
    KEY_REG(17, 17, 0)\
    KEY_REG(18, 18, 0)\
    KEY_REG(19, 19, 0)

    // KEY_REG(0, 0, 0)\
    // KEY_REG(1, 1, 0)\
    // KEY_REG(2, 2, 0)\
    // KEY_REG(3, 3, 0)\
    // KEY_REG(4, 0, 1)\
    // KEY_REG(5, 1, 1)\
    // KEY_REG(6, 2, 1)\
    // KEY_REG(7, 3, 1)\
    // KEY_REG(8, 0, 2)\
    // KEY_REG(9, 1, 2)\
    // KEY_REG(10, 2, 2)\
    // KEY_REG(11, 3, 2)\
    // KEY_REG(12, 0, 3)\
    // KEY_REG(13, 1, 3)\
    // KEY_REG(14, 2, 3)\
    // KEY_REG(15, 3, 3) 


//--------------------------------key rmat hitcheck----------------------------------
#define KEY_REGCOLE0_ACTIONandMAT(regIndex)\
    bit<1> tmp_hit_##regIndex = 0;\
    action key_read_action_##regIndex(){\
        tmp_hit_##regIndex = key_##regIndex##_read.execute(meta.kv.hash_index);\
    }\
    action key_update_action_##regIndex(){\
        key_##regIndex##_update.execute(meta.kv.hash_index);\
    }\
    table key_regAction_mat_##regIndex{\
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

#define KEY_REGCOLL0_ACTIONandMAT(regIndex, regIndexLast0, regIndexLast1, regIndexLast2, regIndexLast3)\
    bit<1> tmp_hit_##regIndex = 0;\
    action key_read_action_##regIndex(){\
        tmp_hit_##regIndex = key_##regIndex##_read.execute(meta.kv.hash_index);\
    }\
    action key_update_action_##regIndex(){\
        key_##regIndex##_update.execute(meta.kv.hash_index);\
    }\
    table key_regAction_mat_##regIndex{\
        key = {\
            hdr.ethernet.ether_type : exact;\
            hdr.kv.op_code : ternary;\
            tmp_hit_##regIndexLast0 : ternary;\
            tmp_hit_##regIndexLast1 : ternary;\
            tmp_hit_##regIndexLast2 : ternary;\
            tmp_hit_##regIndexLast3 : ternary;\
            hdr.bridge.egress_bridge_md.colhit_index : ternary;\
        }\
        actions = {\
            key_read_action_##regIndex;\
            key_update_action_##regIndex;\
            NoAction;\
        }\
        default_action = NoAction;\
        size           = 32;\
        const entries = {\
            (ETHERTYPE_IPV4, OP_GET, 1, 1, 1, 1, _) : NoAction;\
            (ETHERTYPE_IPV4, OP_GET, _, _, _, _, INF) : key_read_action_##regIndex(); \
            (0x2333, _, _, _, _, _, _) : key_update_action_##regIndex(); \
        }\
    }

#define KEY_LASTCOLHIT_ACTIONandMAT(colIndexLast, regIndexLast0, regIndexLast1, regIndexLast2, regIndexLast3, regIndexLast4, regIndexLast5, regIndexLast6, regIndexLast7, regIndexLast8, regIndexLast9, regIndexLast10, regIndexLast11, regIndexLast12, regIndexLast13, regIndexLast14, regIndexLast15, regIndexLast16, regIndexLast17, regIndexLast18, regIndexLast19)\
    action key_lastColHit_action_##colIndexLast(){\
        hdr.bridge.egress_bridge_md.colhit_index = colIndexLast;\
    }\
    table key_lastColHitAction_mat_##colIndexLast{\
        key = {\
            hdr.ethernet.ether_type : exact;\
            hdr.kv.op_code : exact;\
            tmp_hit_##regIndexLast0 : exact;\
            tmp_hit_##regIndexLast1 : exact;\
            tmp_hit_##regIndexLast2 : exact;\
            tmp_hit_##regIndexLast3 : exact;\
            tmp_hit_##regIndexLast4 : exact;\
            tmp_hit_##regIndexLast5 : exact;\
            tmp_hit_##regIndexLast6 : exact;\
            tmp_hit_##regIndexLast7 : exact;\
            tmp_hit_##regIndexLast8 : exact;\
            tmp_hit_##regIndexLast9 : exact;\
            tmp_hit_##regIndexLast10 : exact;\
            tmp_hit_##regIndexLast11 : exact;\
            tmp_hit_##regIndexLast12 : exact;\
            tmp_hit_##regIndexLast13 : exact;\
            tmp_hit_##regIndexLast14 : exact;\
            tmp_hit_##regIndexLast15 : exact;\
            tmp_hit_##regIndexLast16 : exact;\
            tmp_hit_##regIndexLast17 : exact;\
            tmp_hit_##regIndexLast18 : exact;\
            tmp_hit_##regIndexLast19 : exact;\
        }\
        actions = {\
            key_lastColHit_action_##colIndexLast;\
            @defaultonly NoAction;\
        }\
        default_action = NoAction;\
        size           = 32;\
        const entries = {\
            (ETHERTYPE_IPV4, OP_GET, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1) : key_lastColHit_action_##colIndexLast(); \
        }\
    }

// #define KEY_LASTCOLHIT_ACTIONandMAT(colIndexLast, regIndexLast0, regIndexLast1, regIndexLast2, regIndexLast3)\
//     action key_lastColHit_action_##colIndexLast(){\
//         hdr.bridge.egress_bridge_md.colhit_index = colIndexLast;\
//     }\
//     table key_lastColHitAction_mat_##colIndexLast{\
//         key = {\
//             hdr.ethernet.ether_type : exact;\
//             hdr.kv.op_code : exact;\
//             tmp_hit_##regIndexLast0 : exact;\
//             tmp_hit_##regIndexLast1 : exact;\
//             tmp_hit_##regIndexLast2 : exact;\
//             tmp_hit_##regIndexLast3 : exact;\
//         }\
//         actions = {\
//             key_lastColHit_action_##colIndexLast;\
//             @defaultonly NoAction;\
//         }\
//         default_action = NoAction;\
//         size           = 32;\
//         const entries = {\
//             (ETHERTYPE_IPV4, OP_GET, 1, 1, 1, 1) : key_lastColHit_action_##colIndexLast(); \
//         }\
//     }

#define KEY_COLS_ACTIONandMAT()\
    KEY_REGCOLE0_ACTIONandMAT(0)\
    KEY_REGCOLE0_ACTIONandMAT(1)\
    KEY_REGCOLE0_ACTIONandMAT(2)\
    KEY_REGCOLE0_ACTIONandMAT(3)\
    KEY_REGCOLE0_ACTIONandMAT(4)\
    KEY_REGCOLE0_ACTIONandMAT(5)\
    KEY_REGCOLE0_ACTIONandMAT(6)\
    KEY_REGCOLE0_ACTIONandMAT(7)\
    KEY_REGCOLE0_ACTIONandMAT(8)\
    KEY_REGCOLE0_ACTIONandMAT(9)\
    KEY_REGCOLE0_ACTIONandMAT(10)\
    KEY_REGCOLE0_ACTIONandMAT(11)\
    KEY_REGCOLE0_ACTIONandMAT(12)\
    KEY_REGCOLE0_ACTIONandMAT(13)\
    KEY_REGCOLE0_ACTIONandMAT(14)\
    KEY_REGCOLE0_ACTIONandMAT(15)\
    KEY_REGCOLE0_ACTIONandMAT(16)\
    KEY_REGCOLE0_ACTIONandMAT(17)\
    KEY_REGCOLE0_ACTIONandMAT(18)\
    KEY_REGCOLE0_ACTIONandMAT(19)\
    KEY_LASTCOLHIT_ACTIONandMAT(0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19)

    // KEY_REGCOLE0_ACTIONandMAT(0)\
    // KEY_REGCOLE0_ACTIONandMAT(1)\
    // KEY_REGCOLE0_ACTIONandMAT(2)\
    // KEY_REGCOLE0_ACTIONandMAT(3)\
    // KEY_REGCOLL0_ACTIONandMAT(4, 0, 1, 2, 3)\
    // KEY_REGCOLL0_ACTIONandMAT(5, 0, 1, 2, 3)\
    // KEY_REGCOLL0_ACTIONandMAT(6, 0, 1, 2, 3)\
    // KEY_REGCOLL0_ACTIONandMAT(7, 0, 1, 2, 3)\
    // KEY_REGCOLL0_ACTIONandMAT(8, 4, 5, 6, 7)\
    // KEY_REGCOLL0_ACTIONandMAT(9, 4, 5, 6, 7)\
    // KEY_REGCOLL0_ACTIONandMAT(10, 4, 5, 6, 7)\
    // KEY_REGCOLL0_ACTIONandMAT(11, 4, 5, 6, 7)\
    // KEY_REGCOLL0_ACTIONandMAT(12, 8, 9, 10, 11)\
    // KEY_REGCOLL0_ACTIONandMAT(13, 8, 9, 10, 11)\
    // KEY_REGCOLL0_ACTIONandMAT(14, 8, 9, 10, 11)\
    // KEY_REGCOLL0_ACTIONandMAT(15, 8, 9, 10, 11)\
    // KEY_LASTCOLHIT_ACTIONandMAT(3, 12, 13, 14, 15)


#define KEY_COLS_APPLY()\
    key_regAction_mat_0.apply();\
    key_regAction_mat_1.apply();\
    key_regAction_mat_2.apply();\
    key_regAction_mat_3.apply();\
    key_regAction_mat_4.apply();\
    key_regAction_mat_5.apply();\
    key_regAction_mat_6.apply();\
    key_regAction_mat_7.apply();\
    key_regAction_mat_8.apply();\
    key_regAction_mat_9.apply();\
    key_regAction_mat_10.apply();\
    key_regAction_mat_11.apply();\
    key_regAction_mat_12.apply();\
    key_regAction_mat_13.apply();\
    key_regAction_mat_14.apply();\
    key_regAction_mat_15.apply();\
    key_regAction_mat_16.apply();\
    key_regAction_mat_17.apply();\
    key_regAction_mat_18.apply();\
    key_regAction_mat_19.apply();\
    key_lastColHitAction_mat_0.apply();

    // key_regAction_mat_0.apply();\
    // key_regAction_mat_1.apply();\
    // key_regAction_mat_2.apply();\
    // key_regAction_mat_3.apply();\
    // key_regAction_mat_4.apply();\
    // key_regAction_mat_5.apply();\
    // key_regAction_mat_6.apply();\
    // key_regAction_mat_7.apply();\
    // key_regAction_mat_8.apply();\
    // key_regAction_mat_9.apply();\
    // key_regAction_mat_10.apply();\
    // key_regAction_mat_11.apply();\
    // key_regAction_mat_12.apply();\
    // key_regAction_mat_13.apply();\
    // key_regAction_mat_14.apply();\
    // key_regAction_mat_15.apply();\
    // key_lastColHitAction_mat_3.apply();

//--------------------------------hash----------------------------------
// the key (hash input) is the whole key, key segs should be intergrated
#define HASH_KEY2INDEX_ACTION()\
    Hash<bit<64>>(HashAlgorithm_t.CRC64) cal_hash;\
    bit<64> hash_value = 0;\
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
#define VALUE_STAGENUM 5  // the number of stages that one value crosses (VALUE_REGNUMINSTAGE * VALUE_STAGENUM) must be an interger
#define VALUE_REGWIDTH (VALUE_VALUEWIDTH / VALUE_REGNUMINSTAGE / VALUE_STAGENUM) // the bit width of one reg that stores the value (segment)
#define VALUE_REGNUM (VALUE_REGNUMINSTAGE * VALUE_STAGENUM) // the number of regs that one value crosses in total

// for all values
#define VALUE_VALUENUM SLOT_SIZE // the size of the r-mat (row * col)
#define VALUE_COLNUM 1 // the column of the r-mat (must equal to VALUE_COLNUM)
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
    VALUE_REG(4, 4, 0)\
    VALUE_REG(5, 5, 0)\
    VALUE_REG(6, 6, 0)\
    VALUE_REG(7, 7, 0)\
    VALUE_REG(8, 8, 0)\
    VALUE_REG(9, 9, 0)\
    VALUE_REG(10, 10, 0)\
    VALUE_REG(11, 11, 0)\
    VALUE_REG(12, 12, 0)\
    VALUE_REG(13, 13, 0)\
    VALUE_REG(14, 14, 0)\
    VALUE_REG(15, 15, 0)\
    VALUE_REG(16, 16, 0)\
    VALUE_REG(17, 17, 0)\
    VALUE_REG(18, 18, 0)\
    VALUE_REG(19, 19, 0)

    // VALUE_REG(0, 0, 0)\
    // VALUE_REG(1, 1, 0)\
    // VALUE_REG(2, 2, 0)\
    // VALUE_REG(3, 3, 0)\
    // VALUE_REG(4, 0, 1)\
    // VALUE_REG(5, 1, 1)\
    // VALUE_REG(6, 2, 1)\
    // VALUE_REG(7, 3, 1)\
    // VALUE_REG(8, 0, 2)\
    // VALUE_REG(9, 1, 2)\
    // VALUE_REG(10, 2, 2)\
    // VALUE_REG(11, 3, 2)\
    // VALUE_REG(12, 0, 3)\
    // VALUE_REG(13, 1, 3)\
    // VALUE_REG(14, 2, 3)\
    // VALUE_REG(15, 3, 3) 

// the number of value_##...##_read equals (VALUE_REGNUM)
#define VALUE_READ_COL_RUN(colIndex, regIndex0, regIndex1, regIndex2, regIndex3, regIndex4, regIndex5, regIndex6, regIndex7, regIndex8, regIndex9, regIndex10, regIndex11, regIndex12, regIndex13, regIndex14, regIndex15, regIndex16, regIndex17, regIndex18, regIndex19)\
    if(colIndex == meta.kv.colhit_index){\
        hdr.kv_data.value_0 = value_##regIndex0##_read.execute(meta.kv.hash_index);\
        hdr.kv_data.value_1 = value_##regIndex1##_read.execute(meta.kv.hash_index);\
        hdr.kv_data.value_2 = value_##regIndex2##_read.execute(meta.kv.hash_index);\
        hdr.kv_data.value_3 = value_##regIndex3##_read.execute(meta.kv.hash_index);\
        hdr.kv_data.value_4 = value_##regIndex4##_read.execute(meta.kv.hash_index);\
        hdr.kv_data.value_5 = value_##regIndex5##_read.execute(meta.kv.hash_index);\
        hdr.kv_data.value_6 = value_##regIndex6##_read.execute(meta.kv.hash_index);\
        hdr.kv_data.value_7 = value_##regIndex7##_read.execute(meta.kv.hash_index);\
        hdr.kv_data.value_8 = value_##regIndex8##_read.execute(meta.kv.hash_index);\
        hdr.kv_data.value_9 = value_##regIndex9##_read.execute(meta.kv.hash_index);\
        hdr.kv_data.value_10 = value_##regIndex10##_read.execute(meta.kv.hash_index);\
        hdr.kv_data.value_11 = value_##regIndex11##_read.execute(meta.kv.hash_index);\
        hdr.kv_data.value_12 = value_##regIndex12##_read.execute(meta.kv.hash_index);\
        hdr.kv_data.value_13 = value_##regIndex13##_read.execute(meta.kv.hash_index);\
        hdr.kv_data.value_14 = value_##regIndex14##_read.execute(meta.kv.hash_index);\
        hdr.kv_data.value_15 = value_##regIndex15##_read.execute(meta.kv.hash_index);\
        hdr.kv_data.value_16 = value_##regIndex16##_read.execute(meta.kv.hash_index);\
        hdr.kv_data.value_17 = value_##regIndex17##_read.execute(meta.kv.hash_index);\
        hdr.kv_data.value_18 = value_##regIndex18##_read.execute(meta.kv.hash_index);\
        hdr.kv_data.value_19 = value_##regIndex19##_read.execute(meta.kv.hash_index);\
    }

// the number of VALUE_READ_COL_RUN equals (VALUE_COLNUM)
#define VALUE_READ_COLS_RUN()\
    VALUE_READ_COL_RUN(0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19)

    // VALUE_READ_COL_RUN(0, 0, 1, 2, 3)\
    // VALUE_READ_COL_RUN(1, 4, 5, 6, 7)\
    // VALUE_READ_COL_RUN(2, 8, 9, 10, 11)\
    // VALUE_READ_COL_RUN(3, 12, 13, 14, 15)

// the number of value_##...##_update equals (VALUE_REGNUM)
#define VALUE_MODIFY_COL_RUN(colIndex, regIndex0, regIndex1, regIndex2, regIndex3, regIndex4, regIndex5, regIndex6, regIndex7, regIndex8, regIndex9, regIndex10, regIndex11, regIndex12, regIndex13, regIndex14, regIndex15, regIndex16, regIndex17, regIndex18, regIndex19)\
    value_##regIndex0##_update.execute(hdr.ctrl_opHashInfo.hash_index);\
    value_##regIndex1##_update.execute(hdr.ctrl_opHashInfo.hash_index);\
    value_##regIndex2##_update.execute(hdr.ctrl_opHashInfo.hash_index);\
    value_##regIndex3##_update.execute(hdr.ctrl_opHashInfo.hash_index);\
    value_##regIndex4##_update.execute(hdr.ctrl_opHashInfo.hash_index);\
    value_##regIndex5##_update.execute(hdr.ctrl_opHashInfo.hash_index);\
    value_##regIndex6##_update.execute(hdr.ctrl_opHashInfo.hash_index);\
    value_##regIndex7##_update.execute(hdr.ctrl_opHashInfo.hash_index);\
    value_##regIndex8##_update.execute(hdr.ctrl_opHashInfo.hash_index);\
    value_##regIndex9##_update.execute(hdr.ctrl_opHashInfo.hash_index);\
    value_##regIndex10##_update.execute(hdr.ctrl_opHashInfo.hash_index);\
    value_##regIndex11##_update.execute(hdr.ctrl_opHashInfo.hash_index);\
    value_##regIndex12##_update.execute(hdr.ctrl_opHashInfo.hash_index);\
    value_##regIndex13##_update.execute(hdr.ctrl_opHashInfo.hash_index);\
    value_##regIndex14##_update.execute(hdr.ctrl_opHashInfo.hash_index);\
    value_##regIndex15##_update.execute(hdr.ctrl_opHashInfo.hash_index);\
    value_##regIndex16##_update.execute(hdr.ctrl_opHashInfo.hash_index);\
    value_##regIndex17##_update.execute(hdr.ctrl_opHashInfo.hash_index);\
    value_##regIndex18##_update.execute(hdr.ctrl_opHashInfo.hash_index);\
    value_##regIndex19##_update.execute(hdr.ctrl_opHashInfo.hash_index);\

#define VALUE_MODIFY_COLS_RUN()\
    VALUE_MODIFY_COL_RUN(0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19)

    // VALUE_MODIFY_COL_RUN(0, 0, 1, 2, 3)\
    // VALUE_MODIFY_COL_RUN(1, 4, 5, 6, 7)\
    // VALUE_MODIFY_COL_RUN(2, 8, 9, 10, 11)\
    // VALUE_MODIFY_COL_RUN(3, 12, 13, 14, 15)


// #define CPU_CONTROLLER 24

#endif