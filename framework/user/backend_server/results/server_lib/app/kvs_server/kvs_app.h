//
// Created by Alicia on 2023/9/6.
//

#ifndef KVS_SERVER_KVS_APP_H
#define KVS_SERVER_KVS_APP_H

#define KV_NUM 2000000
#define KEY_BYTE_NUM 16
#define VALUE_BYTE_NUM 128

#define OP_GET                  1
#define OP_GETSUCC              6
#define OP_GETFAIL              7

typedef uint32_t uint_key;
typedef uint32_t uint_value;

class KVS{
private:
    std::unordered_map<uint_key, uint_value> KV_mapping;
public:
    void init_kv_mapping();
};

struct kvs_request_payload{
    uint8_t op_type;
    uint8_t key_byte_list[KEY_BYTE_NUM];
};

struct kvs_response_payload{
    uint8_t op_type;
    uint8_t key_byte_list[KEY_BYTE_NUM];
    uint8_t value_byte_list[VALUE_BYTE_NUM];
};

#endif //KVS_SERVER_KVS_APP_H
