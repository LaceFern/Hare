//
// Created by Alex_Li on 2022/7/6.
//

#ifndef MYP4RUNTIME_P4_PROGRAM_H
#define MYP4RUNTIME_P4_PROGRAM_H


extern "C"
{
#include <bf_switchd/bf_switchd.h>
#include <bf_pm/bf_pm_intf.h>
#include <mc_mgr/mc_mgr_intf.h>
#include <tofino/pdfixed/pd_conn_mgr.h>
#include <tofino/pdfixed/pd_mirror.h>
}

#include <bf_rt/bf_rt_info.hpp>
#include <bf_rt/bf_rt_init.hpp>
#include <bf_rt/bf_rt_session.hpp>

class P4Table;

class P4Program
{
	friend class P4Table;

public:
	P4Program() = default;
	explicit P4Program(const std::string &program_name);
	P4Program(const std::string &program_name, const std::string& install_dir, const std::string& conf_file, uint16_t dev_sts_port = 7777);
	~P4Program();

public:
	void init(const std::string &program_name, const std::string &install_dir, const std::string &conf_file, uint16_t dev_sts_port = 7777);

private:
	bf_switchd_context_t *switchd_context{};
	void initSwitchd(const std::string &install_dir, const std::string &conf_file, uint16_t dev_sts_port);

private:
	std::shared_ptr<bfrt::BfRtSession> session;
	void openSession();

private:
	bf_rt_target_t device_target{};
	const bfrt::BfRtInfo *bfrt_info{};
	void connect2Dev(const std::string &program_name);

public:
	void initAllTables();
	void setTable(const std::string &table_name);
	P4Table& getTable(const std::string &table_name);

private:
	std::map<std::string, P4Table> tables;

public:
	inline bf_status_t completeOperations() const { return session->sessionCompleteOperations(); }
	inline bf_status_t beginBatch() const { return session->beginBatch(); }
	inline bf_status_t endBatch(bool hardwareSynchronous) const { return session->endBatch(hardwareSynchronous); }

public:
	void portAdd(const std::string &port, bf_port_speed_e speed, bf_fec_type_e fec, bf_pm_port_autoneg_policy_e an_policy = PM_AN_DEFAULT) const;
	void portAdd(bf_dev_port_t port, bf_port_speed_e speed, bf_fec_type_e fec, bf_pm_port_autoneg_policy_e an_policy = PM_AN_DEFAULT) const;

private:
	void configPort(bf_pal_front_port_handle_t *port_hdl,  bf_port_speed_e speed, bf_fec_type_e fec, bf_pm_port_autoneg_policy_e an_policy) const;

public:
	void configMirroring(p4_pd_mirror_id_t id, uint16_t port, p4_pd_mirror_type_e type = PD_MIRROR_TYPE_NORM, p4_pd_direction_t dir = PD_DIR_BOTH, bool egr_port_v = true, uint16_t max_pkt_len = 16384) const;
	void configMulticast(bf_mc_grp_id_t gid, bf_mc_rid_t rid, const std::vector<bf_dev_port_t> &ports);

private:
	void createMulticastGroup(bf_mc_grp_id_t gid, bool force = false);
	void createMulticastNode(bf_mc_rid_t rid, const std::vector<bf_dev_port_t> &ports, const std::vector<bf_dev_port_t> &lags = {}, bool force = false);
	void associateMulticastNode(bf_mc_grp_id_t gid, bf_mc_rid_t rid, bool force = false);

private:
	p4_pd_sess_hdl_t mirror_session{};
	bf_mc_session_hdl_t multicast_session{};
	std::unordered_map<bf_mc_grp_id_t, bf_mc_mgrp_hdl_t> multicast_group_hdl;
	std::unordered_map<bf_mc_rid_t, bf_mc_node_hdl_t> multicast_node_hdl;
	std::multiset<std::pair<bf_mc_grp_id_t, bf_mc_rid_t>> multicast_edge;

public:
	void idle();
};


#endif //MYP4RUNTIME_P4_PROGRAM_H
