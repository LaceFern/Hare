//
// Created by Alicia on 2023/9/4.
//

#include <iostream>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include "general_ctrl.h"
#include "p4_ctrl.h"


void SwitchControlPlane::transfer_init() {
    // config mac forwarding
    {
        std::cout << "Initiating mac forward table..." << std::endl;

        auto &forward = program->getTable("Ingress.mac_forward");

        auto dst_addr_key_id = forward.getKey("hdr.ethernet.dst_addr");
        auto send_action_id = forward.getAction("Ingress.send");
        auto drop_action_id = forward.getAction("Ingress.drop");
        auto send_action_port_data_id = forward.getData("port", "Ingress.send");
        auto multicast_action_id = forward.getAction("Ingress.multicast");
        auto multicast_action_port_data_id = forward.getData("mcast_grp", "Ingress.multicast");

        forward.keyHandlerReset();
        forward.keyHandlerSetValue(dst_addr_key_id, 0xb8599fe96b1c);
        forward.dataHandlerReset(send_action_id);
        forward.dataHandlerSetValue(send_action_port_data_id, (uint64_t) 40); // 11/-
        forward.tableEntryAdd();

        forward.keyHandlerReset();
        forward.keyHandlerSetValue(dst_addr_key_id, 0x98039bca4838);
        forward.dataHandlerReset(send_action_id);
        forward.dataHandlerSetValue(send_action_port_data_id, (uint64_t) 32); // 12/-
        forward.tableEntryAdd();

        forward.keyHandlerReset();
        forward.keyHandlerSetValue(dst_addr_key_id, 0x98039bc7c818);
        forward.dataHandlerReset(send_action_id);
        forward.dataHandlerSetValue(send_action_port_data_id, (uint64_t) 24); // 13/-
        forward.tableEntryAdd();

        forward.keyHandlerReset();
        forward.keyHandlerSetValue(dst_addr_key_id, 0x98039bc7c0a8);
        forward.dataHandlerReset(send_action_id);
        forward.dataHandlerSetValue(send_action_port_data_id, (uint64_t) 16); // 14/-
        forward.tableEntryAdd();

        forward.keyHandlerReset();
        forward.keyHandlerSetValue(dst_addr_key_id, 0x043f72deba44);
        forward.dataHandlerReset(send_action_id);
        forward.dataHandlerSetValue(send_action_port_data_id, (uint64_t) 8); // 15/-
        forward.tableEntryAdd();

        forward.keyHandlerReset();
        forward.keyHandlerSetValue(dst_addr_key_id, 0x98039bc7c7fc);
        forward.dataHandlerReset(send_action_id);
        forward.dataHandlerSetValue(send_action_port_data_id, (uint64_t) 0); // 16/-
        forward.tableEntryAdd();

        forward.keyHandlerReset();
        forward.keyHandlerSetValue(dst_addr_key_id, 0x98039bca4018);
        forward.dataHandlerReset(send_action_id);//forward.dataHandlerReset(drop_action_id);//
        forward.dataHandlerSetValue(send_action_port_data_id, (uint64_t) 4); // 17/-
        forward.tableEntryAdd();

        forward.keyHandlerReset();
        forward.keyHandlerSetValue(dst_addr_key_id, 0x0c42a12b0d70);
        forward.dataHandlerReset(send_action_id);
        forward.dataHandlerSetValue(send_action_port_data_id, (uint64_t) 12); // 18/-
        forward.tableEntryAdd();

        forward.keyHandlerReset();
        forward.keyHandlerSetValue(dst_addr_key_id, 0x98039bCA4008);
        forward.dataHandlerReset(send_action_id);
        forward.dataHandlerSetValue(send_action_port_data_id, (uint64_t) 28); // 20/-
        forward.tableEntryAdd();

        forward.keyHandlerReset();
        forward.keyHandlerSetValue(dst_addr_key_id, 0x0090fb7063c7);
        forward.dataHandlerReset(send_action_id);
        // forward.dataHandlerSetValue(send_action_port_data_id, (uint64_t) 66); //
        forward.dataHandlerSetValue(send_action_port_data_id, (uint64_t) 64); // 33/0
        forward.tableEntryAdd();

        // 33/2�����˿ڣ� cpu control
        forward.keyHandlerReset();
        forward.keyHandlerSetValue(dst_addr_key_id, 0x0090fb7063c6);
        forward.dataHandlerReset(send_action_id);
        // forward.dataHandlerSetValue(send_action_port_data_id, (uint64_t) 64); //
        forward.dataHandlerSetValue(send_action_port_data_id, (uint64_t) 66); // 33/2
        forward.tableEntryAdd();

        // pcie port (4 switch data plane)
        forward.keyHandlerReset();
        forward.keyHandlerSetValue(dst_addr_key_id, 0x0090fb7063c5);
        forward.dataHandlerReset(send_action_id);
        forward.dataHandlerSetValue(send_action_port_data_id, (uint64_t) 66); //192
        forward.tableEntryAdd();

        //multicast
        forward.keyHandlerReset();
        forward.keyHandlerSetValue(dst_addr_key_id, 0xffffffffffff);
        forward.dataHandlerReset(multicast_action_id);
        forward.dataHandlerSetValue(multicast_action_port_data_id, (uint64_t) 1); // 18/-
        forward.tableEntryAdd();
    }

    // config ports and mirroring
    {
        std::cout << "Initiating ports..." << std::endl;
        
        program->portAdd("11/0", BF_SPEED_100G, BF_FEC_TYP_RS); //40
        program->portAdd("12/0", BF_SPEED_100G, BF_FEC_TYP_RS); //32
        program->portAdd("13/0", BF_SPEED_100G, BF_FEC_TYP_RS); //24
        program->portAdd("14/0", BF_SPEED_100G, BF_FEC_TYP_RS); //16
        program->portAdd("15/0", BF_SPEED_100G, BF_FEC_TYP_RS); //8
        // program->portAdd("16/0", BF_SPEED_100G, BF_FEC_TYP_RS); //0
        program->portAdd("17/0", BF_SPEED_100G, BF_FEC_TYP_RS); //4
        program->portAdd("18/0", BF_SPEED_100G, BF_FEC_TYP_RS); //12
        program->portAdd("20/0", BF_SPEED_100G, BF_FEC_TYP_RS); //28
        program->portAdd("33/0", BF_SPEED_10G, BF_FEC_TYP_NONE); //64
        program->portAdd("33/2", BF_SPEED_10G, BF_FEC_TYP_NONE); //66

        //multicast
        program->configMulticast(1, 11, {40, 32, 24, 16, 8, 0, 4, 12, 28});
    }

//    std::cout << "input 1 to initiate APP and CTRL:" << std::endl;
//    uint32_t start_word;
//    std::cin >> start_word;
//    while(1);


    // ctrl function
    mapping = &program->getTable("Ingress.mapping_table_0");
    mapping_key = mapping->getKey("hdr.app.objseg_0");
    mapping_action = mapping->getAction("Ingress.mapping_action_0");
    mapping_data = mapping->getData("index", "Ingress.mapping_action_0");


    {
        std::cout << "Initiating mapping..." << std::endl;
        std::cout << "Init obj from obj id " << init_begin << " to obj id " << init_end << std::endl;

        program->beginBatch();
        for (uint64_t i = init_begin; i < init_end; ++i) {
            uint_obj obj = i;
            uint32_t obj_index = i;

            obj_2_obj_index_mapping[obj] = obj_index;
            obj_index_2_obj_mapping[obj_index] = obj;

            uint8_t obj_small_end[NUM_OBJ_BYTE] = {0};
            uint8_t obj_big_end[NUM_OBJ_BYTE] = {0};
            ((uint_obj *)obj_small_end)[0] = obj;
            for (int j = 0; j < NUM_OBJ_BYTE; j++) {
                obj_big_end[NUM_OBJ_BYTE - j] = obj_small_end[j];
            }

            mapping->keyHandlerReset();
            mapping->keyHandlerSetValue(mapping_key, obj_big_end, NUM_OBJ_BYTE);
            mapping->dataHandlerReset(mapping_action);
            mapping->dataHandlerSetValue(mapping_data, (uint64_t)i);
            mapping->tableEntryAdd();
        }
        program->endBatch(true);
    }

    // init precise counter
    {
        std::cout << "Initiating precise counter..." << std::endl;

        for (int i = 0; i < counter_reg_num; ++i) {
//            pcounter_vector.push_back(&program->getTable("counter_bucket_reg_" + std::to_string(i)));
//            pcounter_key_vector.push_back(pcounter_vector[i]->getKey("$REGISTER_INDEX"));
//            pcounter_data_vector.push_back(
//                    pcounter_vector[i]->getData("Ingress.counter_bucket_reg_" + std::to_string(i) + ".f1"));
            pcounter_vector.push_back(&program->getTable("Ingress.counter_reg"));
            pcounter_key_vector.push_back(pcounter_vector[i]->getKey("$REGISTER_INDEX"));
            pcounter_data_vector.push_back(
                    pcounter_vector[i]->getData("Ingress.counter_reg.f1"));
        }

        if(debug_flag == 1){
            for (int i = 0; i < counter_reg_num; ++i) {
                for(int j = 0; j < MAX_CACHE_SIZE; j++){
                    pcounter_vector[i]->keyHandlerReset();
                    pcounter_vector[i]->keyHandlerSetValue(
                            pcounter_key_vector[i], j);
                    pcounter_vector[i]->dataHandlerReset();
                    pcounter_vector[i]->dataHandlerSetValue(
                            pcounter_data_vector[i], (uint64_t) j);
                    pcounter_vector[i]->tableEntryModify();
                    pcounter_vector[i]->completeOperations();
                }
            }
        }
    }

    // init state manager
    state = &program->getTable("Ingress.state_reg");
    state_key = state->getKey("$REGISTER_INDEX");
    state_data = state->getData("Ingress.state_reg.f1");
    {
        std::cout << "Initiating state..." << std::endl;
        std::cout << "Init obj from obj id " << init_begin << " to obj id " << init_end << std::endl;

        program->beginBatch();
        for(uint64_t i = init_begin; i < init_end; ++i)
        {
            state->keyHandlerReset();
            state->keyHandlerSetValue(state_key, i);
            state->dataHandlerReset();
            state->dataHandlerSetValue(state_data, (uint64_t) 0);
            state->tableEntryModify();
            state->completeOperations();
        }
        program->endBatch(true);
    }
}

