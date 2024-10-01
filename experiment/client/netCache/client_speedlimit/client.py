# from asyncio.windows_events import None
import os
import subprocess
# from scapy.all import Ether, IP, UDP, sendp, sniff
import threading
import socket
import time
# from ping3 import ping

TPYE_START  = b"\x01"
TPYE_END    = b"\x02"

# each has unique one
if_master           = 1 # corresponding with master_client_ip
local_client_id     = b"\x00"
local_client_ip     = 7
local_iface         = "eno1"

# only for master
client_slave_ip_list = [10,13,14]
# client_slave_ip_list = [13, 14]
# client_slave_ip_list = [8, 9]
# client_slave_ip_list = [8,9,10]

# client_slave_id_list = [1,2,3]
# client_slave_ip_list = [8, 9, 10, 11, 12]
# client_slave_id_list = [1, 2, 3,  4,  5]
# client_slave_ip_list = [8, 9, 10, 12, 13, 14]
# client_slave_id_list = [1, 2, 3,  5,  6,  7]

# for each
# client
client_master_ip    = 7 # corresponding with if_master
n_list = [9000000]
a_list = [99]#[99, 80, 50] # 90 = 0.9 * 100
p_list = [99] # 90 = 0.9 * 100
k_list = [1]
t_list = [8388608]
s_list = [1]
d_list = [500000000]#5000000
h_list = [512]#,2048,8192]#[256, 32768]
z_list = [0]#[0, 1] #0:zipf, use a_list; 1:rect, use p_list
# fpga
w_list = [100]#, 50, 100]#[1, 10, 100] #wait time for stats
e_list = [64]#, 512]

# n_list = [32768]
# a_list = [99, 95, 90, 80, 70, 60, 50, 40, 30, 20, 10, 0]
# k_list = [1, 2, 4, 8]
# t_list = [16384]
# s_list = [1]
# d_list = [1000, 5000, 10000, 50000, 100000, 500000, 1000000, 5000000]

# def sndPkt(pkt_type, dst_ip):
#     payload = pkt_type + local_client_id #str
#     p = Ether() / IP(dst = dst_ip) / UDP(sport = 2333, dport = 2333) / payload
#     sendp(p, iface = local_iface)

# def rcvPkt(expected_pkt_num):
#     pkts = sniff(filter="port 2333", prn=lambda x:x.summary(), count = expected_pkt_num)
#     for pkt in pkts:
#         payload = pkt.payload.payload.payload
#         pkt_type = payload[0]
#         client_id = payload[1]
#     return pkt_type, client_id

# connection start
tcp_switch_socket = None
tcp_fpga_socket = None
tcp_client_socket_list = []
tcp_server_socket = None

client_socket = None
client_addr = None
if(if_master == 1):
    # tcp_switch_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # switch_ip = "192.168.189.34"
    # switch_port = 2333
    # tcp_switch_socket.connect((switch_ip, switch_port))

    # tcp_fpga_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # fpga_ip = "192.168.189.12" #"192.168.189.27"
    # fpga_port = 2333
    # tcp_fpga_socket.connect((fpga_ip, fpga_port))

    for client_slave_ip in client_slave_ip_list:
        tcp_client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_ip = "192.168.189." + str(client_slave_ip)
        server_port = 2333
        tcp_client_socket.connect((server_ip, server_port))
        tcp_client_socket_list.append(tcp_client_socket)
        print (server_ip)
else:
    tcp_server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    tcp_server_socket.bind(("192.168.189." + str(local_client_ip), 2333))
    tcp_server_socket.listen(128)
    client_socket, client_addr = tcp_server_socket.accept()

p=subprocess.Popen("mkdir -p ./testRes_normal",shell=True)
return_code=p.wait()  # waiting for end
p=subprocess.Popen("mkdir -p ./log",shell=True)
return_code=p.wait()  # waiting for end
p=subprocess.Popen("mkdir -p ./stats",shell=True)
return_code=p.wait()  # waiting for end

round_count = 0

# if if_master:
#     # restart fpga
#     tcp_fpga_socket.send("clear".encode("utf-8"))
#     recv_data = tcp_fpga_socket.recv(1024)
#     print("fpga : %s" % (recv_data.decode("utf-8")))
#     while(recv_data.decode("utf-8") != "done"):
#         recv_data = tcp_fpga_socket.recv(1024)
#     print("fpga : %s" % (recv_data.decode("utf-8")))

