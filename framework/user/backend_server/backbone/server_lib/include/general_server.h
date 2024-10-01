//
// Created by Alicia on 2023/9/6.
//

#ifndef KVS_SERVER_GENERAL_SERVER_H
#define KVS_SERVER_GENERAL_SERVER_H

#define SERVER_MAC "98:03:9b:ca:40:18" // switch control plane enp4s0f0
#define SERVER_IP "10.0.0.7"


static int32_t user_loop(uint32_t lcore_id, uint32_t rx_queue_id);

#endif //KVS_SERVER_GENERAL_SERVER_H
