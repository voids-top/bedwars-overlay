import socket, subprocess, psutil, ctypes, threading
from ctypes import wintypes

HOST = "127.0.0.1"
PORT = 46001
sessions: dict[int, socket.socket] = {}
sessions_lock = threading.Lock()
current_pid: int | None = None


user32 = ctypes.windll.user32
EnumWindows = user32.EnumWindows
EnumWindowsProc = ctypes.WINFUNCTYPE(ctypes.c_bool, wintypes.HWND, wintypes.LPARAM)
GetWindowTextW = user32.GetWindowTextW
GetWindowTextLengthW = user32.GetWindowTextLengthW
GetWindowThreadProcessId = user32.GetWindowThreadProcessId
IsWindowVisible = user32.IsWindowVisible

def get_window_titles_by_pid(target_pid: int):
    titles = []

    def callback(hwnd, lParam):
        # 非表示ウィンドウはスキップ
        if not IsWindowVisible(hwnd):
            return True

        pid = wintypes.DWORD()
        GetWindowThreadProcessId(hwnd, ctypes.byref(pid))

        if pid.value == target_pid:
            length = GetWindowTextLengthW(hwnd)
            if length > 0:
                buffer = ctypes.create_unicode_buffer(length + 1)
                GetWindowTextW(hwnd, buffer, length + 1)
                title = buffer.value
                if title:
                    titles.append(title)
        return True  # 列挙を続行

    EnumWindows(EnumWindowsProc(callback), 0)
    return titles

def get_pids_by_name(name: str):
    """名前が name のプロセスの PID 一覧を返す（例: 'notepad.exe'）"""
    pids = []
    for proc in psutil.process_iter(['name', 'pid']):
        try:
            if proc.info['name'] and proc.info['name'].lower() == name.lower():
                pids.append(proc.info['pid'])
        except (psutil.NoSuchProcess, psutil.AccessDenied):
            pass
    return pids

def accept_loop(self):
    global sessions

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind((HOST, PORT))
        s.listen(5)
        print(f"[srv] listening on {HOST}:{PORT}")

        while True:
            conn, addr = s.accept()
            #print("[srv] incoming connection from", addr)

            # hello <pid> を 1 行読む
            hello = recv_line(conn)
            if hello is None:
                print("[srv] no hello, closing")
                conn.close()
                continue

            #print("[srv] hello line:", repr(hello))

            parts = hello.split()
            pid = None
            if len(parts) >= 2 and parts[0].lower() == "hello":
                try:
                    pid = int(parts[1])
                except ValueError:
                    pass

            if pid is None:
                print("[srv] could not parse pid, closing")
                conn.close()
                continue

            with sessions_lock:
                # 同じ pid が既にいたら閉じて差し替え
                old = sessions.get(pid)
                if old is not None:
                    print(f"[srv] replacing existing session pid={pid}")
                    try:
                        old.close()
                    except OSError:
                        pass
                sessions[pid] = conn

            print(f"[srv] registered session pid={pid}")


def recv_line(sock) -> str | None:
    """改行まで 1 行読む（hello 用）"""
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

def injected_pids():
    return list(set(sessions.keys()))

def recv_until_nul(sock) -> str | None:
    """NUL 終端までを 1 メッセージとして読む"""
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

def inject(exe_path, pid, dll_path):
    return subprocess.run(f"{exe_path} {pid} {dll_path}", shell=True, stdout=subprocess.PIPE).stdout.decode()

def send_line(sock, text: str):
    if isinstance(text, str):
        data = text.encode("utf-8")
    else:
        data = text
    sock.sendall(data + b"\n")

def send(text:str):
    if len(sessions) == 1:
        for a,s in list(sessions.items()):
            try:
                send_line(s, text)
            except ConnectionResetError:
                try:
                    del sessions[a]
                except:
                    pass

def recv():
    if len(sessions) == 1:
        for a,s in list(sessions.items()):
            try:
                return recv_until_nul(s)
            except ConnectionResetError:
                try:
                    del sessions[a]
                except:
                    pass

if __name__ == "__main__":
    print(send("id"))
    #pids = get_pids_by_name("javaw.exe")
    #print(get_window_titles_by_pid(pids[0]))