# netlock
for n in n_list:
    for z in z_list:
        if z == 0:
            a_wideSense_list = a_list
        else:
            a_wideSense_list = p_list
        for a in a_wideSense_list:
            for k in k_list:
                for t in t_list:
                    for s in s_list:
                        for d in d_list:
                            for h in h_list:
                                for w in w_list:
                                    for e in e_list:

                                        print ("-----------round " + str(round_count) + " start-----------")
                                        # start
                                        if if_master:
                                            # restart switch
                                            # tcp_switch_socket.send("restart".encode("utf-8"))
                                            # cmd = "restart -b 0 -e 32768 -d"
                                            # cmd = "restart"
                                            # tcp_switch_socket.send(cmd.encode("utf-8"))
                                            # recv_data = tcp_switch_socket.recv(1024)
                                            # while(recv_data.decode("utf-8") != "done"):
                                            #     recv_data = tcp_switch_socket.recv(1024)
                                            # print("switch : %s" % (recv_data.decode("utf-8")))
                                            # time.sleep(15)

                                            # # restart fpga
                                            # cmd = "restart -w " + str(w) + " -e " + str(e) 
                                            # tcp_fpga_socket.send(cmd.encode("utf-8"))
                                            # recv_data = tcp_fpga_socket.recv(1024)
                                            # while(recv_data.decode("utf-8") != "done"):
                                            #     recv_data = tcp_fpga_socket.recv(1024)
                                            # print("fpga : %s" % (recv_data.decode("utf-8")))
                                            # time.sleep(15)

                                            # cmd = "restart -s " + str(e)
                                            # tcp_fpga_socket.send(cmd.encode("utf-8"))
                                            # recv_data = tcp_fpga_socket.recv(1024)
                                            # while(recv_data.decode("utf-8") != "done"):
                                            #     recv_data = tcp_fpga_socket.recv(1024)
                                            # print("fpga : %s" % (recv_data.decode("utf-8")))
                                            # time.sleep(15)

                                            # send start pkt
                                            for tcp_client_socket in tcp_client_socket_list:
                                                tcp_client_socket.send("start".encode("utf-8"))
                                        else:
                                            # wait start pkt
                                            recv_data = client_socket.recv(1024)
                                            print("%s : %s" % (str(client_addr), recv_data.decode("utf-8")))

                                        print ("-----------ping start-----------")
                                        cmd = "ping 10.0.0.235 -c 5"
                                        p=subprocess.Popen(cmd,shell=True)
                                        return_code=p.wait()  # waiting for end
                                        print ("-----------ping end-----------")

                                        # run
                                        # print (" n:" + str(n) + " a:" + str(a) + " k:" + str(k) + " t:" + str(t) + " s:" + str(s))
                                        # cmd = "sudo ./build/netlock_client -l 0-11,24-30 --" + \
                                        #     " -n" + str(n) + " -a" + str(a) + " -k" + str(k) + " -t" + str(t) + " -s" + str(s) + " -d" + str(d) + \
                                        #     " > ./testRes/" + "n" + str(n) + "_a" + str(a) + "_k" + str(k) + "_t" + str(t) + "_s" + str(s) + "_d" + str(d) + ".txt"
                                        # cmd = "sudo ./build/netlock_client -l 0-11,24-30 --" + \
                                        #     " -n" + str(n) + " -a" + str(a) + " -k" + str(k) + " -t" + str(t) + " -s" + str(s) + " -d" + str(d) + \
                                        #     " > ./testRes/" + "n" + str(n) + "_a" + str(a) + "_k" + str(k) + "_t" + str(t) + "_s" + str(s) + "_d" + str(d) + ".txt"
                                        cmd = "sudo ./build/netlock_client -l 0-11,24-30 --" + \
                                            " -n" + str(n) + " -a" + str(a) + " -k" + str(k) + " -t" + str(t) + " -s" + str(s) + " -d" + str(d) + " -h" + str(h) + " -z" + str(z) + " -w" + str(w) + " -e" + str(e) + \
                                            " > ./testRes_normal/" + "n" + str(n) + "_a" + str(a) + "_k" + str(k) + "_t" + str(t) + "_s" + str(s) + "_d" + str(d) + "_h" + str(h)  + "_z" + str(z) + " _w" + str(w) + " -e" + str(e) + ".txt"
                                        # the content after > is too long! 
                                        # print(cmd)
                                        # os.open(cmd)
                                        p=subprocess.Popen(cmd,shell=True)
                                        return_code=p.wait()  # waiting for end

                                        # end
                                        if if_master:
                                            # wait end pkt
                                            for tcp_client_socket in tcp_client_socket_list:
                                                recv_data = tcp_client_socket.recv(1024)
                                                print("%s : %s" % (str(client_addr), recv_data.decode("utf-8")))
                                        else:
                                            # send end pkt
                                            client_socket.send("end".encode("utf-8"))
                                        print ("-----------round " + str(round_count) + " end------------")
                                        round_count += 1

# connection end
if(if_master == 1):
    for tcp_client_socket in tcp_client_socket_list:
        tcp_client_socket.close()
    tcp_switch_socket.close()
    tcp_fpga_socket.close()
else:
    tcp_server_socket.close()