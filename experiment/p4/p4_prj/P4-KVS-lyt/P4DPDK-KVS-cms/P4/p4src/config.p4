#ifndef _CONFIG_P4_
#define _CONFIG_P4_

#define SAMPLE_PORT 40
// forward
#define FPGA_CONTROLLER 60
#define CONTROL_PLANE   66 
#define CPU_CONTROLLER  4 //32


// data width
#define SLOT_SIZE 10000 //32768 // 45056 // 16384; // 22528; // 143360 (= 35 * 4096) maximum with 32 bits for single stage, 4 meter ALU as most 

#define IDX_WIDTH 16 // 2 ^ IDX_WIDTH == SLOT_SIZE
#define IDX_PADDING_WIDTH 16 // = 32 - 1 (slot) - IDX_WIDTH
#define HDR_PADDING_WIDTH 416 // 512 - 32 - 32 - 32

#define VAL_WIDTH 32

#define MAP_WIDTH 32

#define CNT_WIDTH 16 // range match only support 20 bits

#define NODE_SIZE 1

#define QUEUE_SIZE 8 // 32
#define QUEUE_MASK 0x07 // 0x1f
#define QUEUE_ITER_WIDTH 8

#define SKETCH_DEPTH 65536 //16384
#define FILTER_SKETCH_DEPTH 262144 //65536

// data type
typedef bit<8> oper_t;
#define OPER_MUSK 0x3f
#define FLAG_MUSK 0xc0

// lock type
#define EXCLU_LOCK 1
#define SHARE_LOCK 2

// lock op code
#define GRANT_LOCK 0x01
#define RELEA_LOCK 0x02

#define CHECK_HEAD 0x3f
#define EMPTY_CALL 0x3e

#define LOCK_GRANT_SUCC 0x03
#define LOCK_GRANT_FAIL 0x04
#define LOCK_RELEA_SUCC 0x05
#define LOCK_RELEA_FAIL 0x06
#define LOCK_QUEUE_SUCC 0x07

#define OP_CNT_GET 0x08
#define OP_CNT_CLR 0x09

#define SUSP_FLAG_SET 0x0a
#define SUSP_FLAG_RST 0x0b

#define STAT_FLAG_SET 0x0c
#define STAT_FLAG_RST 0x0d

#define HOT_REPORT   0x0e
#define REPLACE_SUCC 0x0f
#define LOCK_HOT_QUEUE  0x13
#define OP_CALLBACK  0x11

#define LOCK_GRANT_CONGEST_FAIL 0x16
#define LOCK_RELEA_CONGEST_FAIL 0x17

#define RE_GRANT_LOCK 0x81
#define RE_RELEA_LOCK 0x82

#define RE_LOCK_GRANT_SUCC 0x83
#define RE_LOCK_GRANT_FAIL 0x84
#define RE_LOCK_RELEA_SUCC 0x85
#define RE_LOCK_RELEA_FAIL 0x86
#define RE_LOCK_QUEUE_SUCC 0x87

// kvs
#define KEY_WIDTH 128

#define OP_GET                  1		
#define OP_DEL                  2		
#define OP_PUT                  3
#define OP_PUT_SWITCHHIT        4
#define OP_PUT_SWITCHMISS	    5
#define OP_GETSUCC              6		
#define OP_GETFAIL              7		
#define OP_DELSUCC              8		
#define OP_DELFAIL              9		
#define OP_PUTSUCC_SWITCHHIT    10
#define OP_PUTSUCC_SWITCHMISS   11
#define OP_PUTFAIL              12

// congestion control
#define CONGESTION_THRESHOLD 11 // ns

// queue node op code
#define NODE_SET 1
#define NODE_RST 2
#define NODE_GET 3

#define THRESHOLD 0x8

#endif //_CONFIG_P4_
