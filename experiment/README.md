## How to run Hare experiments

### Switch
SDE: 9.6.0 (if there is no $SDE_INSTALL/bin/bf_kpkt_mod_load or $SDE_INSTALL, using 9.9.0)

DPDK：21.05

KVS：./p4/p4_prj/P4-KVS-lyt/P4DPDK-KVS-noturn-scpcontrol

Lock：./p4/p4_prj/P4-Lock-lyt/P4-Lock-testlatency

#### Step 1. check SDE version

```bash
sudo update-alternatives --config bf-sde
```

#### Step 2. config environment variables

```bash
# SDE
export SDE=/root/Software/bf-sde
export SDE_INSTALL=$SDE/install
export PATH=$PATH:$SDE_INSTALL/bin
# export LD_LIBRARY_PATH=/usr/local/lib:$SDE_INSTALL/lib
```

#### Step 3. config PISA architecture

```bash
sudo -i
$SDE_INSTALL/bin/bf_kpkt_mod_load $SDE_INSTALL # load pisa driver
ifconfig enp6s0 up # bring up virtual network interface
```

#### Step 4. config dpdk-21.05

```bash
sudo -i
modprobe uio_pci_generic # load kernel module
dpdk-devbind.py --status # check status 
dpdk-devbind.py --bind=uio_pci_generic 04:00.0 # bind network interface to uio module
mount -t hugetlbfs pagesize=1GB /mnt/huge # mount huge page
```

#### Step 5. config parameters

Config parameters in controller/include_dpdk/cache_upd_ctrl.h

```c
#define MAX_CACHE_SIZE  32768
#define TOPK 800
#define NUM_BACKENDNODE 2
char* ip_backendNode_arr[NUM_BACKENDNODE] = {...}; //e.g., {"10.0.0.7", "10.0.0.8"}
struct rte_ether_addr mac_backendNode_arr[NUM_BACKENDNODE] = {...} //e.g., {.{addr_bytes = {0x98, 0x03, 0x9b, 0xc7, 0xc0, 0xa8}}, ...}
```

Config parameters in p4src/config.p4

```c
#define SLOT_SIZE 32768
```

#### Step 6. compile and run

```bash
mkdir build && cd build
cmake ../
cd p4src/p4build && make && make install && cd ../../
cd controller && make && cd ../
#
cd controller
./hello_bfrt -l 0
```
#### Debug

```bash
$SDE/run_bfshell.sh
ucli
pm show
```

************************************************************************************************
### Backend server

KVS：./server/server_code_kvs_noturn_testlatency/KVS_dpdk_server_code_amax7

Lock：./server/server_code_lock_noturn_testlatency/ver_UPD

#### Step 1. config parameters

Config parameters in ./include/cache_upd_ctrl.h

```c
#define MAX_CACHE_SIZE  32768
#define TOPK 800
#define NUM_BACKENDNODE 2
char* ip_backendNode_arr[NUM_BACKENDNODE] = {...};
struct rte_ether_addr mac_backendNode_arr[NUM_BACKENDNODE] = {...}
#define NUM_CLIENT 4
char* ip_client_arr[NUM_CLIENT] = {...};
struct rte_ether_addr mac_client_arr[NUM_CLIENT] = {...};
```

Config parameters in ./include/util.h

```c
#define IP_LOCAL                "..." //e.g., "10.0.0.7"
#define MAC_LOCAL               {...} //e.g., {0x98, 0x03, 0x9b, 0xc7, 0xc0, 0xa8}
```

#### Step 2. compile & run

```bash
make
sudo ./build/netlock_server_kvs -l 0-11,24-35 # KVS
# or
make
sudo ./build/netlock_server_lock -l 0-11,24-35 # Lock
```

************************************************************************************************

### Client

KVS project: ./client/netCache/client_speedlimit

Lock project: ./client/netLock/client

#### Step 1. config parameters

For each client server, config parameters in ./include/util.h

```c
#define IP_LOCAL                "..."
#define MAC_LOCAL               {...}
#define HC_MASTER               1 // master client:1; slave client:0. Master client inform slaves clients to change hotspot distribution.

#define HC_SLAVE_NUM            1 // number of slave clients
volatile int hc_flag = 0;
char* ip_dst_hc_arr[HC_SLAVE_NUM] = {...}; //slave clients' ips
struct rte_ether_addr mac_dst_hc_arr[HC_SLAVE_NUM] = {...}; //slave clients' macs

#define BN_NUM            2 // number of backend servers
char* ip_dst_arr[BN_NUM] = {...}; // backend servers' ips
struct rte_ether_addr mac_dst_arr[BN_NUM] = {...}; // backend servers' macs
```

#### Step 2. 编译&运行

```bash
make
sudo ./build/netlock_client -l 0-10
```

************************************************************************************************

### Probe

Project: ./probe/client_probe_noClientID

#### Step 1. compile & run

mention: the hc_flag in util.h should be are consistent with that in client-related files.

```bash
make
sudo ./build/netlock_probe -l 0-1
```

## How to run microbench about BFRT-MAT-insertion batching 

P4 Project: ./p4/p4_prj/P4-cp_api_batch

```bash
./hello_bfrt -p P4Lock
```

## How to run microbench about switch dp resource

P4 Project: ./p4/p4_prj/P4-dp_resource

compile, run, and use P4i to check resource utilization

## How to run microbench about server with multi Stat threads

Server Project：./server/server_code_kvs_noturn_testlatency_multiStatThread

```bash
make
sudo ./build/netlock_server_kvs -l 0-11,24-35
```

## How to run with twitter trace

Client project：./client/client_speedlimit_w_twitter

Probe project：./probe/client_probe_noClientID_w_twitter