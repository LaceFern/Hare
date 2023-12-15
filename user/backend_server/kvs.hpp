
#ifndef KVS_SERVER_KVS_APP_H
#define KVS_SERVER_KVS_APP_H

#define KV_NUM 2000000
#define KEY_BYTE_NUM 80
#define VALUE_BYTE_NUM 80

#define OP_GET                  1
#define OP_PUT                  2
#define OP_GETSUCC              6
#define OP_GETFAIL              7

typedef uint32_t uint_key;
typedef uint32_t uint_value;

struct kvs_request_payload{
    uint8_t op_type;
    uint8_t key_byte_list[KEY_BYTE_NUM];
};

struct kvs_response_payload{
    uint8_t op_type;
    uint8_t key_byte_list[KEY_BYTE_NUM];
    uint8_t value_byte_list[VALUE_BYTE_NUM];
};

struct data_movement_payload{
    uint8_t op_type;
    uint32_t index;
    uint8_t value_byte_list[VALUE_BYTE_NUM];
};

#endif //KVS_SERVER_KVS_APP_H
