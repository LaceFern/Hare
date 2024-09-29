### 如何运行Hare功能代码 (已跑通, 用户发出25Mpps，系统响应22Mpps? 发现是数据面开了限速代码，目前已取消，如果流量大可以开启)

## Switch
交换机SDE版本: 9.6.0 (当前交换机上9.6.0不存在$SDE_INSTALL/bin/bf_kpkt_mod_load $SDE_INSTALL，可换用9.9.0)

DPDK版本：21.05

KVS工程目录：/home/zxy/nfs/Hare/p4/p4_prj/P4-KVS-lyt/P4DPDK-KVS-noturn-latency

Lock工程目录：/home/zxy/nfs/Hare/p4/p4_prj/P4-Lock-lyt/P4-Lock-testlatency

# Step 1. 检查交换机版本

```bash
sudo update-alternatives --config bf-sde
```

# Step 2. 设置环境变量（可以添加在.bashrc里面）

```bash
# SDE
export SDE=/root/Software/bf-sde
export SDE_INSTALL=$SDE/install
export PATH=$PATH:$SDE_INSTALL/bin
# export LD_LIBRARY_PATH=/usr/local/lib:$SDE_INSTALL/lib
```

# Step 3. 配置启用PISA架构控制面

```bash
sudo -i
$SDE_INSTALL/bin/bf_kpkt_mod_load $SDE_INSTALL #加载pisa驱动
ifconfig enp6s0 up #开启虚拟网卡，用于控制面与数据面交互
```

# Step 4. 配置启用dpdk-21.05

```bash
sudo -i
modprobe uio_pci_generic #启动内核模块
cd /home/zxy/dpdk-21.05/usertools/ #py文件位置，但实际上不进入也行
dpdk-devbind.py --status #检查状态，若自信可跳过
dpdk-devbind.py --bind=uio_pci_generic 04:00.0 #将网卡绑定在uio内核模块上
ifconfig enp4s0f0 up #物理网卡，若该网卡未启动（表现：查不到该网卡状态），则启动网卡
mount -t hugetlbfs pagesize=1GB /mnt/huge #挂载大页
```

# Step 5. 修改配置参数

注意控制面controller/config.h中的以下宏需要与数据面、控制节点和后端节点一致

```c
#define SLOT_SIZE 32768
#define TOPK 600
#define HOTOBJ_IN_ONEPKT 20
```

注意数据面p4src/config.p4中的以下宏需要与控制面、控制节点和后端节点一致

```c
#define SLOT_SIZE 32768
#define CPU_CONTROLLER  24 //控制节点所连端口,当前值对应amax3
```

# Step 6. 编译&控制面运行

进入工程所在位置并执行以下脚本

```bash
mkdir build && cd build
cmake ../
cd p4src/p4build && make && make install && cd ../../
cd controller && make && cd ../
#
cd controller
./hello_bfrt -l 0
```
# Debug

使用 $SDE/run_bfshell.sh 进入bfshell，输入 ucli ，然后输入 pm show

************************************************************************************************
## Server

KVS工程目录：/home/zxy/nfs/Hare/server/server_code_kvs_noturn_testlatency/KVS_dpdk_server_code_amax7
（backup：server_code_kvs_noturn）

Lock工程目录：/home/zxy/nfs/Hare/server/server_code_lock_noturn_testlatency/ver_UPD

# Step 1. 修改配置参数

注意控制面cache_upd_ctrl.h中的以下宏需要与交换机、控制节点一致，并且符合实际情况

```c
#define IP_CTRL               "10.0.0.3"
#define MAC_CTRL               {0x98, 0x03, 0x9b, 0xc7, 0xc8, 0x18}

#define MAX_CACHE_SIZE  32768
#define TOPK 600//800//8

#define NUM_BACKENDNODE 1
char* ip_backendNode_arr[NUM_BACKENDNODE] = {
    "10.0.0.4"
};
struct rte_ether_addr mac_backendNode_arr[NUM_BACKENDNODE] = {
    {.addr_bytes = {0x98, 0x03, 0x9b, 0xc7, 0xc0, 0xa8}}
};

//如果是lock工程，还需要注意以下参数
#define NUM_CLIENT 3
char* ip_client_arr[NUM_CLIENT] = {
    "10.0.0.1",
    "10.0.0.2",
    "10.0.0.5"
};
struct rte_ether_addr mac_client_arr[NUM_CLIENT] = {
    {.addr_bytes = {0xb8, 0x59, 0x9f, 0xe9, 0x6b, 0x1c}},
    {.addr_bytes = {0x98, 0x03, 0x9b, 0xca, 0x48, 0x38}},
    {.addr_bytes = {0x04, 0x3f, 0x72, 0xde, 0xba, 0x44}}
};
```

