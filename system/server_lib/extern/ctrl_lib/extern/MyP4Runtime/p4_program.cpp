//
// Created by Alex_Li on 2022/7/6.
//

#include <iostream>
#include "p4_program.h"
#include "p4_table.h"

P4Program::P4Program(const std::string &program_name)
{
	if(getenv("SDE") == nullptr)
		throw std::runtime_error("Environment variable SDE not set");
	if(getenv("SDE_INSTALL") == nullptr)
		throw std::runtime_error("Environment variable SDE_INSTALL not set");

	const std::string SDE(getenv("SDE"));
	const std::string SDE_INSTALL(getenv("SDE_INSTALL"));

	init(program_name,
	     SDE_INSTALL,
	     SDE_INSTALL + "/share/p4/targets/tofino/" + program_name + ".conf"
	);

	BF_TRY(p4_pd_client_init(&mirror_session));
	BF_TRY(bf_mc_create_session(&multicast_session));
}

P4Program::P4Program(const std::string &program_name, const std::string &install_dir, const std::string &conf_file, uint16_t dev_sts_port)
{
	init(program_name,
		 install_dir,
		 conf_file,
		 dev_sts_port);

	BF_TRY(p4_pd_client_init(&mirror_session));
	BF_TRY(bf_mc_create_session(&multicast_session));
}

P4Program::~P4Program()
{
	if(switchd_context)
	{
		delete switchd_context->install_dir;
		delete switchd_context->conf_file;
		delete switchd_context;
	}

	BF_TRY(p4_pd_client_cleanup(mirror_session));
	BF_TRY(bf_mc_destroy_session(multicast_session));
}

void P4Program::init(const std::string &program_name, const std::string &install_dir, const std::string &conf_file, uint16_t dev_sts_port)
{
	std::cout << "Start initiating..." << std::endl;
	initSwitchd(install_dir, conf_file, dev_sts_port);
	openSession();
	connect2Dev(program_name);
}

void P4Program::initSwitchd(const std::string &install_dir, const std::string &conf_file, uint16_t dev_sts_port)
{
	std::cout << "Initiating switchd..." << std::endl;

	if((switchd_context = (bf_switchd_context_t *) calloc(1, sizeof(bf_switchd_context_t))) == nullptr)
	{
		throw std::runtime_error("Cannot Allocate switchd context");
	}

	switchd_context->install_dir = strdup(install_dir.data());
	switchd_context->conf_file = strdup(conf_file.data());

	switchd_context->running_in_background = true;

	switchd_context->dev_sts_thread = true;
	switchd_context->dev_sts_port = dev_sts_port;

	switchd_context->kernel_pkt = true;

	std::cout << "Initiating switchd lib..." << std::endl;
	bf_switchd_lib_init(switchd_context);
}

void P4Program::openSession()
{
	std::cout << "Opening session..." << std::endl;
	session = bfrt::BfRtSession::sessionCreate();
}

void P4Program::connect2Dev(const std::string &program_name)
{
	std::cout << "Connecting to device..." << std::endl;
	device_target.dev_id = 0;
	device_target.pipe_id = 0xffff; // All pipes

	BF_TRY(bfrt::BfRtDevMgr::getInstance().bfRtInfoGet(device_target.dev_id, program_name, &bfrt_info));
}

void P4Program::initAllTables()
{
	std::vector<const bfrt::BfRtTable *> table_ptr;
	BF_TRY(bfrt_info->bfrtInfoGetTables(&table_ptr));
	for(auto p : table_ptr)
	{
		std::string table_name;
		BF_TRY(p->tableNameGet(&table_name));
		tables.emplace(table_name, P4Table(*this, p));
	}
}

void P4Program::setTable(const std::string &table_name)
{
	const bfrt::BfRtTable *table;
	bf_status_t status = bfrt_info->bfrtTableFromNameGet(table_name, &table);
	if(status == BF_SUCCESS)
	{
		tables.emplace(table_name, P4Table(*this, table));
	}
	else
	{
		throw std::out_of_range("No such table");
	}
}

P4Table& P4Program::getTable(const std::string &table_name)
{
	if(tables.find(table_name) == tables.cend())
	{
		setTable(table_name);
	}
	return tables.at(table_name);
}

void P4Program::configPort(bf_pal_front_port_handle_t *port_hdl, bf_port_speed_e speed, bf_fec_type_e fec, bf_pm_port_autoneg_policy_e an_policy) const
{
	BF_TRY(bf_pm_port_add(device_target.dev_id, port_hdl, speed, fec));
	BF_TRY(bf_pm_port_autoneg_set(device_target.dev_id, port_hdl, an_policy));
	BF_TRY(bf_pm_port_enable(device_target.dev_id, port_hdl));
}

