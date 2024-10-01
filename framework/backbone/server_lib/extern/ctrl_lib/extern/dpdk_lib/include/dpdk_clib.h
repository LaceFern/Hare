//
// Created by Alicia on 2023/8/28.
//

#ifndef CONTROLLER_V1_DPDK_CLIB_H
#define CONTROLLER_V1_DPDK_CLIB_H

/******************** dpdk c lib *********************/
extern "C" {
#include <rte_memory.h>
#include <rte_memzone.h>
#include <rte_launch.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_lcore.h>
#include <rte_debug.h>
#include <rte_cycles.h>
#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>
#include <rte_ethdev.h>
}

#endif //CONTROLLER_V1_DPDK_CLIB_H
