mkdir -p ./res_zipf
mkdir -p ./res_zipf_hc
g++ -o genTxn_zipf.run genTxn_zipf.cpp
./genTxn_zipf.run


# scp -r "/home/tt/dpdk-21.05/examples/netlock_huawei/client_code_v9_zipf_mul/trace/res" tt@192.168.189.10:"/home/tt/dpdk-21.05/examples/netlock_huawei/client_code_v9_zipf_mul/trace/"
# scp -r "/home/tt/dpdk-21.05/examples/netlock_huawei/client_code_v9_zipf_mul/trace/res" tt@192.168.189.11:"/home/tt/dpdk-21.05/examples/netlock_huawei/client_code_v9_zipf_mul/trace/"
# scp -r "/home/tt/dpdk-21.05/examples/netlock_huawei/client_code_v9_zipf_mul/trace/res" tt@192.168.189.12:"/home/tt/dpdk-21.05/examples/netlock_huawei/client_code_v9_zipf_mul/trace/"
# scp -r "/home/tt/dpdk-21.05/examples/netlock_huawei/client_code_v9_zipf_mul/trace/res" tt@192.168.189.13:"/home/tt/dpdk-21.05/examples/netlock_huawei/client_code_v9_zipf_mul/trace/"
# scp -r "/home/tt/dpdk-21.05/examples/netlock_huawei/client_code_v9_zipf_mul/trace/res" tt@192.168.189.14:"/home/tt/dpdk-21.05/examples/netlock_huawei/client_code_v9_zipf_mul/trace/"