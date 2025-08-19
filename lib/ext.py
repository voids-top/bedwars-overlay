import socket

def send(cmd):
    try:
        with socket.create_connection(("127.0.0.1", 46001), timeout=1) as sock:
            sock.recv(6)
            sock.sendall((cmd + "\n").encode("utf-8"))
            try:
                return sock.recv(1000).decode("utf-8").strip()
            except:
                pass
            try:
                sock.close()
            except:
                pass
    except:
        pass

if __name__ == "__main__":
    print(send("id"))