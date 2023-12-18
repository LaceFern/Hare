//
// Created by Alicia on 2023/9/4.
//

#ifndef CONTROLLER_P4_CTRL_H
#define CONTROLLER_P4_CTRL_H

#include "p4_program.h"
#include "p4_table.h"
#include "general_ctrl.h"

class SwitchControlPlane{
public:
    explicit SwitchControlPlane(const std::string& program_name) {
        program = new P4Program(program_name);
    };
public:
    void transfer_init();
    void modify_hitcheck_MAT(upd_obj_info *uo_info_list, uint32_t uo_num);
private:
    P4Program *program;
    // init entries
    int counter_reg_num = 1;
    int error_count = 0;
    std::unordered_map<uint_obj, uint32_t> obj_2_obj_index_mapping;
    std::unordered_map<uint32_t, uint_obj> obj_index_2_obj_mapping;

    P4Table *mapping{};
    bf_rt_id_t mapping_key{};
    bf_rt_id_t mapping_action{};
    bf_rt_id_t mapping_data{};

    std::vector<P4Table *> pcounter_vector;
    std::vector<bf_rt_id_t> pcounter_key_vector;
    std::vector<bf_rt_id_t> pcounter_data_vector;

    P4Table *state{};
    bf_rt_id_t state_key{};
    bf_rt_id_t state_data{};
};

#endif //CONTROLLER_P4_CTRL_H
