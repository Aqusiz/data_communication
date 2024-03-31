import socket
PORT = 65000
count = 0
is_sending = True

c_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
c_socket.connect(("localhost", PORT))
c_socket.send(b"SEND\n")

while True:
    msg = input()
    if msg == "Q":
        is_sending = False
        c_socket.send(b"Q\n")
        continue

    if is_sending:
        count += 1
        c_socket.send((msg + "\n").encode())
    else:
        if msg == "SEND":
            is_sending = True
            c_socket.send(b"SEND\n")
        elif msg == "RECV":
            c_socket.send(b"RECV\n")
            while True:
                data = c_socket.recv(1024)
                if not data:
                    break
                print(data.decode(), end="")
            break