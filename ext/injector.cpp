// injector_pe_x64_v2.cpp  (x64 専用 / UTF-8)
// 使い方: injector_pe_x64_v2.exe <pid> "<絶対パス>\\agents_null_vN.dll"
// ビルド: (x64 Native Tools で)  cl /EHsc /O2 /utf-8 injector_pe_x64_v2.cpp
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <tlhelp32.h>
#include <winnt.h>
#include <stdio.h>
#include <wchar.h>
#include <string>
#include <vector>

static void die(const wchar_t* msg){
    fwprintf(stderr,L"[inj] %s (err=%lu)\n",msg,GetLastError());
    ExitProcess(1);
}
static_assert(sizeof(void*)==8, "Build as x64 (open 'x64 Native Tools Command Prompt')");

static std::wstring FullPathOf(const wchar_t* p){
    wchar_t buf[MAX_PATH]; DWORD n=GetFullPathNameW(p,MAX_PATH,buf,nullptr);
    if(!n||n>=MAX_PATH) die(L"GetFullPathNameW failed");
    return std::wstring(buf);
}
static std::wstring BaseNameOf(const std::wstring& path){
    size_t pos = path.find_last_of(L"\\/");
    return (pos==std::wstring::npos) ? path : path.substr(pos+1);
}

template<typename T>
static bool rpm(HANDLE h, LPCVOID addr, T& out){
    SIZE_T got=0; BOOL ok = ReadProcessMemory(h, addr, &out, sizeof(T), &got);
    return ok && got==sizeof(T);
}
static bool rpm_buf(HANDLE h, LPCVOID addr, void* buf, SIZE_T sz){
    SIZE_T got=0; BOOL ok = ReadProcessMemory(h, addr, buf, sz, &got);
    return ok && got==sz;
}
static bool rpm_cstr(HANDLE h, LPCVOID addr, char* out, size_t max){
    size_t i=0; for(; i+1<max; ++i){
        char c=0; SIZE_T got=0;
        if(!ReadProcessMemory(h,(LPCVOID)((uintptr_t)addr+i),&c,1,&got) || got!=1) return false;
        out[i]=c; if(!c){ out[i]=0; return true; }
    }
    out[max-1]=0; return true;
}

// <--- ここがキモ：ツールヘルプでリモートの HMODULE を取り直す
static HMODULE FindRemoteModuleByName(DWORD pid, const wchar_t* nameOrFullPath){
    std::wstring needleName = BaseNameOf(nameOrFullPath);
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32, pid);
    if (snap==INVALID_HANDLE_VALUE) return nullptr;
    MODULEENTRY32W me{}; me.dwSize = sizeof(me);
    HMODULE found = nullptr;
    if (Module32FirstW(snap, &me)) {
        do{
            const wchar_t* base = wcsrchr(me.szExePath, L'\\'); base = base ? base+1 : me.szExePath;
            if (_wcsicmp(base, needleName.c_str())==0) { found = me.hModule; break; }
        }while(Module32NextW(snap, &me));
    }
    CloseHandle(snap);
    return found;
}

// リモートPEのエクスポートから関数アドレス取得
static LPTHREAD_START_ROUTINE RemoteGetExportByName(HANDLE h, BYTE* base, const char* name){
    IMAGE_DOS_HEADER dos{};
    if(!rpm(h, base, dos) || dos.e_magic!=IMAGE_DOS_SIGNATURE) { SetLastError(ERROR_INVALID_DATA); return nullptr; }
    IMAGE_NT_HEADERS64 nt{};
    if(!rpm(h, base + dos.e_lfanew, nt) || nt.Signature!=IMAGE_NT_SIGNATURE || nt.OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC){
        SetLastError(ERROR_INVALID_DATA); return nullptr;
    }
    auto dir = nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if(!dir.VirtualAddress || !dir.Size){ SetLastError(ERROR_INVALID_DATA); return nullptr; }

    IMAGE_EXPORT_DIRECTORY exp{};
    BYTE* expBase = base + dir.VirtualAddress;
    if(!rpm(h, expBase, exp)) { SetLastError(ERROR_PARTIAL_COPY); return nullptr; }
    if(!exp.NumberOfNames || !exp.NumberOfFunctions){ SetLastError(ERROR_INVALID_DATA); return nullptr; }

    std::vector<DWORD> names(exp.NumberOfNames);
    std::vector<WORD>  ords(exp.NumberOfNames);
    std::vector<DWORD> funcs(exp.NumberOfFunctions);
    if(!rpm_buf(h, base + exp.AddressOfNames, names.data(), names.size()*sizeof(DWORD))) { SetLastError(ERROR_PARTIAL_COPY); return nullptr; }
    if(!rpm_buf(h, base + exp.AddressOfNameOrdinals, ords.data(), ords.size()*sizeof(WORD))) { SetLastError(ERROR_PARTIAL_COPY); return nullptr; }
    if(!rpm_buf(h, base + exp.AddressOfFunctions, funcs.data(), funcs.size()*sizeof(DWORD))) { SetLastError(ERROR_PARTIAL_COPY); return nullptr; }

    char tmp[256];
    for(DWORD i=0;i<exp.NumberOfNames;i++){
        if(!rpm_cstr(h, base + names[i], tmp, sizeof(tmp))) continue;
        if(lstrcmpiA(tmp, name)==0){
            WORD ord = ords[i];
            if(ord >= funcs.size()){ SetLastError(ERROR_INVALID_DATA); return nullptr; }
            DWORD rva = funcs[ord];
            return (LPTHREAD_START_ROUTINE)(base + rva);
        }
    }
    SetLastError(ERROR_PROC_NOT_FOUND);
    return nullptr;
}