void SwitchControlPlane::modify_hitcheck_MAT(upd_obj_info *uo_info_list, uint32_t uo_num) {

    // begin sniff
    std::cout << "-----------------------------------\n"
              << "Service begin" << std::endl;

    // program->beginBatch();
    error_count = 0;
    for (int i = 0; i < uo_num; ++i) {
        upd_obj_info uo_info = uo_info_list[i];
        uint_obj upd_obj = uo_info.hot_obj;
        uint32_t upd_obj_index = uo_info.cold_obj_index;
        
        auto index_iter = obj_2_obj_index_mapping.find(upd_obj);
        if (index_iter == obj_2_obj_index_mapping.cend()) {

            auto reverse_iter = obj_index_2_obj_mapping.find(upd_obj_index);
            if (reverse_iter != obj_index_2_obj_mapping.cend()) {
                
                obj_2_obj_index_mapping.erase(reverse_iter->second);

                uint8_t obj_small_end[NUM_OBJ_BYTE] = {0};
                uint8_t obj_big_end[NUM_OBJ_BYTE] = {0};
                ((uint_obj *)obj_small_end)[0] = upd_obj;
                for (int j = 0; j < NUM_OBJ_BYTE; j++) {
                    obj_big_end[NUM_OBJ_BYTE - j] = obj_small_end[j];
                }
                
                mapping->keyHandlerReset();
                mapping->keyHandlerSetValue(mapping_key, obj_big_end, NUM_OBJ_BYTE);
                mapping->tableEntryDelete();
                mapping->completeOperations();

                if (debug_flag) std::cout << "{" << reverse_iter->second << ", " << upd_obj_index << "} ==> ";
            }

            obj_2_obj_index_mapping[upd_obj] = upd_obj_index;
            obj_index_2_obj_mapping[upd_obj_index] = upd_obj;

            uint8_t obj_small_end[NUM_OBJ_BYTE] = {0};
            uint8_t obj_big_end[NUM_OBJ_BYTE] = {0};
            ((uint_obj *)obj_small_end)[0] = upd_obj;
            for (int j = 0; j < NUM_OBJ_BYTE; j++) {
                obj_big_end[NUM_OBJ_BYTE - j] = obj_small_end[j];
            }

            mapping->keyHandlerReset();
            mapping->keyHandlerSetValue(mapping_key, obj_big_end, NUM_OBJ_BYTE);
            mapping->dataHandlerReset(mapping_action);
            mapping->dataHandlerSetValue(mapping_data, (uint64_t) upd_obj_index);
            mapping->tableEntryAdd();
            mapping->completeOperations();

            pcounter_vector[upd_obj_index % counter_reg_num]->keyHandlerReset();
            pcounter_vector[upd_obj_index % counter_reg_num]->keyHandlerSetValue(
                    pcounter_key_vector[upd_obj_index % counter_reg_num], upd_obj_index / counter_reg_num);
            pcounter_vector[upd_obj_index % counter_reg_num]->dataHandlerReset();
            pcounter_vector[upd_obj_index % counter_reg_num]->dataHandlerSetValue(
                    pcounter_data_vector[upd_obj_index % counter_reg_num], (uint64_t) 0xffffffff);
            pcounter_vector[upd_obj_index % counter_reg_num]->tableEntryModify();
            pcounter_vector[upd_obj_index % counter_reg_num]->completeOperations();

            if (debug_flag) std::cout << "{" << upd_obj << ", " << upd_obj_index << "}" << std::endl;
        } else {
            if (debug_flag)
                std::cout << "{" << upd_obj << ", " << index_iter->second << "} <=> {" << upd_obj << ", " << upd_obj_index << "}"
                     << std::endl;
            error_count++;
        }

        state->keyHandlerReset();
        state->keyHandlerSetValue(state_key, upd_obj_index);
        state->dataHandlerReset();
        state->dataHandlerSetValue(state_data, (uint64_t) 0);
        state->tableEntryModify();
        state->completeOperations();

        if(debug_flag) std::cout << "Successfully handle " << uo_num - error_count << " hot replace request" << std::endl;
    }
    // program->endBatch(true);
}

