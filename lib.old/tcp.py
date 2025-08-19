import socket

HOST = "127.0.0.1"
PORT = 46001

while True:
    with socket.create_connection((HOST, PORT), timeout=5) as sock:
        sock.recv(6)
        sock.sendall((input("> ") + "\n").encode("utf-8"))
        #sock.recv(1)
        try:
            print(sock.recv(1000).decode("utf-8"))
            """
            buf = sock.recv(1000)
            print(buf)
            buf = buf.split(b'\x00', 1)[0].rstrip(b'\r\n')

            # CESU-8(Modified UTF-8) → Python 文字列
            txt = buf.decode('utf-8', 'surrogatepass')               # サロゲートを保持して一旦デコード
            txt = txt.encode('utf-16', 'surrogatepass').decode('utf-16')  # 正規の Unicode に再デコード
            print(txt)
            buf = sock.recv(1000)
            """
        except:
            pass
        try:
            sock.close()
        except:
            pass