# from asyncio.windows_events import None
import os
import subprocess
# from scapy.all import Ether, IP, UDP, sendp, sniff
import threading
import socket
import time
# from ping3 import ping

print("start!")
tcp_switch_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
switch_ip = "192.168.189.34"
switch_port = 2333
tcp_switch_socket.connect((switch_ip, switch_port))
print("connect!")
cmd = "restart -b 0 -e 32768"
# cmd = "restart -d"
tcp_switch_socket.send(cmd.encode("utf-8"))
recv_data = tcp_switch_socket.recv(1024)
print("send cmd!")
while(recv_data.decode("utf-8") != "done"):
    recv_data = tcp_switch_socket.recv(1024)
print("switch : %s" % (recv_data.decode("utf-8")))
tcp_switch_socket.close()