注意util.h中的以下宏需要符合实际情况

```c
#define IP_LOCAL                "10.0.0.4"
#define MAC_LOCAL               {0x98, 0x03, 0x9b, 0xc7, 0xc0, 0xa8}
```

# Step 2. 编译&运行

```bash
make
sudo ./build/netlock_server_kvs -l 0-11,24-35 # 对于KVS工程
# or
make
sudo ./build/netlock_server_lock -l 0-11,24-35 # 对于lock工程
```

************************************************************************************************
## Client

KVS工程目录：/home/zxy/nfs/Hare/client/netCache_huawei/client_speedlimit

Lock工程目录：/home/zxy/nfs/Hare/client/netLock_huawei_latency/client

# Step 1. 修改配置参数

注意util.h符合实际情况

```c
#define IP_LOCAL                "10.0.0.1"
#define MAC_LOCAL               {0xb8, 0x59, 0x9f, 0xe9, 0x6b, 0x1c} //amax 1
#define CLIENT_PORT             1111 // 不同client需要有不同udp port, 分别是1111,9303,17495,25687,33879,42071,50263,58455
#define HC_MASTER               1 // "hc" is short for hotspot change
#define HC_SLAVE_NUM            4
volatile int hc_flag = 0;
char* ip_dst_hc_arr[HC_SLAVE_NUM] = {
    "10.0.0.2",
    "10.0.0.3",
    "10.0.0.5",
    "10.0.0.8"
};
struct rte_ether_addr mac_dst_hc_arr[HC_SLAVE_NUM] = {
    {.addr_bytes = {0x98, 0x03, 0x9b, 0xca, 0x48, 0x38}},
    {.addr_bytes = {0x98, 0x03, 0x9b, 0xc7, 0xc8, 0x18}},
    {.addr_bytes = {0x04, 0x3f, 0x72, 0xde, 0xba, 0x44}},
    {.addr_bytes = {0x0c, 0x42, 0xa1, 0x2b, 0x0d, 0x70}}
};

#define BN_NUM            1
char* ip_dst_arr[BN_NUM] = {
    "10.0.0.4"
};
struct rte_ether_addr mac_dst_arr[BN_NUM] = {
    {.addr_bytes = {0x98, 0x03, 0x9b, 0xc7, 0xc0, 0xa8}}
};
```

# Step 2. 编译&运行

无论KVS还是Lock应用，都使用如下指令运行

```bash
make
sudo ./build/netlock_client -l 0-？？？
```

************************************************************************************************
## control node

KVS工程目录：/home/zxy/nfs/Hare/controller/netlock_huawei/ctrl_code_kvs_noturn_latency

Lock工程目录：/home/zxy/nfs/Hare/controller/netlock_huawei/ctrl_code_lock_noturn_latency

# Step 1. 修改配置参数

注意头文件cache_upd_ctrl.h中的以下宏需要与交换机、后端节点一致，并且符合实际情况

```c
#define IP_CTRL               "10.0.0.3"
#define MAC_CTRL               {0x98, 0x03, 0x9b, 0xc7, 0xc8, 0x18}

#define MAX_CACHE_SIZE  32768
#define TOPK 600//800//1000 //8
#define HOTOBJ_IN_ONEPKT 20//20//20 //50 //20

#define NUM_BACKENDNODE 1
char* ip_backendNode_arr[NUM_BACKENDNODE] = {
    "10.0.0.4"
};
struct rte_ether_addr mac_backendNode_arr[NUM_BACKENDNODE] = {
    {.addr_bytes = {0x98, 0x03, 0x9b, 0xc7, 0xc0, 0xa8}}
};
```