void P4Program::portAdd(const std::string &port, bf_port_speed_e speed, bf_fec_type_e fec, bf_pm_port_autoneg_policy_e an_policy) const
{
	bf_pal_front_port_handle_t port_hdl;
	BF_TRY(bf_pm_port_str_to_hdl_get(device_target.dev_id, port.data(), &port_hdl));
	configPort(&port_hdl, speed, fec, an_policy);
}

void P4Program::portAdd(bf_dev_port_t port, bf_port_speed_e speed, bf_fec_type_e fec, bf_pm_port_autoneg_policy_e an_policy) const
{
	bf_pal_front_port_handle_t port_hdl;
	BF_TRY(bf_pm_port_dev_port_to_front_panel_port_get(device_target.dev_id, port, &port_hdl));
	configPort(&port_hdl, speed, fec, an_policy);
}

void P4Program::configMirroring(p4_pd_mirror_id_t id, uint16_t port, p4_pd_mirror_type_e type, p4_pd_direction_t dir, bool egr_port_v, uint16_t max_pkt_len) const
{
	p4_pd_dev_target_t pd_dev_tgt = {
			device_target.dev_id,
			device_target.pipe_id
	};

//	p4_pd_mirror_session_info_t mirror_session_info = {
//			.type        = PD_MIRROR_TYPE_NORM, // Not sure
//			.dir         = dir,
//			.id          = id,
//			.egr_port    = port,
//			.egr_port_v  = true,
//			.max_pkt_len = 16384 // Refer to example in Barefoot Academy
//	};
	p4_pd_mirror_session_info_t mirror_session_info;
	mirror_session_info.type        = type;
	mirror_session_info.dir         = dir;
	mirror_session_info.id          = id;
	mirror_session_info.egr_port    = port;
	mirror_session_info.egr_port_v  = egr_port_v;
	mirror_session_info.max_pkt_len = max_pkt_len;

	BF_TRY(p4_pd_mirror_session_create(mirror_session, pd_dev_tgt, &mirror_session_info));
}

void P4Program::createMulticastGroup(bf_mc_grp_id_t gid, bool force)
{
	if(force || multicast_group_hdl.find(gid) == multicast_group_hdl.cend())
	{
		bf_mc_mgrp_create(multicast_session, device_target.dev_id, gid, &multicast_group_hdl[gid]);
	}
}

void P4Program::createMulticastNode(bf_mc_rid_t rid, const std::vector<bf_dev_port_t> &ports, const std::vector<bf_dev_port_t> &lags, bool force)
{
	if(force || multicast_node_hdl.find(rid) == multicast_node_hdl.cend())
	{
		bf_mc_port_map_t mc_port_map;
		BF_MC_PORT_MAP_INIT(mc_port_map);
		for(auto p : ports)
		{
			BF_MC_PORT_MAP_SET(mc_port_map, p);
		}

		bf_mc_lag_map_t mc_lag_map;
		BF_MC_LAG_MAP_INIT(mc_lag_map);
		for(auto g : lags)
		{
			BF_MC_LAG_MAP_SET(mc_lag_map, g);
		}

		bf_mc_node_create(multicast_session, device_target.dev_id, rid, mc_port_map, mc_lag_map, &multicast_node_hdl[rid]);
	}
}

void P4Program::associateMulticastNode(bf_mc_grp_id_t gid, bf_mc_rid_t rid, bool force)
{
	if(force || multicast_edge.count({gid, rid}) == 0)
	{
		multicast_edge.emplace(gid, rid);
		bf_mc_associate_node(multicast_session, device_target.dev_id, multicast_group_hdl[gid], multicast_node_hdl[rid], false, 0);
	}
}

void P4Program::configMulticast(bf_mc_grp_id_t gid, bf_mc_rid_t rid, const std::vector<bf_dev_port_t> &ports)
{
	createMulticastGroup(gid);
	createMulticastNode(rid, ports);
	associateMulticastNode(gid, rid);
	bf_mc_complete_operations(multicast_session);
}

void P4Program::idle()
{
	pthread_join(switchd_context->tmr_t_id, nullptr);
	pthread_join(switchd_context->dma_t_id, nullptr);
	pthread_join(switchd_context->int_t_id, nullptr);
	pthread_join(switchd_context->pkt_t_id, nullptr);
	pthread_join(switchd_context->port_fsm_t_id, nullptr);
	pthread_join(switchd_context->drusim_t_id, nullptr);
	pthread_join(switchd_context->accton_diag_t_id, nullptr);
}

