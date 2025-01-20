import os
import subprocess
import threading
import inspect
import socket
import time


dpdkCtrl_program = "sudo ./build/netlock_ctrl"
dpdkCtrl_options = "-l 0-1 -- "
dpdkCtrl_init_latency = 20
dpdkCtrl_stop_latency = 5

process = None
# time.sleep(dpdkCtrl_init_latency)

tcp_server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
tcp_server_socket.bind(("192.168.189.12", 2333))
# tcp_server_socket.listen(128)
# client_socket, client_addr = tcp_server_socket.accept()
tcp_server_socket.listen(5)

while True:
    print(
        inspect.cleandoc("""
        Waiting for connection...
        """)
    )

    client_socket, client_addr = tcp_server_socket.accept()

    print(
        inspect.cleandoc("""
        Build connection with {}
        """)
        .format(
            client_addr,
        )
    )

    while True:
        print(
            inspect.cleandoc("""
            Waiting for command from {}...
            """)
            .format(
                client_addr,
            )
        )

        recv_data = client_socket.recv(1024).decode("utf-8")

        print(
            inspect.cleandoc(
            """
            Receive command "{}"
            """)
            .format(
                recv_data,
            )
        )

        cmd = recv_data.split()

        if recv_data == '':
            # process.kill()
            # process.terminate()
            client_socket.close()
            
            print(
                inspect.cleandoc("""
                ============================================================
                Test finished
                Restarting...
                ============================================================
                """)
            )

            # time.sleep(dpdkCtrl_stop_latency)
            # process = subprocess.Popen([dpdkCtrl_program, *dpdkCtrl_options.split()])
            # time.sleep(dpdkCtrl_init_latency)

            break
        elif cmd[0] == "restart":
            if process is not None:
                process.kill()
            # process.terminate()

            print(
                inspect.cleandoc("""
                Restarting...
                ============================================================
                """)
            )

            time.sleep(dpdkCtrl_stop_latency)
            # process = subprocess.Popen([dpdkCtrl_program, (*dpdkCtrl_options).split(), *cmd[1:]])
            print(" ".join([dpdkCtrl_program] + dpdkCtrl_options.split() + cmd[1:]))
            process = subprocess.Popen(" ".join([dpdkCtrl_program] + dpdkCtrl_options.split() + cmd[1:]))
            time.sleep(dpdkCtrl_init_latency)

            client_socket.send("done".encode("utf-8"))

tcp_server_socket.close()
