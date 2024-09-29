from scapy.all import *
# pkt = Ether()/IP(dst = "10.0.0.7", src = "10.0.0.8")/UPD()/"GET /index.html HTTP/1.0 \n\n"
# print(ls())
data = ""
data_type = ""
for i in range(8):
    data += b"\xff\xff\xff\xff"
    data_type += b"\xee"

payload = b"\x00" + b"\x00\x00\x00\x01" + data + data_type
sendp(Ether(src="98:03:9b:ca:40:18", dst="98:03:9b:c7:c8:18", type=0x2335)/payload, inter=1, count=5, iface="enp62s0")



# from scapy.all import *
# from time import sleep

# NC_READ_REQUEST     = b"\x00"
# NC_READ_REPLY       = b"\x01"
# NC_HOT_READ_REQUEST = b"\x02"
# NC_WRITE_REQUEST    = b"\x04"
# NC_WRITE_REPLY      = b"\x05"
# NC_UPDATE_REQUEST   = b"\x08"
# NC_UPDATE_REPLY     = b"\x09"
# NC_DELETE_REQUEST   = b"\x0a"
# NC_DELETE_REPLY     = b"\x0b"

# def gen_pkt(op, key, value):
#     payload = op + key + value
#     p = Ether() / IP(src = "192.168.1.1", dst = "192.168.1.2") / UDP(dport = 8888) / payload
#     sendp(p, iface = "enp5s0")

# key = b"\x00\x00\x00\x0f"
# value = b"\x00\x00\x00\x01"
# value1 = b"\x00\x00\x00\x02"
# value2 = b"\x00\x00\x00\x03"
# padding = b"\x00\x00\x00\x00"

# gen_pkt(NC_HOT_READ_REQUEST, key, value)
# sleep(1)
# gen_pkt(NC_READ_REQUEST, key, padding)
# sleep(1)
# gen_pkt(NC_WRITE_REQUEST, key, value1)
# sleep(1)
# gen_pkt(NC_WRITE_REPLY, key, value1)
# sleep(1)
# gen_pkt(NC_READ_REQUEST, key, padding)
# sleep(1)
# gen_pkt(NC_UPDATE_REQUEST, key, value2)
# sleep(1)
# gen_pkt(NC_UPDATE_REPLY, key, value2)
# sleep(1)
# gen_pkt(NC_READ_REQUEST, key, padding)
# sleep(1)
# # gen_pkt(NC_DELETE_REQUEST, key, padding)
# # sleep(1)
# gen_pkt(NC_DELETE_REPLY, key, padding)
# sleep(1)
# gen_pkt(NC_READ_REQUEST, key, padding)
# sleep(1)
