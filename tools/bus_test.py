import socket
import time
import os

HOST = "127.0.0.1"
PORT = 46012
LOG_PATH = os.path.join("build", "bus_out.txt")


def recv_line(sock):
    data = bytearray()
    while True:
        chunk = sock.recv(1)
        if not chunk:
            return None if not data else data.decode("utf-8", "replace")
        if chunk == b"\n":
            break
        data.extend(chunk)
    if data.endswith(b"\r"):
        data = data[:-1]
    return data.decode("utf-8", "replace")


def recv_until_nul(sock):
    buf = bytearray()
    while True:
        chunk = sock.recv(4096)
        if not chunk:
            return None if not buf else buf.decode("utf-8", "replace")
        pos = chunk.find(b"\0")
        if pos != -1:
            buf.extend(chunk[:pos])
            break
        buf.extend(chunk)
    return buf.decode("utf-8", "replace")


def send_line(sock, text: str):
    if isinstance(text, str):
        data = text.encode("utf-8")
    else:
        data = text
    sock.sendall(data + b"\n")


def log(msg: str):
    print(msg, flush=True)
    try:
        os.makedirs(os.path.dirname(LOG_PATH), exist_ok=True)
        with open(LOG_PATH, "a", encoding="utf-8") as f:
            f.write(msg + "\n")
    except Exception as e:
        # best-effort logging
        pass


def main():
    # reset file
    try:
        os.makedirs(os.path.dirname(LOG_PATH), exist_ok=True)
        with open(LOG_PATH, "w", encoding="utf-8") as f:
            f.write("[srv] starting bus_test\n")
    except Exception:
        pass
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        try:
            s.bind((HOST, PORT))
        except OSError as e:
            log(f"[srv] bind failed on {HOST}:{PORT}: {e}")
            log("Another server may already be running (overlay.py).")
            return
        s.listen(1)
        log(f"[srv] listening on {HOST}:{PORT}")
        conn, addr = s.accept()
        log(f"[srv] connection from {addr}")
        hello = recv_line(conn)
        log(f"[srv] hello: {hello}")
        # Send test commands
        send_line(conn, "chat [overlay] bus_test hello")
        resp = recv_until_nul(conn)
        log(f"[srv] chat resp: {resp}")
        time.sleep(0.2)
        send_line(conn, "id")
        resp = recv_until_nul(conn)
        log(f"[srv] id resp: {resp}")
        time.sleep(0.2)
        send_line(conn, "tab")
        resp = recv_until_nul(conn)
        log(f"[srv] tab resp:\n{resp}")


if __name__ == "__main__":
    main()
