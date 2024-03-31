import socket
PORT = 65000
q = []

s_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s_socket.bind(('localhost', PORT))
s_socket.listen(5)

while True:
    try:
        (c_socket, addr) = s_socket.accept()
        is_acceptable = False

        while True:
            data = c_socket.recv(1024)
            if is_acceptable:
                if data == b"Q\n":
                    is_acceptable = False
                    print("Q")
                else:
                    q.append(data)
                    print(data.decode(), end="")
            else:
                if data == b"SEND\n":
                    is_acceptable = True
                    print("SEND")
                elif data == b"RECV\n":
                    print("RECV")
                    for msg in q:
                        c_socket.sendall(msg)
                    c_socket.sendall(b"LAST_MSG\n")
                    q = []
                    break
        
        c_socket.close()
    except:
        print("error occured")
        c_socket.close()
        s_socket.close()
        exit()