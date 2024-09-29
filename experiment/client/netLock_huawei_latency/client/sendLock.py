from scapy.all import Ether, IP, UDP, sendp
from time import sleep

EXCLU_LOCK = b"\x01"
SHARE_LOCK = b"\x02"

GRANT_LOCK      = b"\x01"
RELEA_LOCK      = b"\x02"
LOCK_GRANT_SUCC = b"\x03"
LOCK_GRANT_FAIL = b"\x04"
LOCK_RELEA_SUCC = b"\x05"
LOCK_RELEA_FAIL = b"\x06"

def gen_pkt(op_type, lock_mode, client_id, lock_id, txn_id, timestamp):
    payload = op_type + lock_mode + client_id + lock_id + txn_id + timestamp + b"\x00\x00" # + b"\x00\x00\x00\x00\x00\x00\x00\x00"
    p = Ether() / IP(dst = "10.0.0.7") / UDP(sport = 1111, dport = 5001) / payload
    sendp(p, iface = "enp65s0")

txn_id = b"\x00\x00\x00\x00"
timestamp = b"\x00\x00\x00\x00\x00\x00\x00\x00"

def g():
    gen_pkt(GRANT_LOCK, EXCLU_LOCK, client_id = b"\x00\x01", lock_id = b"\x00\x00\x00\x01", txn_id = txn_id, timestamp = timestamp)

def r():
    gen_pkt(RELEA_LOCK, EXCLU_LOCK, client_id = b"\x00\x01", lock_id = b"\x00\x00\x00\x01", txn_id = txn_id, timestamp = timestamp)

# for i in range(0, 100):
#     gen_pkt(magic_num, GRANT_LOCK, b"\x00\x00\x00\x00", EXCLU_LOCK, txn_id, b"\x01", timestamp)
#     gen_pkt(magic_num, RELEA_LOCK, b"\x00\x00\x00\x00", EXCLU_LOCK, txn_id, b"\x01", timestamp)

gen_pkt(GRANT_LOCK, EXCLU_LOCK, client_id = b"\x00\x01", lock_id = b"\x00\x00\x00\x01", txn_id = txn_id, timestamp = timestamp)
gen_pkt(GRANT_LOCK, SHARE_LOCK, client_id = b"\x00\x02", lock_id = b"\x00\x00\x00\x01", txn_id = txn_id, timestamp = timestamp)
gen_pkt(GRANT_LOCK, SHARE_LOCK, client_id = b"\x00\x03", lock_id = b"\x00\x00\x00\x01", txn_id = txn_id, timestamp = timestamp)
gen_pkt(GRANT_LOCK, SHARE_LOCK, client_id = b"\x00\x04", lock_id = b"\x00\x00\x00\x01", txn_id = txn_id, timestamp = timestamp)
gen_pkt(GRANT_LOCK, EXCLU_LOCK, client_id = b"\x00\x05", lock_id = b"\x00\x00\x00\x01", txn_id = txn_id, timestamp = timestamp)
'''
[E S S S E] 
'''

# gen_pkt(RELEA_LOCK, EXCLU_LOCK, client_id = b"\x00\x01", lock_id = b"\x00\x00\x00\x01", txn_id = txn_id, timestamp = timestamp)

# gen_pkt(GRANT_LOCK, EXCLU_LOCK, client_id = b"\x00\x01", lock_id = b"\x00\x06\xa9\xa1", txn_id = txn_id, timestamp = timestamp)
# gen_pkt(GRANT_LOCK, EXCLU_LOCK, client_id = b"\x00\x01", lock_id = b"\x00\x01\x48\x27", txn_id = txn_id, timestamp = timestamp)
# gen_pkt(GRANT_LOCK, EXCLU_LOCK, client_id = b"\x00\x01", lock_id = b"\x00\x00\x96\x07", txn_id = txn_id, timestamp = timestamp)
# gen_pkt(GRANT_LOCK, EXCLU_LOCK, client_id = b"\x00\x01", lock_id = b"\x00\x00\x1c\xc7", txn_id = txn_id, timestamp = timestamp)
# gen_pkt(GRANT_LOCK, EXCLU_LOCK, client_id = b"\x00\x01", lock_id = b"\x00\x02\x63\xe9", txn_id = txn_id, timestamp = timestamp)
# gen_pkt(GRANT_LOCK, EXCLU_LOCK, client_id = b"\x00\x01", lock_id = b"\x00\x55\x3b\x48", txn_id = txn_id, timestamp = timestamp)
# gen_pkt(GRANT_LOCK, EXCLU_LOCK, client_id = b"\x00\x01", lock_id = b"\x00\x00\xc0\x50", txn_id = txn_id, timestamp = timestamp)
# gen_pkt(GRANT_LOCK, EXCLU_LOCK, client_id = b"\x00\x01", lock_id = b"\x00\x09\x52\xc3", txn_id = txn_id, timestamp = timestamp)
# '''
# [S S S E]
# recv 4 pkt
# '''

# gen_pkt(RELEA_LOCK, SHARE_LOCK, client_id = b"\x00\x02", lock_id = b"\x00\x00\x00\x01", txn_id = txn_id, timestamp = timestamp)
# gen_pkt(RELEA_LOCK, SHARE_LOCK, client_id = b"\x00\x03", lock_id = b"\x00\x00\x00\x01", txn_id = txn_id, timestamp = timestamp)
# gen_pkt(RELEA_LOCK, SHARE_LOCK, client_id = b"\x00\x04", lock_id = b"\x00\x00\x00\x01", txn_id = txn_id, timestamp = timestamp)
# '''
# [E]
# recv 2 pkt
# '''

