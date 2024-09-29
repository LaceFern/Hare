//
// Created by Alex_Li on 2022/7/19.
//

#ifndef P4_LOCK_CONFIG_H
#define P4_LOCK_CONFIG_H

// #define SLOT_SIZE 45056

#define KEY_BYTES 16 // = KEY_WIDTH / 8
#define VALUE_BYTES 16 // = VALUE_WIDTH / 8
#define KEY_COLNUM 1
#define KEY_COLNUM_HW 5
#define KEY_ROWNUM 32768//16384//32768
#define VALUE_COLNUM 1
#define VALUE_COLNUM_HW 5

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

#endif //P4_LOCK_CONFIG_H