int wmain(int argc, wchar_t** argv){
    if(argc<3){ fwprintf(stderr,L"usage: injector_pe_x64_v2.exe <pid> <dll>\n"); return 1; }
    wchar_t* end = nullptr;
    unsigned long parsedPid = wcstoul(argv[1], &end, 10);
    if (!argv[1][0] || (end && *end != L'\0') || parsedPid == 0) {
        fwprintf(stderr, L"[inj] invalid pid: %s\n", argv[1]);
        return 1;
    }
    DWORD pid = (DWORD)parsedPid;
    std::wstring dll = FullPathOf(argv[2]);
    DWORD attr = GetFileAttributesW(dll.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES || (attr & FILE_ATTRIBUTE_DIRECTORY)) {
        SetLastError(ERROR_FILE_NOT_FOUND);
        die(L"DLL path is invalid");
    }

    HANDLE hProc = OpenProcess(PROCESS_CREATE_THREAD|PROCESS_QUERY_INFORMATION|
                               PROCESS_VM_OPERATION|PROCESS_VM_WRITE|PROCESS_VM_READ,
                               FALSE, pid);
    if(!hProc) die(L"OpenProcess failed");

    // DLL パスをリモートに置く
    SIZE_T bytes=(dll.size()+1)*sizeof(wchar_t);
    LPVOID remote=VirtualAllocEx(hProc,nullptr,bytes,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    if(!remote) die(L"VirtualAllocEx failed");
    if(!WriteProcessMemory(hProc,remote,dll.c_str(),bytes,nullptr)) die(L"WriteProcessMemory failed");

    // LoadLibraryW をリモートで呼ぶ
    auto pLL=(LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"LoadLibraryW");
    if(!pLL) die(L"GetProcAddress(LoadLibraryW) failed");
    HANDLE hLL=CreateRemoteThread(hProc,nullptr,0,pLL,remote,0,nullptr);
    if(!hLL) die(L"CreateRemoteThread(LoadLibraryW) failed");
    if (WaitForSingleObject(hLL, INFINITE) != WAIT_OBJECT_0) die(L"WaitForSingleObject(LoadLibraryW) failed");
    DWORD llCode = 0;
    if (!GetExitCodeThread(hLL, &llCode)) die(L"GetExitCodeThread(LoadLibraryW) failed");
    if (llCode == 0) die(L"LoadLibraryW failed in remote process");
    CloseHandle(hLL);
    VirtualFreeEx(hProc,remote,0,MEM_RELEASE);

    // ★ ツールヘルプで本当の HMODULE を取得
    HMODULE remoteBase = FindRemoteModuleByName(pid, dll.c_str());
    if(!remoteBase) die(L"FindRemoteModuleByName failed (module not loaded?)");

    // エクスポートから StartAgent を解決
    LPTHREAD_START_ROUTINE pRemoteStart =
        RemoteGetExportByName(hProc, (BYTE*)remoteBase, "StartAgent");
    if(!pRemoteStart) die(L"RemoteGetExportByName(StartAgent) failed");

    // StartAgent をリモートで呼ぶ
    HANDLE hSA=CreateRemoteThread(hProc,nullptr,0,pRemoteStart,nullptr,0,nullptr);
    if(!hSA) die(L"CreateRemoteThread(StartAgent) failed");
    WaitForSingleObject(hSA,2000);
    DWORD code=0; GetExitCodeThread(hSA,&code);  // これは DWORD でOK（StartAgent は DWORD 戻り）
    CloseHandle(hSA);

    CloseHandle(hProc);
    wprintf(L"OK (remoteBase=0x%p, StartAgent exit=0x%08lX)\n",(void*)remoteBase,code);
    return 0;
}
