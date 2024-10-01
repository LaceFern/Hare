//
// Created by Alicia on 2023/9/6.
//

#include <iostream>
#include <unordered_map>
#include "kvs_app.h"

void KVS::init_kv_mapping() {
    for(uint32_t i = 0; i < KV_NUM; i++){
        uint_key key = i;
        uint_value value = i;
        KV_mapping[key] = value;
    }
}