# gen_pkt(GRANT_LOCK, EXCLU_LOCK, client_id = b"\x00\x02", lock_id = b"\x00\x00\x00\x01", txn_id = txn_id, timestamp = timestamp)
# '''
# [E E]
# '''

# gen_pkt(RELEA_LOCK, EXCLU_LOCK, client_id = b"\x00\x05", lock_id = b"\x00\x00\x00\x01", txn_id = txn_id, timestamp = timestamp)
# '''
# [E]
# recv 2 pkt
# '''

# gen_pkt(GRANT_LOCK, SHARE_LOCK, client_id = b"\x00\x03", lock_id = b"\x00\x00\x00\x01", txn_id = txn_id, timestamp = timestamp)
# gen_pkt(GRANT_LOCK, SHARE_LOCK, client_id = b"\x00\x04", lock_id = b"\x00\x00\x00\x01", txn_id = txn_id, timestamp = timestamp)
# gen_pkt(GRANT_LOCK, SHARE_LOCK, client_id = b"\x00\x05", lock_id = b"\x00\x00\x00\x01", txn_id = txn_id, timestamp = timestamp)
# '''
# [E S S S]
# '''

# gen_pkt(RELEA_LOCK, EXCLU_LOCK, client_id = b"\x00\x02", lock_id = b"\x00\x00\x00\x01", txn_id = txn_id, timestamp = timestamp)
# '''
# [S S S]
# recv 4 pkt
# '''

# gen_pkt(GRANT_LOCK, SHARE_LOCK, client_id = b"\x00\x01", lock_id = b"\x00\x00\x00\x01", txn_id = txn_id, timestamp = timestamp)
# '''
# [S S S S]
# grant succ
# '''



# gen_pkt(GRANT_LOCK, SHARE_LOCK, client_id = b"\x00\x01", lock_id = b"\x00\x00\x00\x01", txn_id = txn_id, timestamp = timestamp)
# gen_pkt(GRANT_LOCK, SHARE_LOCK, client_id = b"\x00\x02", lock_id = b"\x00\x00\x00\x01", txn_id = txn_id, timestamp = timestamp)
# gen_pkt(GRANT_LOCK, SHARE_LOCK, client_id = b"\x00\x03", lock_id = b"\x00\x00\x00\x01", txn_id = txn_id, timestamp = timestamp)
# gen_pkt(GRANT_LOCK, SHARE_LOCK, client_id = b"\x00\x04", lock_id = b"\x00\x00\x00\x01", txn_id = txn_id, timestamp = timestamp)
# gen_pkt(GRANT_LOCK, SHARE_LOCK, client_id = b"\x00\x05", lock_id = b"\x00\x00\x00\x01", txn_id = txn_id, timestamp = timestamp)
# gen_pkt(GRANT_LOCK, SHARE_LOCK, client_id = b"\x00\x06", lock_id = b"\x00\x00\x00\x01", txn_id = txn_id, timestamp = timestamp)
# gen_pkt(GRANT_LOCK, SHARE_LOCK, client_id = b"\x00\x07", lock_id = b"\x00\x00\x00\x01", txn_id = txn_id, timestamp = timestamp)
# gen_pkt(GRANT_LOCK, SHARE_LOCK, client_id = b"\x00\x08", lock_id = b"\x00\x00\x00\x01", txn_id = txn_id, timestamp = timestamp)

# gen_pkt(GRANT_LOCK, SHARE_LOCK, client_id = b"\x00\x09", lock_id = b"\x00\x00\x00\x01", txn_id = txn_id, timestamp = timestamp)

# gen_pkt(RELEA_LOCK, SHARE_LOCK, client_id = b"\x00\x01", lock_id = b"\x00\x00\x00\x01", txn_id = txn_id, timestamp = timestamp)
# gen_pkt(RELEA_LOCK, SHARE_LOCK, client_id = b"\x00\x02", lock_id = b"\x00\x00\x00\x01", txn_id = txn_id, timestamp = timestamp)
# gen_pkt(RELEA_LOCK, SHARE_LOCK, client_id = b"\x00\x03", lock_id = b"\x00\x00\x00\x01", txn_id = txn_id, timestamp = timestamp)
# gen_pkt(RELEA_LOCK, SHARE_LOCK, client_id = b"\x00\x04", lock_id = b"\x00\x00\x00\x01", txn_id = txn_id, timestamp = timestamp)
# gen_pkt(RELEA_LOCK, SHARE_LOCK, client_id = b"\x00\x05", lock_id = b"\x00\x00\x00\x01", txn_id = txn_id, timestamp = timestamp)
# gen_pkt(RELEA_LOCK, SHARE_LOCK, client_id = b"\x00\x06", lock_id = b"\x00\x00\x00\x01", txn_id = txn_id, timestamp = timestamp)
# gen_pkt(RELEA_LOCK, SHARE_LOCK, client_id = b"\x00\x07", lock_id = b"\x00\x00\x00\x01", txn_id = txn_id, timestamp = timestamp)
# gen_pkt(RELEA_LOCK, SHARE_LOCK, client_id = b"\x00\x08", lock_id = b"\x00\x00\x00\x01", txn_id = txn_id, timestamp = timestamp)


# gen_pkt(RELEA_LOCK, SHARE_LOCK, client_id = b"\x00\x08", lock_id = b"\x10\x00\x00\x01", txn_id = txn_id, timestamp = timestamp)