# Step 2. 编译&运行

无论KVS还是Lock应用，都使用如下指令运行

```bash
make
sudo ./build/netlock_ctrl -l 0-2
```

************************************************************************************************
## Probe

无论KVS还是Lock应用，都使用如下工程：/home/zxy/nfs/Hare/probe/client_probe_noClientID

# Step 1. 编译&运行

注意一定要先起client再起probe，注意util.h中的hc_flag需要与client保持一致
```bash
make
sudo ./build/netlock_probe -l 0-1
```

************************************************************************************************
************************************************************************************************
### 如何运行DySO功能代码 (已跑通)

## Switch
KVS工程目录：/home/zxy/nfs/Hare/p4/p4_prj/P4-KVS-lyt/P4-KVS-DySO-k1v1c5-hitrate

# Step 1. 修改配置参数

注意控制面config.h中的以下宏

```c
#define KEY_COLNUM 1
#define KEY_COLNUM_HW 5
#define KEY_ROWNUM 32768//16384//32768
#define VALUE_COLNUM 1
#define VALUE_COLNUM_HW 5
```

# Step 2. 编译&控制面运行

进入工程所在位置并执行以下脚本

```bash
mkdir build && cd build
cmake ../
cd p4src/p4build && make && make install && cd ../../
cd controller && make && cd ../
#
cd controller
./hello_bfrt -l 0
```

************************************************************************************************
## Server（可同Hare，或使用以下工程）

KVS工程目录：/home/zxy/nfs/Hare/server/server_code_kvs_dyso_speedlimit

# Step 1. 修改配置参数

注意util.h中的以下宏需要符合实际情况

```c
#define IP_LOCAL                "10.0.0.4"
#define MAC_LOCAL               {0x98, 0x03, 0x9b, 0xc7, 0xc0, 0xa8}
```

# Step 2. 编译运行（同Hare）

```bash
make
sudo ./build/netlock_server_kvs -l 0-11,24-35
```
************************************************************************************************
## Client（同Hare）

************************************************************************************************
************************************************************************************************
### 如何运行NetCache功能代码 (已跑通)

## Switch

KVS工程目录：/home/zxy/nfs/Hare/p4/p4_prj/P4-KVS-lyt/P4-KVS-cms-hitrate

# Step 1. 修改配置参数

config.p4
```c
#define SLOT_SIZE 32768
```

config.h
```c
#define SLOT_SIZE 32768
```

# Step 2. 编译&控制面运行

进入工程所在位置并执行以下脚本

```bash
mkdir build && cd build
cmake ../
cd p4src/p4build && make && make install && cd ../../
cd controller && make && cd ../
#
cd controller
./hello_bfrt -p P4Lock
```

************************************************************************************************
## Server（可同Hare，或同DySO）

************************************************************************************************
## Client（同Hare）


************************************************************************************************
************************************************************************************************
### 如何运行switch cp api batch 功能代码

microbench 工程目录：/home/zxy/nfs/Hare/p4/p4_prj/P4-cp_api_batch

./hello_bfrt -p P4Lock

注：
当前key size改成80B


************************************************************************************************
************************************************************************************************
### 如何运行switch dp resource 功能代码

microbench 工程目录：/home/zxy/nfs/Hare/p4/p4_prj/P4-dp_resource

编译然后用p4i观察资源占用


************************************************************************************************
************************************************************************************************
### 如何运行backend server multiStatsThread 功能代码

server 工程目录：/home/zxy/nfs/Hare/server/server_code_kvs_noturn_testlatency_multiStatThread

```bash
make
sudo ./build/netlock_server_kvs -l 0-11,24-35
```

无替换，只需client配合即可


************************************************************************************************
************************************************************************************************
### 如何使用twitter数据集做测试
client目录：client_speedlimit_w_twitter

server端使用线速变量wpkts_send_limit_s

注意：对于Hare来说，发包不可加barrier（一下子过大的流量打到server，造成server丢包）

注意：如果观察到client tx > client rx + server tx && server tx < thres，则是由于client收包速率跟不上发包速率，应增加client收包循环数目