#ifndef _CONFIG_P4_
#define _CONFIG_P4_


/* USER DEFININATION begins*/
#define OP_GET                  1
#define OP_GETSUCC              6
#define OP_GETFAIL              7
#define VALSEG_WIDTH 32
#define CACHE_SIZE 32768
#define UDP_CTRL_PORT 1024
/* USER DEFININATION begins*/

#define INVALID_4B 0xffffffff

#define STATE_NOUPD 0
#define STATE_UPD 1

//number of bits
#define OBJIDX_WIDTH 32 //16
#define COUNTER_WIDTH 32
#define OBJSEG_WIDTH 128
#define STATE_WIDTH 8

#define OP_LOCK_STAT_SWITCH 0x0C
#define OP_LOCK_STAT_FPGA 12
#define OP_GET_COLD_COUNTER_SWITCH 0x08
#define OP_GET_HOT_COUNTER_FPGA 18
#define OP_GET_END_FPGA 20
#define OP_GET_END_2_UPDATE_FPGA 21
#define OP_GET_END_2_CLEAN_FPGA 22

#define OP_QUEUE_EMPTY_SWITCH 0x11
#define OP_QUEUE_EMPTY_FPGA 0x11
#define OP_HOT_REPORT_SWITCH 0x0E
//#define OP_LOCK_COLD_QUEUE_SWITCH 0x0A
//#define OP_LOCK_HOT_QUEUE_FPGA 19
#define OP_MOD_STATE_UPD_SWITCH 0x0A
#define OP_MOD_STATE_UPD_FPGA 19
#define OP_MOD_STATE_NOUPD_SWITCH 0x0A
#define OP_MOD_STATE_NOUPD_FPGA 19
#define OP_CLN_COLD_COUNTER_SWITCH 0x09
#define OP_CLN_HOT_COUNTER_FPGA 9
#define OP_UNLOCK_STAT_SWITCH 0x0D
#define OP_UNLOCK_STAT_FPGA 13
#define OP_UNLOCK_HOT_QUEUE_FPGA 15

// forward
//#define FPGA_CONTROLLER 60
#define CONTROL_PLANE_ETH   66
//#define CPU_CONTROLLER  24
#define CONTROL_PLANE_PCIE   192
#define AMAX1  40

// #define IDX_WIDTH 16 // 2 ^ IDX_WIDTH == SLOT_SIZE
// #define IDX_PADDING_WIDTH 16 // = 32 - 1 (slot) - IDX_WIDTH
// #define HDR_PADDING_WIDTH 416 // 512 - 32 - 32 - 32



// #define MAP_WIDTH 32

// #define CNT_WIDTH 16 // range match only support 20 bits

// #define NODE_SIZE 1

// #define QUEUE_SIZE 8 // 32
// #define QUEUE_MASK 0x07 // 0x1f
// #define QUEUE_ITER_WIDTH 8

// #define SKETCH_DEPTH 16384

// // data type
// typedef bit<8> oper_t;
// #define OPER_MUSK 0x3f
// #define FLAG_MUSK 0xc0

// // lock type
// #define EXCLU_LOCK 1
// #define SHARE_LOCK 2

// // lock op code
// #define GRANT_LOCK 0x01
// #define RELEA_LOCK 0x02

// #define CHECK_HEAD 0x3f
// #define EMPTY_CALL 0x3e

// #define LOCK_GRANT_SUCC 0x03
// #define LOCK_GRANT_FAIL 0x04
// #define LOCK_RELEA_SUCC 0x05
// #define LOCK_RELEA_FAIL 0x06
// #define LOCK_QUEUE_SUCC 0x07

// #define OP_CNT_GET 0x08
// #define OP_CNT_CLR 0x09

// #define SUSP_FLAG_SET 0x0a
// #define SUSP_FLAG_RST 0x0b

// #define STAT_FLAG_SET 0x0c
// #define STAT_FLAG_RST 0x0d

// #define HOT_REPORT   0x0e
// #define REPLACE_SUCC 0x0f
// #define OP_CALLBACK  0x11

// #define LOCK_GRANT_CONGEST_FAIL 0x16
// #define LOCK_RELEA_CONGEST_FAIL 0x17

// #define RE_GRANT_LOCK 0x81
// #define RE_RELEA_LOCK 0x82

// #define RE_LOCK_GRANT_SUCC 0x83
// #define RE_LOCK_GRANT_FAIL 0x84
// #define RE_LOCK_RELEA_SUCC 0x85
// #define RE_LOCK_RELEA_FAIL 0x86
// #define RE_LOCK_QUEUE_SUCC 0x87

// kvs
// #define KEY_WIDTH 128

// #define OP_GET                  1		
// #define OP_DEL                  2		
// #define OP_PUT                  3
// #define OP_PUT_SWITCHHIT        4
// #define OP_PUT_SWITCHMISS	    5
// #define OP_GETSUCC              6		
// #define OP_GETFAIL              7		
// #define OP_DELSUCC              8		
// #define OP_DELFAIL              9		
// #define OP_PUTSUCC_SWITCHHIT    10
// #define OP_PUTSUCC_SWITCHMISS   11
// #define OP_PUTFAIL              12



// // congestion control
// #define CONGESTION_THRESHOLD 11 // ns

// // queue node op code
// #define NODE_SET 1
// #define NODE_RST 2
// #define NODE_GET 3


#endif //_CONFIG_P4_
