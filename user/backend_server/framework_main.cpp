/* 
class User{
    uint8_t query_payload[PAYLOAD_SIZE];
    uint8_t response_payload[PAYLOAD_SIZE];
    void receive_query();
    void process_query();
    void send_response(query_payload_len);
    void update_statistics(uint8_t obj[OBJECT_SIZE]);
	int is_replacing(uint8_t obj[OBJECT_SIZE]); // Return 0: not; return 1: yes. 

    uint8_t control_payload[PAYLOAD_SIZE];
    uint32_t replace_num; // The number of the offloaded objects that need to be replaced.
    uint32_t hit_index_list[K]; // The hit_indexes of the offloaded objects that need to be replaced.
    uint8_t obj_switch_list[K][OBJECT_SIZE]; // The offloaded objects that need to be replaced.
    uint8_t obj_server_list[K][OBJECT_SIZE]; // The non-offloaded objects that are going to replace the offloaded objects.
    void move_appdata_server2switch(); // Move APP data  from the switch data plane to the backend server.
    void move_appdata_switch2server(); // Move APP data  from the backend server to the switch data plane.
    void send_control_packet();
    void receive_control_packet();
}
*/

#include <unordered_map>
#include "kvs.hpp"

void User::global_init(){
	std::unordered_map<uint_key, uint_value> KVmapping;
    for(uint32_t i = 0; i < KV_NUM; i++){
        uint_key key = i;
        uint_value value = i;
        KV_mapping[key] = value;
    }
}

void User::process_query(){
	user->receive_query(sizeof(send_kr_payload));
	auto rcv_kr_payload = (kvs_request_payload *)(user->query_payload);
	
	uint_key key = rcv_kr_payload->key_byte_list[0] |
                    rcv_kr_payload->key_byte_list[1] << 8 |
                    rcv_kr_payload->key_byte_list[2] << 16 |
                    rcv_kr_payload->key_byte_list[3] << 24;

    if(rcv_kr_payload->op_type == OP_GET && is_replacing(rcv_kr_payload->key_byte_list)){
		auto send_kr_payload = (kvs_response_payload *)(user->response_payload);
    	auto it = KVmapping.find(key);
    	if (it != myMap.end()) {
			send_kr_payload->op_type = OP_GETSUCC;
    	} else {
			send_kr_payload->op_type = OP_GETFAIL;
            for(uint32_t j = 0; j < KEY_BYTE_NUM; j++){
                send_kr_payload->key_byte_list[j] = rcv_kr_payload->key_byte_list[j];
            }
            for(uint32_t j = 0; j < VALUE_BYTE_NUM; j++){
                send_kr_payload->value_byte_list[j] = 0;
            }				
			send_kr_payload->value_byte_list[0] = value & 0xff;
			send_kr_payload->value_byte_list[1] = (value >> 8) & 0xff;
			send_kr_payload->value_byte_list[2] = (value >> 16) & 0xff;
			send_kr_payload->value_byte_list[3] = (value >> 24) & 0xff;
			user->send_response(sizeof(send_kr_payload));
    	}
		stats_update(rcv_kr_payload->key_byte_list);
	}
}

void User::move_appdata_server2switch(){
	auto send_dm_payload = (data_movement_payload *)(user->control_payload);
	for(int i = 0; i < replace_num; i++){
		uint_key key = obj_server_list[i][3] << 24 | 
						obj_server_list[i][2] << 16 | 
						obj_server_list[i][1] << 8 | 
						obj_server_list[i][0];
		auto it = KVmapping.find(key);
		if (it != myMap.end()){
			send_dm_payload->op_type = OP_PUT;
			send_dm_payload->index = hit_index_list[i];
            for(uint32_t j = 0; j < VALUE_BYTE_NUM; j++){
                send_dm_payload->value_byte_list[j] = 0;
            }
			uint_value value = KVmapping[key];
			send_dm_payload->value_byte_list[0] = value & 0xff;
			send_dm_payload->value_byte_list[1] = (value >> 8) & 0xff;
			send_dm_payload->value_byte_list[2] = (value >> 16) & 0xff;
			send_dm_payload->value_byte_list[3] = (value >> 24) & 0xff;
		}
	}
}

User::move_appdata_switch2server(){;}
