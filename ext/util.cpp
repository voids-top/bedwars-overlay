#ifndef AGENTS_MIN_WINSOCK_INCLUDE_GUARD
#define AGENTS_MIN_WINSOCK_INCLUDE_GUARD

#define _CRT_SECURE_NO_WARNINGS
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#pragma comment(lib, "Ws2_32.lib")
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <jni.h>
#include <jvmti.h>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>

#pragma comment(lib, "Ws2_32.lib")
#define BUS_HOST "127.0.0.1"
#define BUS_PORT 46012
// Try ports in order to avoid clashes with other listeners and catch older servers if needed
static const int BUS_PORTS[] = { BUS_PORT, 46011, 46001 };
static const size_t BUS_PORTS_COUNT = sizeof(BUS_PORTS) / sizeof(BUS_PORTS[0]);


static void dlog(const char* fmt, ...) {
    char buf[2048];
    va_list ap; va_start(ap, fmt);
    _vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, ap);
    va_end(ap);
    OutputDebugStringA("[AGENT] ");
    OutputDebugStringA(buf);
    OutputDebugStringA("\n");
}

static void flog(const char* fmt, ...) {
    char buf[2048];
    va_list ap; va_start(ap, fmt);
    _vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, ap);
    va_end(ap);
    char line[2200];
    _snprintf_s(line, _TRUNCATE, "%s\r\n", buf);
    HANDLE h = CreateFileW(L"C:\\Users\\user\\bedwars-overlay\\build\\agent.log", GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return;
    SetFilePointer(h, 0, NULL, FILE_END);
    DWORD written = 0;
    WriteFile(h, line, (DWORD)strlen(line), &written, NULL);
    CloseHandle(h);
}

static JavaVM* g_vm = nullptr;
typedef jint (JNICALL* PFN_JNI_GetCreatedJavaVMs)(JavaVM**, jsize, jsize*);

static JNIEnv* get_env() {
    if (!g_vm) {
        HMODULE hjvm = GetModuleHandleW(L"jvm.dll");
        if (!hjvm) { dlog("jvm.dll not found"); return nullptr; }
        auto p = (PFN_JNI_GetCreatedJavaVMs)GetProcAddress(hjvm, "JNI_GetCreatedJavaVMs");
        if (!p) { dlog("GetProcAddress(JNI_GetCreatedJavaVMs) failed"); return nullptr; }
        JavaVM* vms[4] = {}; jsize n = 0;
        if (p(vms, 4, &n) != 0 || n <= 0) { dlog("JNI_GetCreatedJavaVMs failed"); return nullptr; }
        g_vm = vms[0];
    }
    JNIEnv* env = nullptr;
    jint rc = g_vm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (rc == JNI_EDETACHED) {
        if (g_vm->AttachCurrentThread((void**)&env, nullptr) != 0) { dlog("AttachCurrentThread failed"); return nullptr; }
    } else if (rc != JNI_OK) {
        dlog("GetEnv rc=%d", (int)rc); return nullptr;
    }
    return env;
}

struct JniLocalFrame {
    JNIEnv* env;
    JniLocalFrame(JNIEnv* e, jint capacity = 64) : env(e) {
        if (env) {
            if (env->PushLocalFrame(capacity) < 0) {
                env = nullptr;
            }
        }
    }
    ~JniLocalFrame() {
        if (env) {
            env->PopLocalFrame(nullptr);
        }
    }
};

static std::string jstr(JNIEnv* env, jstring s) {
    if (!s) return std::string();
    const jchar* pw = env->GetStringChars(s, nullptr);
    jsize nch = env->GetStringLength(s);
    // UTF-16LE -> UTF-8
    int need = WideCharToMultiByte(CP_UTF8, 0,
                                   (LPCWCH)pw, nch,
                                   nullptr, 0, nullptr, nullptr);
    std::string out;
    out.resize(need);
    if (need > 0) {
        WideCharToMultiByte(CP_UTF8, 0,
                            (LPCWCH)pw, nch,
                            &out[0], need, nullptr, nullptr);
    }
    env->ReleaseStringChars(s, pw);
    return out;
}

static std::string strip_mc_codes(const std::string& in) {
    std::string out; out.reserve(in.size());
    for (size_t i = 0; i < in.size(); ++i) {
        unsigned char c = (unsigned char)in[i];
        if (c == 0xC2 && i + 1 < in.size() && (unsigned char)in[i+1] == 0xA7) { // UTF-8 '§' starts as 0xC2 0xA7
            // skip '§' and next one byte (format code)
            i += 1;
            if (i + 1 < in.size()) i += 1;
            continue;
        }
        if (c == 0xA7) { // raw '§' (rare)
            if (i + 1 < in.size()) i += 1;
            continue;
        }
        // drop any multi-byte that encodes surrogate / private use (ED A0..ED BF.. / EE 80..EE 9F..)
        if (c >= 0xED) {
            // skip a UTF-8 sequence (2~4 bytes)
            if      ((c & 0xF8) == 0xF0) { i += 3; }
            else if ((c & 0xF0) == 0xE0) { i += 2; }
            else if ((c & 0xE0) == 0xC0) { i += 1; }
            // but we don't append it
            continue;
        }
        // keep ASCII printable (space..~)
        if (c >= 0x20 && c <= 0x7E) out.push_back((char)c);
        else if (c == '\n' || c == '\r' || c == '\t') out.push_back((char)c);
    }
    return out;
}

static std::string trim(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && (unsigned char)s[a] <= 0x20) ++a;
    while (b > a && (unsigned char)s[b-1] <= 0x20) --b;
    return s.substr(a, b - a);
}

static jmethodID tryMethod(JNIEnv* env, jclass cls,
    const std::vector<std::pair<const char*, const char*>>& cands,
    const char* tag)
{
    for (size_t i = 0; i < cands.size(); ++i) {
        jmethodID m = env->GetMethodID(cls, cands[i].first, cands[i].second);
        if (m) { dlog("%s: resolved '%s' %s", tag, cands[i].first, cands[i].second); return m; }
        env->ExceptionClear();
    }
    dlog("%s: no candidate matched", tag);
    return nullptr;
}

static std::string ascii_only(const std::string& in) {
    std::string out; out.reserve(in.size());
    for (size_t i = 0; i < in.size(); ++i) {
        unsigned char c = (unsigned char)in[i];
        if (c >= 32 && c <= 126) out.push_back((char)c);
    }
    return out;
}

static jobject getTCCL(JNIEnv* env) {
    jclass cThread = env->FindClass("java/lang/Thread");
    if (!cThread) { env->ExceptionClear(); return nullptr; }
    jmethodID mCur = env->GetStaticMethodID(cThread, "currentThread", "()Ljava/lang/Thread;");
    if (!mCur) { env->ExceptionClear(); return nullptr; }
    jobject t = env->CallStaticObjectMethod(cThread, mCur);
    if (env->ExceptionCheck()) { env->ExceptionClear(); return nullptr; }
    jmethodID mCL = env->GetMethodID(cThread, "getContextClassLoader", "()Ljava/lang/ClassLoader;");
    if (!mCL) { env->ExceptionClear(); return nullptr; }
    jobject cl = env->CallObjectMethod(t, mCL);
    if (env->ExceptionCheck()) { env->ExceptionClear(); return nullptr; }
    return cl;
}
static jobject getSysCL(JNIEnv* env) {
    jclass cCL = env->FindClass("java/lang/ClassLoader");
    if (!cCL) { env->ExceptionClear(); return nullptr; }
    jmethodID m = env->GetStaticMethodID(cCL, "getSystemClassLoader", "()Ljava/lang/ClassLoader;");
    if (!m) { env->ExceptionClear(); return nullptr; }
    jobject cl = env->CallStaticObjectMethod(cCL, m);
    if (env->ExceptionCheck()) { env->ExceptionClear(); return nullptr; }
    return cl;
}
static jclass forName(JNIEnv* env, const char* dot, jobject loader) {
    jclass cClass = env->FindClass("java/lang/Class");
    if (!cClass) { env->ExceptionClear(); return nullptr; }
    jmethodID mid = env->GetStaticMethodID(
        cClass, "forName", "(Ljava/lang/String;ZLjava/lang/ClassLoader;)Ljava/lang/Class;");
    if (!mid) { env->ExceptionClear(); return nullptr; }
    jstring jn = env->NewStringUTF(dot);
    jobject r = env->CallStaticObjectMethod(cClass, mid, jn, JNI_FALSE, loader);
    if (env->ExceptionCheck()) { env->ExceptionClear(); r = nullptr; }
    env->DeleteLocalRef(jn);
    return (jclass)r;
}
static void collect_loaders(JNIEnv* env, std::vector<jobject>& out) {
    auto add = [&](jobject cl) {
        if (!cl) return;
        for (auto e: out) if (env->IsSameObject(e, cl)) return;
        out.push_back(env->NewGlobalRef(cl));
    };
    add(getTCCL(env));
    add(getSysCL(env));

    jclass cThread = env->FindClass("java/lang/Thread");
    if (!cThread) { env->ExceptionClear(); return; }
    jmethodID mAll = env->GetStaticMethodID(cThread, "getAllStackTraces", "()Ljava/util/Map;");
    if (!mAll) { env->ExceptionClear(); return; }
    jobject mp = env->CallStaticObjectMethod(cThread, mAll);
    if (env->ExceptionCheck() || !mp) { env->ExceptionClear(); return; }

    jclass cMap = env->FindClass("java/util/Map");
    jclass cSet = env->FindClass("java/util/Set");
    jclass cObj = env->FindClass("java/lang/Object");
    if (!cMap || !cSet || !cObj) { env->ExceptionClear(); return; }
    jmethodID mKeySet = env->GetMethodID(cMap, "keySet", "()Ljava/util/Set;");
    jmethodID mToArr  = env->GetMethodID(cSet, "toArray", "([Ljava/lang/Object;)[Ljava/lang/Object;");
    if (!mKeySet || !mToArr) { env->ExceptionClear(); return; }
    jobject st = env->CallObjectMethod(mp, mKeySet);
    if (env->ExceptionCheck() || !st) { env->ExceptionClear(); return; }
    jobjectArray empty = env->NewObjectArray(0, cObj, nullptr);
    jobjectArray arr   = (jobjectArray)env->CallObjectMethod(st, mToArr, empty);
    env->DeleteLocalRef(empty);
    if (env->ExceptionCheck() || !arr) { env->ExceptionClear(); return; }
    jmethodID mGetCL = env->GetMethodID(cThread, "getContextClassLoader", "()Ljava/lang/ClassLoader;");
    jsize n = env->GetArrayLength(arr);
    for (jsize i=0;i<n;++i) {
        jobject th = env->GetObjectArrayElement(arr, i);
        jobject cl = env->CallObjectMethod(th, mGetCL);
        if (!env->ExceptionCheck()) add(cl);
        env->DeleteLocalRef(th);
        if (cl) env->DeleteLocalRef(cl);
    }
    env->DeleteLocalRef(arr);
}

static bool is_minecraft_signature(const char* sig) {
    if (!sig) return false;
    if (strcmp(sig, "Lnet/minecraft/client/Minecraft;") == 0) return true;
    if (strcmp(sig, "Lave;") == 0) return true;
    if (strcmp(sig, "Lbsu;") == 0) return true;
    if (strstr(sig, "/minecraft/client/Minecraft;") != nullptr) return true;
    return false;
}

static jclass find_mc_class_via_jvmti(JNIEnv* env) {
    if (!env || !g_vm) return nullptr;

    jvmtiEnv* jvmti = nullptr;
    jint rc = g_vm->GetEnv((void**)&jvmti, JVMTI_VERSION_1_2);
    if (rc != JNI_OK || !jvmti) {
        dlog("step0: JVMTI unavailable rc=%d", (int)rc);
        return nullptr;
    }

    jint count = 0;
    jclass* classes = nullptr;
    jvmtiError err = jvmti->GetLoadedClasses(&count, &classes);
    if (err != JVMTI_ERROR_NONE || !classes || count <= 0) {
        dlog("step0: JVMTI GetLoadedClasses failed err=%d", (int)err);
        if (classes) jvmti->Deallocate((unsigned char*)classes);
        return nullptr;
    }

    jclass hit = nullptr;
    for (jint i = 0; i < count; ++i) {
        char* sig = nullptr;
        if (jvmti->GetClassSignature(classes[i], &sig, nullptr) != JVMTI_ERROR_NONE || !sig) {
            continue;
        }
        if (is_minecraft_signature(sig)) {
            hit = (jclass)env->NewLocalRef(classes[i]);
            dlog("step0: JVMTI hit: %s", sig);
            jvmti->Deallocate((unsigned char*)sig);
            break;
        }
        jvmti->Deallocate((unsigned char*)sig);
    }

    jvmti->Deallocate((unsigned char*)classes);
    return hit;
}

static jclass find_mc_class(JNIEnv* env) {
    const char* candidates[] = {
        "net.minecraft.client.Minecraft", // deobf
        "ave",                            // obf (1.8.9)
        "bsu",
    };
    std::vector<jobject> loaders; collect_loaders(env, loaders);
    for (auto n: candidates) {
        jclass c = env->FindClass(n[0]=='n' ? "net/minecraft/client/Minecraft" : n);
        if (c) { dlog("step0: FindClass hit: %s", n); for (auto g: loaders) env->DeleteGlobalRef(g); return c; }
        env->ExceptionClear();
    }
    for (auto n: candidates) for (auto cl: loaders) {
        jclass c = forName(env, n, cl);
        if (c) { dlog("step0: class loaded by some loader: %s", n); for (auto g: loaders) env->DeleteGlobalRef(g); return c; }
    }
    for (auto g: loaders) env->DeleteGlobalRef(g);

    jclass viaJvmti = find_mc_class_via_jvmti(env);
    if (viaJvmti) return viaJvmti;

    dlog("step0: class lookup fallback exhausted");
    return nullptr;
}
static jobject get_minecraft_singleton(JNIEnv* env) {
    jclass mc = find_mc_class(env);
    if (!mc) {
        dlog("step0 NG: Minecraft class not found");
        return nullptr;
    }

    // まずは従来の「名前決め打ち」パス（Vanilla / Lunar 等）
    struct M { const char* n; const char* s; } methods[] = {
        {"getMinecraft","()Lnet/minecraft/client/Minecraft;"},
        {"getMinecraft","()Ljava/lang/Object;"},
        {"func_71410_x","()Lnet/minecraft/client/Minecraft;"},
        {"func_71410_x","()Ljava/lang/Object;"},
        {"S","()Lave;"},
        {"S","()Ljava/lang/Object;"},
    };
    for (auto& m : methods) {
        jmethodID mid = env->GetStaticMethodID(mc, m.n, m.s);
        if (!mid) { env->ExceptionClear(); continue; }
        jobject inst = env->CallStaticObjectMethod(mc, mid);
        if (env->ExceptionCheck()) { env->ExceptionClear(); continue; }
        if (inst) {
            dlog("step0: got minecraft_singleton() via %s%s", m.n, m.s);
            return inst;
        }
    }

    struct F { const char* n; const char* s; } fields[] = {
        {"theMinecraft","Lnet/minecraft/client/Minecraft;"},
        {"theMinecraft","Ljava/lang/Object;"},
        {"M","Lave;"},
        {"M","Ljava/lang/Object;"},
    };
    for (auto& f : fields) {
        jfieldID fid = env->GetStaticFieldID(mc, f.n, f.s);
        if (!fid) { env->ExceptionClear(); continue; }
        jobject inst = env->GetStaticObjectField(mc, fid);
        if (env->ExceptionCheck()) { env->ExceptionClear(); continue; }
        if (inst) {
            dlog("step0: got minecraft_singleton field %s", f.n);
            return inst;
        }
    }

    // ======== ここから Badlion 対策の「リフレクション fallback」 ========

    // mc の Class オブジェクト (java.lang.Class)
    jclass cClass = env->GetObjectClass(mc);
    if (!cClass) { env->ExceptionClear(); dlog("step0 NG: no Class for mc"); return nullptr; }

    jclass cField    = env->FindClass("java/lang/reflect/Field");
    jclass cMethod   = env->FindClass("java/lang/reflect/Method");
    jclass cModifier = env->FindClass("java/lang/reflect/Modifier");
    if (!cField || !cMethod || !cModifier) {
        env->ExceptionClear();
        dlog("step0 NG: reflect classes missing");
        return nullptr;
    }

    jmethodID midGetFields    = env->GetMethodID(cClass, "getDeclaredFields",  "()[Ljava/lang/reflect/Field;");
    jmethodID midGetMethods   = env->GetMethodID(cClass, "getDeclaredMethods", "()[Ljava/lang/reflect/Method;");
    jmethodID midFieldType    = env->GetMethodID(cField, "getType",           "()Ljava/lang/Class;");
    jmethodID midFieldMods    = env->GetMethodID(cField, "getModifiers",      "()I");
    jmethodID midMethodRT     = env->GetMethodID(cMethod, "getReturnType",    "()Ljava/lang/Class;");
    jmethodID midMethodMods   = env->GetMethodID(cMethod, "getModifiers",     "()I");
    jmethodID midMethodParams = env->GetMethodID(cMethod, "getParameterTypes","()[Ljava/lang/Class;");
    jmethodID midIsStatic     = env->GetStaticMethodID(cModifier, "isStatic", "(I)Z");

    if (!midGetFields || !midGetMethods || !midFieldType || !midFieldMods ||
        !midMethodRT || !midMethodMods || !midMethodParams || !midIsStatic) {
        env->ExceptionClear();
        dlog("step0 NG: reflect methods missing");
        return nullptr;
    }

    // --- 1) static フィールドを総当たり ---
    {
        jobjectArray fldArr = (jobjectArray)env->CallObjectMethod(mc, midGetFields);
        if (!env->ExceptionCheck() && fldArr) {
            jsize n = env->GetArrayLength(fldArr);
            for (jsize i = 0; i < n; ++i) {
                jobject f = env->GetObjectArrayElement(fldArr, i);
                if (!f) continue;

                jobject t = env->CallObjectMethod(f, midFieldType);
                if (env->ExceptionCheck() || !t) {
                    env->ExceptionClear();
                    env->DeleteLocalRef(f);
                    continue;
                }

                jboolean sameType = env->IsSameObject(t, mc);
                env->DeleteLocalRef(t);
                if (!sameType) {
                    env->DeleteLocalRef(f);
                    continue;
                }

                jint mods = env->CallIntMethod(f, midFieldMods);
                jboolean isStatic = env->CallStaticBooleanMethod(cModifier, midIsStatic, mods);
                if (env->ExceptionCheck()) {
                    env->ExceptionClear();
                    env->DeleteLocalRef(f);
                    continue;
                }
                if (!isStatic) {
                    env->DeleteLocalRef(f);
                    continue;
                }

                jfieldID fid = env->FromReflectedField(f);
                env->DeleteLocalRef(f);
                if (!fid) continue;

                jobject inst = env->GetStaticObjectField(mc, fid);
                if (env->ExceptionCheck()) {
                    env->ExceptionClear();
                    continue;
                }
                if (inst) {
                    dlog("step0: got minecraft_singleton via reflected static field");
                    env->DeleteLocalRef(fldArr);
                    return inst;
                }
            }
            env->DeleteLocalRef(fldArr);
        } else {
            env->ExceptionClear();
        }
    }

    // --- 2) static メソッド（戻り値 mc, 引数 0）を総当たり ---
    {
        jobjectArray mArr = (jobjectArray)env->CallObjectMethod(mc, midGetMethods);
        if (!env->ExceptionCheck() && mArr) {
            jsize n = env->GetArrayLength(mArr);
            for (jsize i = 0; i < n; ++i) {
                jobject m = env->GetObjectArrayElement(mArr, i);
                if (!m) continue;

                // 戻り値の型 == mc ?
                jobject rt = env->CallObjectMethod(m, midMethodRT);
                if (env->ExceptionCheck() || !rt) {
                    env->ExceptionClear();
                    env->DeleteLocalRef(m);
                    continue;
                }
                jboolean sameType = env->IsSameObject(rt, mc);
                env->DeleteLocalRef(rt);
                if (!sameType) {
                    env->DeleteLocalRef(m);
                    continue;
                }

                // static か？
                jint mods = env->CallIntMethod(m, midMethodMods);
                jboolean isStatic = env->CallStaticBooleanMethod(cModifier, midIsStatic, mods);
                if (env->ExceptionCheck()) {
                    env->ExceptionClear();
                    env->DeleteLocalRef(m);
                    continue;
                }
                if (!isStatic) {
                    env->DeleteLocalRef(m);
                    continue;
                }

                // 引数 0 個か？
                jobjectArray params = (jobjectArray)env->CallObjectMethod(m, midMethodParams);
                if (env->ExceptionCheck() || !params) {
                    env->ExceptionClear();
                    env->DeleteLocalRef(m);
                    continue;
                }
                jsize pc = env->GetArrayLength(params);
                env->DeleteLocalRef(params);
                if (pc != 0) {
                    env->DeleteLocalRef(m);
                    continue;
                }

                // 呼んでみる
                jmethodID mid = env->FromReflectedMethod(m);
                env->DeleteLocalRef(m);
                if (!mid) continue;

                jobject inst = env->CallStaticObjectMethod(mc, mid);
                if (env->ExceptionCheck()) {
                    env->ExceptionClear();
                    continue;
                }
                if (inst) {
                    dlog("step0: got minecraft_singleton via reflected static method");
                    env->DeleteLocalRef(mArr);
                    return inst;
                }
            }
            env->DeleteLocalRef(mArr);
        } else {
            env->ExceptionClear();
        }
    }

    dlog("step0 NG: no static method/field yielded instance (even reflected)");
    return nullptr;
}

static void send_line(SOCKET c, const char* s, bool with_nul=false) {
    if (!s) s = "";
    int sent = 0;
    if (with_nul) {
        std::string body(s);
        body.push_back('\n');
        body.push_back('\0');
        sent = send(c, body.c_str(), (int)body.size(), 0);
        dlog("BUS: sent %d bytes (+NUL)", sent);
    } else {
        std::string body(s); body.push_back('\n');
        sent = send(c, body.c_str(), (int)body.size(), 0);
        dlog("BUS: sent %d bytes", sent);
    }
}

static void send_line(SOCKET c, const std::string& s, bool with_nul) {
    send_line(c, s.c_str(), with_nul);
}

static jobject make_chat_packet(JNIEnv* env, jclass cC01, jstring jmsg) {
    if (!env || !cC01 || !jmsg) return nullptr;
    jclass cUnsafe = env->FindClass("sun/misc/Unsafe");
    if (!cUnsafe) { env->ExceptionClear(); dlog("chat: Unsafe class not found"); return nullptr; }
    jfieldID fTheUnsafe = env->GetStaticFieldID(cUnsafe, "theUnsafe", "Lsun/misc/Unsafe;");
    if (!fTheUnsafe) { env->ExceptionClear(); dlog("chat: Unsafe.theUnsafe not found"); return nullptr; }
    jobject u = env->GetStaticObjectField(cUnsafe, fTheUnsafe);
    if (!u || env->ExceptionCheck()) { env->ExceptionClear(); dlog("chat: Unsafe instance not available"); return nullptr; }
    jmethodID midAlloc = env->GetMethodID(cUnsafe, "allocateInstance", "(Ljava/lang/Class;)Ljava/lang/Object;");
    if (!midAlloc) { env->ExceptionClear(); dlog("chat: Unsafe.allocateInstance not found"); return nullptr; }
    jobject pkt = env->CallObjectMethod(u, midAlloc, cC01);
    if (!pkt || env->ExceptionCheck()) { env->ExceptionClear(); dlog("chat: Unsafe.allocateInstance failed"); return nullptr; }

    jclass cClass = env->FindClass("java/lang/Class");
    jclass cField = env->FindClass("java/lang/reflect/Field");
    if (!cClass || !cField) { env->ExceptionClear(); dlog("chat: reflection classes missing for field set"); return nullptr; }
    jmethodID midGetFields = env->GetMethodID(cClass, "getDeclaredFields", "()[Ljava/lang/reflect/Field;");
    jmethodID midGetType   = env->GetMethodID(cField, "getType",           "()Ljava/lang/Class;");
    jmethodID midSetAcc    = env->GetMethodID(cField, "setAccessible",     "(Z)V");
    jmethodID midSet       = env->GetMethodID(cField, "set",               "(Ljava/lang/Object;Ljava/lang/Object;)V");
    jmethodID midGetName   = env->GetMethodID(cClass, "getName",           "()Ljava/lang/String;");
    if (!midGetFields || !midGetType || !midSet || !midGetName) { env->ExceptionClear(); dlog("chat: reflection methods missing for field set"); return nullptr; }

    jobjectArray flds = (jobjectArray)env->CallObjectMethod(cC01, midGetFields);
    if (env->ExceptionCheck() || !flds) { env->ExceptionClear(); dlog("chat: getDeclaredFields failed on C01"); return nullptr; }
    jsize n = env->GetArrayLength(flds);
    bool set = false;
    for (jsize i = 0; i < n && !set; ++i) {
        jobject f = env->GetObjectArrayElement(flds, i);
        if (!f) continue;
        jobject t = env->CallObjectMethod(f, midGetType);
        if (env->ExceptionCheck() || !t) { env->ExceptionClear(); env->DeleteLocalRef(f); continue; }
        jstring jn = (jstring)env->CallObjectMethod((jclass)t, midGetName);
        std::string tn = jn ? jstr(env, jn) : std::string();
        if (jn) env->DeleteLocalRef(jn);
        if (tn == "java.lang.String") {
            if (midSetAcc) env->CallVoidMethod(f, midSetAcc, JNI_TRUE);
            env->CallVoidMethod(f, midSet, pkt, jmsg);
            if (!env->ExceptionCheck()) set = true; else env->ExceptionClear();
        }
        env->DeleteLocalRef(t);
        env->DeleteLocalRef(f);
    }
    env->DeleteLocalRef(flds);
    if (!set) { dlog("chat: Unsafe-allocated C01 but no String field set"); return nullptr; }
    dlog("chat: C01 packet created via Unsafe fallback");
    return pkt;
}

static bool send_chat(JNIEnv* env, jobject mc, const char* text) {
    if (!env || !mc || !text) return false;

    // Sanitize/trim and enforce vanilla length limit (~256 chars)
    std::string msg = trim(std::string(text));
    if (msg.empty()) { dlog("chat: empty"); return false; }
    if (msg.size() > 256) msg.resize(256);
    {
        std::string msgLog = ascii_only(msg);
        dlog("chat: request='%s'", msgLog.c_str());
    }

    jclass clsMc = env->GetObjectClass(mc);
    if (!clsMc) { env->ExceptionClear(); dlog("chat: no mc class"); return false; }

    // Fast path: resolve NetHandler directly from Minecraft (getNetHandler/func_147114_u) and send packet
    {
        jmethodID midNH = tryMethod(env, clsMc, {
            {"getNetHandler", "()Lnet/minecraft/client/network/NetHandlerPlayClient;"},
            {"func_147114_u", "()Lnet/minecraft/client/network/NetHandlerPlayClient;"},
        }, "chat:getNH");
        if (midNH) {
            jobject net = env->CallObjectMethod(mc, midNH);
            if (env->ExceptionCheck()) { env->ExceptionClear(); net = nullptr; }
            if (net) {
                jclass cNet = env->GetObjectClass(net);
                if (cNet) {
                    // Resolve classes using the same ClassLoader that loaded NetHandler to avoid loader mismatches
                    jclass cClass = env->FindClass("java/lang/Class");
                    jmethodID midGetCL = cClass ? env->GetMethodID(cClass, "getClassLoader", "()Ljava/lang/ClassLoader;") : nullptr;
                    jobject cl = midGetCL ? env->CallObjectMethod(cNet, midGetCL) : nullptr;
                    if (env->ExceptionCheck()) { env->ExceptionClear(); cl = nullptr; }

                    jclass cC01 = nullptr;
                    if (cl) {
                        cC01 = forName(env, "net.minecraft.network.play.client.C01PacketChatMessage", cl);
                        if (!cC01) cC01 = forName(env, "C01PacketChatMessage", cl);
                        if (!cC01) cC01 = forName(env, "ih", cl);
                    } else {
                        cC01 = env->FindClass("net/minecraft/network/play/client/C01PacketChatMessage");
                        if (!cC01) { env->ExceptionClear(); cC01 = env->FindClass("C01PacketChatMessage"); }
                        if (!cC01) { env->ExceptionClear(); cC01 = env->FindClass("ih"); }
                    }
                    if (cC01) {
                        jmethodID ctor = env->GetMethodID(cC01, "<init>", "(Ljava/lang/String;)V");
                        if (ctor) {
                            jstring jmsg = env->NewStringUTF(msg.c_str());
                            jobject pkt = env->NewObject(cC01, ctor, jmsg);
                            if (pkt && !env->ExceptionCheck()) {
                                jmethodID mSend = tryMethod(env, cNet, {
                                    {"addToSendQueue", "(Lnet/minecraft/network/Packet;)V"},
                                    {"sendPacket",     "(Lnet/minecraft/network/Packet;)V"}
                                }, "chat:sendPkt");
                                if (!mSend) {
                                    env->ExceptionClear();
                                    mSend = tryMethod(env, cNet, {
                                        {"a", "(Lff;)V"}
                                    }, "chat:sendPkt-obf");
                                }
                                if (mSend) {
                                    env->CallVoidMethod(net, mSend, pkt);
                                    bool ok = !env->ExceptionCheck();
                                    if (!ok) { env->ExceptionDescribe(); env->ExceptionClear(); dlog("chat: addToSendQueue threw (mc->nh path)"); }
                                    if (ok) {
                                        if (jmsg) env->DeleteLocalRef(jmsg);
                                        env->DeleteLocalRef(pkt);
                                        env->DeleteLocalRef(cC01);
                                        env->DeleteLocalRef(cNet);
                                        env->DeleteLocalRef(net);
                                        dlog("chat sent (packet path via mc.getNetHandler)");
                                        return true;
                                    }
                                }
                                // Fallback: try NetworkManager.sendPacket or Channel.writeAndFlush
                                {
                                    jfieldID fidNM = env->GetFieldID(cNet, "netManager", "Lnet/minecraft/network/NetworkManager;");
                                    if (!fidNM) { env->ExceptionClear(); fidNM = env->GetFieldID(cNet, "field_147302_e", "Lnet/minecraft/network/NetworkManager;"); }
                                    if (!fidNM) {
                                        // Reflection fallback: any field whose type has sendPacket/func_179290_a/a(Lff;)V or Channel
                                        jclass cClass2 = env->FindClass("java/lang/Class");
                                        jclass cField2 = env->FindClass("java/lang/reflect/Field");
                                        if (cClass2 && cField2) {
                                            jmethodID mGetFields2 = env->GetMethodID(cClass2, "getDeclaredFields", "()[Ljava/lang/reflect/Field;");
                                            jmethodID mGetType2   = env->GetMethodID(cField2, "getType",           "()Ljava/lang/Class;");
                                            jmethodID mSetAcc2    = env->GetMethodID(cField2, "setAccessible",     "(Z)V");
                                            if (mGetFields2 && mGetType2) {
                                                jobjectArray flds2 = (jobjectArray)env->CallObjectMethod(cNet, mGetFields2);
                                                if (!env->ExceptionCheck() && flds2) {
                                                    jsize fn2 = env->GetArrayLength(flds2);
                                                    for (jsize i2 = 0; i2 < fn2 && !fidNM; ++i2) {
                                                        jobject f2 = env->GetObjectArrayElement(flds2, i2);
                                                        if (!f2) continue;
                                                        jobject t2 = env->CallObjectMethod(f2, mGetType2);
                                                        if (env->ExceptionCheck() || !t2) { env->ExceptionClear(); env->DeleteLocalRef(f2); continue; }
                                                        jclass cType2 = (jclass)t2;
                                                        jmethodID mTry2 = env->GetMethodID(cType2, "sendPacket", "(Lnet/minecraft/network/Packet;)V");
                                                        if (!mTry2) { env->ExceptionClear(); mTry2 = env->GetMethodID(cType2, "func_179290_a", "(Lnet/minecraft/network/Packet;)V"); }
                                                        if (!mTry2) { env->ExceptionClear(); mTry2 = env->GetMethodID(cType2, "a", "(Lff;)V"); }
                                                        // or has Netty Channel
                                                        if (!mTry2) {
                                                            jclass cChan = env->FindClass("io/netty/channel/Channel");
                                                            if (cChan) {
                                                                // assignable check via getName compare
                                                                jclass cCls = env->FindClass("java/lang/Class");
                                                                jmethodID midGetName = cCls ? env->GetMethodID(cCls, "getName", "()Ljava/lang/String;") : nullptr;
                                                                if (midGetName) {
                                                                    jstring jn = (jstring)env->CallObjectMethod(cType2, midGetName);
                                                                    std::string tn = jn ? jstr(env, jn) : std::string();
                                                                    if (jn) env->DeleteLocalRef(jn);
                                                                    if (tn == "io.netty.channel.Channel") mTry2 = (jmethodID)0x1; // marker: it's a Channel field
                                                                }
                                                            } else env->ExceptionClear();
                                                        }
                                                        if (mTry2) {
                                                            if (mSetAcc2) env->CallVoidMethod(f2, mSetAcc2, JNI_TRUE);
                                                            fidNM = env->FromReflectedField(f2);
                                                        }
                                                        env->DeleteLocalRef(t2);
                                                        env->DeleteLocalRef(f2);
                                                    }
                                                    env->DeleteLocalRef(flds2);
                                                } else env->ExceptionClear();
                                            } else env->ExceptionClear();
                                        } else env->ExceptionClear();
                                    }
                                    if (fidNM) {
                                        jobject nm = env->GetObjectField(net, fidNM);
                                        if (!env->ExceptionCheck() && nm) {
                                            jclass cNM = env->GetObjectClass(nm);
                                            bool sentOK = false;
                                            bool usedChannel = false;

                                            // Prefer Netty Channel.writeAndFlush(pkt) first to ensure scheduling on the event loop
                                            jfieldID fidCh = env->GetFieldID(cNM, "channel", "Lio/netty/channel/Channel;");
                                            if (!fidCh) { env->ExceptionClear(); fidCh = env->GetFieldID(cNM, "field_150746_k", "Lio/netty/channel/Channel;"); }
                                            if (!fidCh) {
                                                // reflection scan for Channel-typed field (compare type name to avoid loader issues)
                                                jclass cClass3 = env->FindClass("java/lang/Class");
                                                jclass cField3 = env->FindClass("java/lang/reflect/Field");
                                                if (cClass3 && cField3) {
                                                    jmethodID mGetFields3 = env->GetMethodID(cClass3, "getDeclaredFields", "()[Ljava/lang/reflect/Field;");
                                                    jmethodID mGetType3   = env->GetMethodID(cField3, "getType",           "()Ljava/lang/Class;");
                                                    jmethodID mSetAcc3    = env->GetMethodID(cField3, "setAccessible",     "(Z)V");
                                                    jclass cChan = env->FindClass("io/netty/channel/Channel");
                                                    if (mGetFields3 && mGetType3 && cChan) {
                                                        jobjectArray flds3 = (jobjectArray)env->CallObjectMethod(cNM, mGetFields3);
                                                        if (!env->ExceptionCheck() && flds3) {
                                                            jsize fn3 = env->GetArrayLength(flds3);
                                                            for (jsize i3=0; i3<fn3 && !fidCh; ++i3) {
                                                                jobject f3 = env->GetObjectArrayElement(flds3, i3);
                                                                if (!f3) continue;
                                                                jobject t3 = env->CallObjectMethod(f3, mGetType3);
                                                                if (env->ExceptionCheck() || !t3) { env->ExceptionClear(); env->DeleteLocalRef(f3); continue; }
                                                                jclass cCls = env->FindClass("java/lang/Class");
                                                                jmethodID midGetName3 = cCls ? env->GetMethodID(cCls, "getName", "()Ljava/lang/String;") : nullptr;
                                                                bool isChan = false;
                                                                if (midGetName3) {
                                                                    jstring jn3 = (jstring)env->CallObjectMethod((jobject)t3, midGetName3);
                                                                    std::string tn3 = jn3 ? jstr(env, jn3) : std::string();
                                                                    if (jn3) env->DeleteLocalRef(jn3);
                                                                    if (tn3 == "io.netty.channel.Channel") isChan = true;
                                                                }
                                                                if (isChan) {
                                                                    if (mSetAcc3) env->CallVoidMethod(f3, mSetAcc3, JNI_TRUE);
                                                                    fidCh = env->FromReflectedField(f3);
                                                                }
                                                                env->DeleteLocalRef(t3);
                                                                env->DeleteLocalRef(f3);
                                                            }
                                                            env->DeleteLocalRef(flds3);
                                                        } else env->ExceptionClear();
                                                    } else env->ExceptionClear();
                                                } else env->ExceptionClear();
                                            }
                                            if (fidCh) {
                                                jobject ch = env->GetObjectField(nm, fidCh);
                                                if (!env->ExceptionCheck() && ch) {
                                                    jclass cChan = env->GetObjectClass(ch);
                                                    jmethodID midWF = env->GetMethodID(cChan, "writeAndFlush", "(Ljava/lang/Object;)Lio/netty/channel/ChannelFuture;");
                                                    if (!midWF) { env->ExceptionClear(); }
                                                    if (midWF) {
                                                        env->CallObjectMethod(ch, midWF, pkt);
                                                        sentOK = !env->ExceptionCheck();
                                                        usedChannel = true;
                                                        if (!sentOK) { env->ExceptionDescribe(); env->ExceptionClear(); dlog("chat: Channel.writeAndFlush threw (mc->nh)"); }
                                                    }
                                                    env->DeleteLocalRef(cChan);
                                                    env->DeleteLocalRef(ch);
                                                } else env->ExceptionClear();
                                            }

                                            // Fallback to NetworkManager.sendPacket if Channel wasn't available or failed
                                            if (!sentOK) {
                                                jmethodID mSend2 = tryMethod(env, cNM, {
                                                    {"sendPacket",     "(Lnet/minecraft/network/Packet;)V"},
                                                    {"func_179290_a",  "(Lnet/minecraft/network/Packet;)V"}
                                                }, "chat:NM.send(mc->nh)");
                                                if (!mSend2) {
                                                    env->ExceptionClear();
                                                    mSend2 = tryMethod(env, cNM, {
                                                        {"a", "(Lff;)V"}
                                                    }, "chat:NM.send-obf(mc->nh)");
                                                }
                                                if (mSend2) {
                                                    env->CallVoidMethod(nm, mSend2, pkt);
                                                    sentOK = !env->ExceptionCheck();
                                                    if (!sentOK) { env->ExceptionDescribe(); env->ExceptionClear(); dlog("chat: NetworkManager.send threw (mc->nh)"); }
                                                }
                                            }

                                            if (jmsg) env->DeleteLocalRef(jmsg);
                                            env->DeleteLocalRef(pkt);
                                            env->DeleteLocalRef(cC01);
                                            env->DeleteLocalRef(cNM);
                                            env->DeleteLocalRef(nm);
                                            env->DeleteLocalRef(cNet);
                                            env->DeleteLocalRef(net);
                                            if (sentOK) {
                                                if (usedChannel) dlog("chat sent (Channel path via mc.getNetHandler)");
                                                else dlog("chat sent (NetworkManager path via mc.getNetHandler)");
                                                return true;
                                            }
                                        } else { env->ExceptionClear(); }
                                    }
                                    // cleanup if not already cleaned above
                                    if (jmsg) env->DeleteLocalRef(jmsg);
                                    env->DeleteLocalRef(pkt);
                                    env->DeleteLocalRef(cC01);
                                    env->DeleteLocalRef(cNet);
                                    env->DeleteLocalRef(net);
                                }
                            } else { env->ExceptionClear(); if (jmsg) env->DeleteLocalRef(jmsg); env->DeleteLocalRef(cC01); env->DeleteLocalRef(cNet); env->DeleteLocalRef(net); }
                        } else { env->ExceptionClear(); env->DeleteLocalRef(cC01); env->DeleteLocalRef(cNet); env->DeleteLocalRef(net); }
                    } else { dlog("chat: C01PacketChatMessage class not found"); env->DeleteLocalRef(cNet); env->DeleteLocalRef(net); }
                } else { env->ExceptionClear(); env->DeleteLocalRef(net); }
            }
        }
    }

    // Resolve thePlayer
    jfieldID fidPl = env->GetFieldID(clsMc, "thePlayer", "Lnet/minecraft/client/entity/EntityPlayerSP;");
    if (!fidPl) { env->ExceptionClear(); fidPl = env->GetFieldID(clsMc, "h", "Lbew;"); }
    if (!fidPl) { dlog("chat: player field not found"); return false; }

    jobject player = env->GetObjectField(mc, fidPl);
    if (!player || env->ExceptionCheck()) { env->ExceptionClear(); dlog("chat: player null"); return false; }

    // Try Badlion-safe packet path first: NetHandlerPlayClient.addToSendQueue(new C01PacketChatMessage(msg))
    do {
        jclass cPl = env->GetObjectClass(player);
        if (!cPl) { env->ExceptionClear(); break; }
        // sendQueue field (vanilla/obf variants seen elsewhere in this file)
        jfieldID fidN = env->GetFieldID(cPl, "sendQueue", "Lnet/minecraft/client/network/NetHandlerPlayClient;");
        if (!fidN) { env->ExceptionClear(); fidN = env->GetFieldID(cPl, "c", "Lbcy;"); }
        if (!fidN) { env->ExceptionClear(); fidN = env->GetFieldID(cPl, "b", "Lbcy;"); }
        if (!fidN) { env->ExceptionClear(); fidN = env->GetFieldID(cPl, "a", "Lbcy;"); }
        if (!fidN) {
            // Reflection fallback: scan declared fields for type NetHandlerPlayClient (or obf bcy)
            jclass cClass = env->FindClass("java/lang/Class");
            jclass cField = env->FindClass("java/lang/reflect/Field");
            if (cClass && cField) {
                jmethodID mGetFields = env->GetMethodID(cClass, "getDeclaredFields", "()[Ljava/lang/reflect/Field;");
                jmethodID mGetType   = env->GetMethodID(cField, "getType", "()Ljava/lang/Class;");
                jmethodID mSetAcc    = env->GetMethodID(cField, "setAccessible", "(Z)V");
                jmethodID mGetName   = env->GetMethodID(cClass, "getName", "()Ljava/lang/String;");
                if (mGetFields && mGetType && mGetName) {
                    jobjectArray flds = (jobjectArray)env->CallObjectMethod(cPl, mGetFields);
                    if (!env->ExceptionCheck() && flds) {
                        jsize fn = env->GetArrayLength(flds);
                        for (jsize i = 0; i < fn && !fidN; ++i) {
                            jobject f = env->GetObjectArrayElement(flds, i);
                            if (!f) continue;
                            jobject t = env->CallObjectMethod(f, mGetType);
                            if (env->ExceptionCheck() || !t) { env->ExceptionClear(); env->DeleteLocalRef(f); continue; }
                            jstring jn = (jstring)env->CallObjectMethod(t, mGetName);
                            std::string tn = jn ? jstr(env, jn) : std::string();
                            if (jn) env->DeleteLocalRef(jn);
                            if (tn == "net.minecraft.client.network.NetHandlerPlayClient" || tn == "bcy") {
                                if (mSetAcc) env->CallVoidMethod(f, mSetAcc, JNI_TRUE);
                                fidN = env->FromReflectedField(f);
                            }
                            env->DeleteLocalRef(t);
                            env->DeleteLocalRef(f);
                        }
                        env->DeleteLocalRef(flds);
                    } else env->ExceptionClear();
                } else env->ExceptionClear();
            } else env->ExceptionClear();
        }
        if (!fidN) { dlog("chat: net handler field not found (even reflected)"); break; }
        jobject net = env->GetObjectField(player, fidN);
        if (!net || env->ExceptionCheck()) { env->ExceptionClear(); dlog("chat: net is null"); break; }

        jclass cNet = env->GetObjectClass(net);
        if (!cNet) { env->ExceptionClear(); env->DeleteLocalRef(net); break; }

        // Resolve C01PacketChatMessage using NetHandler's ClassLoader to avoid loader mismatch
        jclass cClass = env->FindClass("java/lang/Class");
        jmethodID midGetCL = cClass ? env->GetMethodID(cClass, "getClassLoader", "()Ljava/lang/ClassLoader;") : nullptr;
        jobject cl = midGetCL ? env->CallObjectMethod(cNet, midGetCL) : nullptr;
        if (env->ExceptionCheck()) { env->ExceptionClear(); cl = nullptr; }

        jclass cC01 = nullptr;
        if (cl) {
            cC01 = forName(env, "net.minecraft.network.play.client.C01PacketChatMessage", cl);
            if (!cC01) cC01 = forName(env, "C01PacketChatMessage", cl);
            if (!cC01) cC01 = forName(env, "ih", cl);
        } else {
            cC01 = env->FindClass("net/minecraft/network/play/client/C01PacketChatMessage");
            if (!cC01) { env->ExceptionClear(); cC01 = env->FindClass("C01PacketChatMessage"); }
            if (!cC01) { env->ExceptionClear(); cC01 = env->FindClass("ih"); }
        }
        if (!cC01) { dlog("chat: C01PacketChatMessage class not found"); env->DeleteLocalRef(cNet); env->DeleteLocalRef(net); break; }

        jstring jmsg = env->NewStringUTF(msg.c_str());
        jobject pkt = nullptr;
        {
            jmethodID ctor = env->GetMethodID(cC01, "<init>", "(Ljava/lang/String;)V");
            if (ctor) {
                pkt = env->NewObject(cC01, ctor, jmsg);
                if (env->ExceptionCheck()) { env->ExceptionClear(); pkt = nullptr; }
            } else {
                env->ExceptionClear();
                dlog("chat: C01 ctor not found; trying Unsafe fallback");
            }
            if (!pkt) {
                pkt = make_chat_packet(env, cC01, jmsg);
            }
        }
        if (!pkt) {
            if (jmsg) env->DeleteLocalRef(jmsg);
            env->DeleteLocalRef(cC01);
            env->DeleteLocalRef(cNet);
            env->DeleteLocalRef(net);
            dlog("chat: could not build C01 packet");
            break;
        }

        // Try several method+signature combos for addToSendQueue
        jmethodID mSend = tryMethod(env, cNet, {
            {"addToSendQueue", "(Lnet/minecraft/network/Packet;)V"},
            {"sendPacket",     "(Lnet/minecraft/network/Packet;)V"}
        }, "chat:sendPkt");
        if (!mSend) {
            env->ExceptionClear();
            mSend = tryMethod(env, cNet, {
                {"a", "(Lff;)V"}
            }, "chat:sendPkt-obf");
        }

        // Fallback: Badlion may expose NetworkManager instead; try NetHandler.netManager.sendPacket(pkt)
        if (!mSend) {
            // Try common field names first
            jfieldID fidNM = env->GetFieldID(cNet, "netManager", "Lnet/minecraft/network/NetworkManager;");
            if (!fidNM) { env->ExceptionClear(); fidNM = env->GetFieldID(cNet, "field_147302_e", "Lnet/minecraft/network/NetworkManager;"); }
            // Reflection fallback: find a field whose type has a sendPacket(Packet) method
            if (!fidNM) {
                jclass cClass = env->FindClass("java/lang/Class");
                jclass cField = env->FindClass("java/lang/reflect/Field");
                if (cClass && cField) {
                    jmethodID mGetFields = env->GetMethodID(cClass, "getDeclaredFields", "()[Ljava/lang/reflect/Field;");
                    jmethodID mGetType   = env->GetMethodID(cField, "getType",           "()Ljava/lang/Class;");
                    jmethodID mSetAcc    = env->GetMethodID(cField, "setAccessible",     "(Z)V");
                    if (mGetFields && mGetType) {
                        jobjectArray flds = (jobjectArray)env->CallObjectMethod(cNet, mGetFields);
                        if (!env->ExceptionCheck() && flds) {
                            jsize fn = env->GetArrayLength(flds);
                            for (jsize i = 0; i < fn && !fidNM; ++i) {
                                jobject f = env->GetObjectArrayElement(flds, i);
                                if (!f) continue;
                                jobject t = env->CallObjectMethod(f, mGetType);
                                if (env->ExceptionCheck() || !t) { env->ExceptionClear(); env->DeleteLocalRef(f); continue; }
                                jclass cType = (jclass)t;
                                // Look for sendPacket/func_179290_a/a(Lff;)V on this type (use cType's loader to resolve signatures)
                                jmethodID mTry = env->GetMethodID(cType, "sendPacket", "(Lnet/minecraft/network/Packet;)V");
                                if (!mTry) { env->ExceptionClear(); mTry = env->GetMethodID(cType, "func_179290_a", "(Lnet/minecraft/network/Packet;)V"); }
                                if (!mTry) { env->ExceptionClear(); mTry = env->GetMethodID(cType, "a", "(Lff;)V"); }
                                if (mTry) {
                                    if (mSetAcc) env->CallVoidMethod(f, mSetAcc, JNI_TRUE);
                                    fidNM = env->FromReflectedField(f);
                                }
                                env->DeleteLocalRef(t);
                                env->DeleteLocalRef(f);
                            }
                            env->DeleteLocalRef(flds);
                        } else env->ExceptionClear();
                    } else env->ExceptionClear();
                } else env->ExceptionClear();
            }

            if (fidNM) {
                jobject nm = env->GetObjectField(net, fidNM);
                if (!env->ExceptionCheck() && nm) {
                    jclass cNM = env->GetObjectClass(nm);
                    bool sentOK = false;
                    bool usedChannel = false;

                    // Prefer Channel.writeAndFlush(pkt) first to leverage Netty's event loop scheduling
                    jfieldID fidCh = env->GetFieldID(cNM, "channel", "Lio/netty/channel/Channel;");
                    if (!fidCh) { env->ExceptionClear(); fidCh = env->GetFieldID(cNM, "field_150746_k", "Lio/netty/channel/Channel;"); }
                    if (!fidCh) {
                        // reflection scan for Channel-typed field (compare type name to avoid loader issues)
                        jclass cClass3 = env->FindClass("java/lang/Class");
                        jclass cField3 = env->FindClass("java/lang/reflect/Field");
                        if (cClass3 && cField3) {
                            jmethodID mGetFields3 = env->GetMethodID(cClass3, "getDeclaredFields", "()[Ljava/lang/reflect/Field;");
                            jmethodID mGetType3   = env->GetMethodID(cField3, "getType",           "()Ljava/lang/Class;");
                            jmethodID mSetAcc3    = env->GetMethodID(cField3, "setAccessible",     "(Z)V");
                            jclass cChan = env->FindClass("io/netty/channel/Channel");
                            if (mGetFields3 && mGetType3 && cChan) {
                                jobjectArray flds3 = (jobjectArray)env->CallObjectMethod(cNM, mGetFields3);
                                if (!env->ExceptionCheck() && flds3) {
                                    jsize fn3 = env->GetArrayLength(flds3);
                                    for (jsize i3=0; i3<fn3 && !fidCh; ++i3) {
                                        jobject f3 = env->GetObjectArrayElement(flds3, i3);
                                        if (!f3) continue;
                                        jobject t3 = env->CallObjectMethod(f3, mGetType3);
                                        if (env->ExceptionCheck() || !t3) { env->ExceptionClear(); env->DeleteLocalRef(f3); continue; }
                                        jclass cCls = env->FindClass("java/lang/Class");
                                        jmethodID midGetName3 = cCls ? env->GetMethodID(cCls, "getName", "()Ljava/lang/String;") : nullptr;
                                        bool isChan = false;
                                        if (midGetName3) {
                                            jstring jn3 = (jstring)env->CallObjectMethod((jobject)t3, midGetName3);
                                            std::string tn3 = jn3 ? jstr(env, jn3) : std::string();
                                            if (jn3) env->DeleteLocalRef(jn3);
                                            if (tn3 == "io.netty.channel.Channel") isChan = true;
                                        }
                                        if (isChan) {
                                            if (mSetAcc3) env->CallVoidMethod(f3, mSetAcc3, JNI_TRUE);
                                            fidCh = env->FromReflectedField(f3);
                                        }
                                        env->DeleteLocalRef(t3);
                                        env->DeleteLocalRef(f3);
                                    }
                                    env->DeleteLocalRef(flds3);
                                } else env->ExceptionClear();
                            } else env->ExceptionClear();
                        } else env->ExceptionClear();
                    }
                    if (fidCh) {
                        jobject ch = env->GetObjectField(nm, fidCh);
                        if (!env->ExceptionCheck() && ch) {
                            jclass cChan = env->GetObjectClass(ch);
                            jmethodID midWF = env->GetMethodID(cChan, "writeAndFlush", "(Ljava/lang/Object;)Lio/netty/channel/ChannelFuture;");
                            if (!midWF) { env->ExceptionClear(); }
                            if (midWF) {
                                env->CallObjectMethod(ch, midWF, pkt);
                                sentOK = !env->ExceptionCheck();
                                usedChannel = true;
                                if (!sentOK) { env->ExceptionDescribe(); env->ExceptionClear(); dlog("chat: Channel.writeAndFlush threw"); }
                            }
                            env->DeleteLocalRef(cChan);
                            env->DeleteLocalRef(ch);
                        } else env->ExceptionClear();
                    }

                    // Fallback to NetworkManager.sendPacket
                    if (!sentOK) {
                        jmethodID mSend2 = tryMethod(env, cNM, {
                            {"sendPacket",     "(Lnet/minecraft/network/Packet;)V"},
                            {"func_179290_a",  "(Lnet/minecraft/network/Packet;)V"}
                        }, "chat:NM.send");
                        if (!mSend2) {
                            env->ExceptionClear();
                            mSend2 = tryMethod(env, cNM, {
                                {"a", "(Lff;)V"}
                            }, "chat:NM.send-obf");
                        }
                        if (mSend2) {
                            env->CallVoidMethod(nm, mSend2, pkt);
                            sentOK = !env->ExceptionCheck();
                            if (!sentOK) { env->ExceptionDescribe(); env->ExceptionClear(); dlog("chat: NetworkManager.send threw"); }
                        }
                    }

                    if (jmsg) env->DeleteLocalRef(jmsg);
                    env->DeleteLocalRef(pkt);
                    env->DeleteLocalRef(cC01);
                    env->DeleteLocalRef(cNM);
                    env->DeleteLocalRef(nm);
                    env->DeleteLocalRef(cNet);
                    env->DeleteLocalRef(net);
                    if (sentOK) { dlog(usedChannel ? "chat sent (Channel path)" : "chat sent (NetworkManager path)"); env->DeleteLocalRef(cPl); return true; }
                } else { env->ExceptionClear(); }
            }

            // No send path found
            if (jmsg) env->DeleteLocalRef(jmsg);
            env->DeleteLocalRef(pkt);
            env->DeleteLocalRef(cC01);
            env->DeleteLocalRef(cNet);
            env->DeleteLocalRef(net);
            dlog("chat: NetHandler/NetworkManager send methods not found");
            break;
        }

        env->CallVoidMethod(net, mSend, pkt);
        bool ok = !env->ExceptionCheck();
        if (!ok) {
            env->ExceptionDescribe();
            env->ExceptionClear();
            dlog("chat: addToSendQueue threw");
        }

        if (jmsg) env->DeleteLocalRef(jmsg);
        env->DeleteLocalRef(pkt);
        env->DeleteLocalRef(cC01);
        env->DeleteLocalRef(cNet);
        env->DeleteLocalRef(net);

        if (ok) { dlog("chat sent (packet path)"); env->DeleteLocalRef(cPl); return true; }
        env->DeleteLocalRef(cPl);
    } while (0);

    // Fallback: call EntityPlayerSP.sendChatMessage directly (vanilla/Lunar etc.)
    {
        jclass clsPl = env->GetObjectClass(player);
        if (!clsPl) { env->ExceptionClear(); dlog("chat: no player class"); return false; }
        jmethodID midChat = env->GetMethodID(clsPl, "sendChatMessage", "(Ljava/lang/String;)V");
        if (!midChat) { env->ExceptionClear(); midChat = env->GetMethodID(clsPl, "a", "(Ljava/lang/String;)V"); }
        if (!midChat) { dlog("chat: sendChatMessage not found"); env->DeleteLocalRef(clsPl); return false; }
        jstring jmsg = env->NewStringUTF(msg.c_str());
        env->CallVoidMethod(player, midChat, jmsg);
        if (env->ExceptionCheck()) { env->ExceptionDescribe(); env->ExceptionClear(); env->DeleteLocalRef(jmsg); env->DeleteLocalRef(clsPl); dlog("chat: direct path failed"); return false; }
        env->DeleteLocalRef(jmsg);
        env->DeleteLocalRef(clsPl);
        dlog("chat sent (direct path)");
        return true;
    }
}

static jmethodID try_get_method(JNIEnv* env, jclass cls,
                                const char* const names[], size_t n,
                                const char* sig, const char* tag) {
    for (size_t i = 0; i < n; ++i) {
        jmethodID m = env->GetMethodID(cls, names[i], sig);
        if (m) { dlog("%s: resolved '%s' %s", tag, names[i], sig); return m; }
        env->ExceptionClear();
    }
    return nullptr;
}
static std::string class_name_of(JNIEnv* env, jobject obj) {
    if (!obj) return {};
    jclass c = env->GetObjectClass(obj);
    jclass k = env->FindClass("java/lang/Class"); if (!k){ env->ExceptionClear(); return {}; }
    jmethodID mid = env->GetMethodID(k, "getName", "()Ljava/lang/String;"); if (!mid){ env->ExceptionClear(); return {}; }
    jstring js = (jstring)env->CallObjectMethod(c, mid);
    if (env->ExceptionCheck() || !js) { env->ExceptionClear(); return {}; }
    std::string s = jstr(env, js); env->DeleteLocalRef(js); return s;
}

static void dump_score(JNIEnv* env, SOCKET c) {
    dlog("score: begin");

    jclass clsMc = find_mc_class(env);
    if (!clsMc) { dlog("score: no mc class"); send_line(c, "score: no mc", true); return; }
    jobject mc = get_minecraft_singleton(env);
    if (!mc)   { dlog("score: no mc instance"); send_line(c, "score: no mc", true); return; }

    jclass cMcObj = env->GetObjectClass(mc);
    jfieldID fidWorld = env->GetFieldID(cMcObj, "theWorld", "Lnet/minecraft/client/multiplayer/WorldClient;");
    if (!fidWorld) { env->ExceptionClear(); fidWorld = env->GetFieldID(cMcObj, "f", "Lbdb;"); }
    jobject world = fidWorld ? env->GetObjectField(mc, fidWorld) : nullptr;
    if (!world || env->ExceptionCheck()) { env->ExceptionClear(); dlog("score: world null"); send_line(c, "score: no world", true); return; }

    jclass cWorld = env->GetObjectClass(world);
    jmethodID midSB = env->GetMethodID(cWorld, "getScoreboard", "()Lnet/minecraft/scoreboard/Scoreboard;");
    if (!midSB) { env->ExceptionClear(); midSB = env->GetMethodID(cWorld, "aA", "()Lbfd;"); }
    jobject scoreboard = midSB ? env->CallObjectMethod(world, midSB) : nullptr;
    if (!scoreboard || env->ExceptionCheck()) { env->ExceptionClear(); dlog("score: no scoreboard"); send_line(c, "score: no scoreboard", true); return; }
    jclass cSB = env->GetObjectClass(scoreboard);

    // sidebar objective
    jmethodID midObj = env->GetMethodID(cSB, "getObjectiveInDisplaySlot", "(I)Lnet/minecraft/scoreboard/ScoreObjective;");
    if (!midObj) { env->ExceptionClear(); midObj = env->GetMethodID(cSB, "a", "(I)Lbfc;"); }
    jobject objective = midObj ? env->CallObjectMethod(scoreboard, midObj, 1) : nullptr;
    if (!objective || env->ExceptionCheck()) { env->ExceptionClear(); send_line(c, "score: no sidebar objective", true); return; }

    // title
    jclass cObj = env->GetObjectClass(objective);
    jmethodID midTitle = env->GetMethodID(cObj, "getDisplayName", "()Ljava/lang/String;");
    if (!midTitle) { env->ExceptionClear(); midTitle = env->GetMethodID(cObj, "d", "()Ljava/lang/String;"); }
    jstring jtitle = (jstring)env->CallObjectMethod(objective, midTitle);
    std::string title = strip_mc_codes(jstr(env, jtitle));
    if (jtitle) env->DeleteLocalRef(jtitle);
    {
        jclass cObj = env->GetObjectClass(objective);
        jmethodID midTitleICC = env->GetMethodID(cObj, "getDisplayName", "()Lnet/minecraft/util/IChatComponent;");
        if (!midTitleICC) { env->ExceptionClear(); midTitleICC = env->GetMethodID(cObj, "d", "()Lfj;"); }
        if (midTitleICC) {
            jobject icc = env->CallObjectMethod(objective, midTitleICC);
            if (!env->ExceptionCheck() && icc) {
                jclass cICC = env->GetObjectClass(icc);
                jmethodID midUnf = env->GetMethodID(cICC, "getUnformattedText", "()Ljava/lang/String;");
                if (!midUnf) { env->ExceptionClear(); midUnf = env->GetMethodID(cICC, "e", "()Ljava/lang/String;"); }
                if (midUnf) {
                    jstring js = (jstring)env->CallObjectMethod(icc, midUnf);
                    if (!env->ExceptionCheck() && js) title = strip_mc_codes(jstr(env, js));
                    else env->ExceptionClear();
                    if (js) env->DeleteLocalRef(js);
                } else env->ExceptionClear();
                env->DeleteLocalRef(cICC);
            } else env->ExceptionClear();
            if (icc) env->DeleteLocalRef(icc);
        } else env->ExceptionClear();
    }

    // scores -> array
    jmethodID midGet = env->GetMethodID(cSB, "getSortedScores", "(Lnet/minecraft/scoreboard/ScoreObjective;)Ljava/util/Collection;");
    if (!midGet) { env->ExceptionClear(); midGet = env->GetMethodID(cSB, "b", "(Lbfc;)Ljava/util/Collection;"); }
    jobject coll = midGet ? env->CallObjectMethod(scoreboard, midGet, objective) : nullptr;
    if (!coll || env->ExceptionCheck()) { env->ExceptionClear(); send_line(c, "score: empty", true); return; }

    jclass cCol = env->FindClass("java/util/Collection");
    jclass cObjCls = env->FindClass("java/lang/Object");
    jmethodID midToArr = env->GetMethodID(cCol, "toArray", "([Ljava/lang/Object;)[Ljava/lang/Object;");
    if (!midToArr) { env->ExceptionClear(); }
    jobjectArray arr = nullptr;
    if (midToArr && cObjCls) {
        jobjectArray empty = env->NewObjectArray(0, cObjCls, nullptr);
        arr = (jobjectArray)env->CallObjectMethod(coll, midToArr, empty);
        env->DeleteLocalRef(empty);
        if (env->ExceptionCheck()) { env->ExceptionDescribe(); env->ExceptionClear(); arr = nullptr; }
    }
    if (!arr) { send_line(c, "score: empty", true); return; }
    jsize n = env->GetArrayLength(arr);

    // Score class
    jclass cScore = nullptr;
    if (n > 0) { jobject t = env->GetObjectArrayElement(arr, 0); if (t){ cScore = env->GetObjectClass(t); env->DeleteLocalRef(t);} }
    if (!cScore) { env->ExceptionClear(); cScore = env->FindClass("net/minecraft/scoreboard/Score"); }
    if (!cScore) { env->ExceptionClear(); cScore = env->FindClass("bfg"); }
    jmethodID midName = cScore ? env->GetMethodID(cScore, "getPlayerName", "()Ljava/lang/String;") : nullptr;
    if (!midName) { env->ExceptionClear(); midName = env->GetMethodID(cScore, "e", "()Ljava/lang/String;"); }
    jmethodID midPts  = cScore ? env->GetMethodID(cScore, "getScorePoints", "()I") : nullptr;
    if (!midPts)  { env->ExceptionClear(); midPts  = env->GetMethodID(cScore, "c", "()I"); }

    // Scoreboard team APIs
    jmethodID midGetPlayersTeam = env->GetMethodID(cSB, "getPlayersTeam", "(Ljava/lang/String;)Lnet/minecraft/scoreboard/ScorePlayerTeam;");
    if (!midGetPlayersTeam) { env->ExceptionClear(); midGetPlayersTeam = env->GetMethodID(cSB, "h", "(Ljava/lang/String;)Lbfe;"); }
    jmethodID midGetTeam = env->GetMethodID(cSB, "getTeam", "(Ljava/lang/String;)Lnet/minecraft/scoreboard/ScorePlayerTeam;");
    if (!midGetTeam) { env->ExceptionClear(); midGetTeam = env->GetMethodID(cSB, "e", "(Ljava/lang/String;)Lbfe;"); }
    jmethodID midGetTeams = env->GetMethodID(cSB, "getTeams", "()Ljava/util/Collection;");
    if (!midGetTeams) { env->ExceptionClear(); midGetTeams = env->GetMethodID(cSB, "g", "()Ljava/util/Collection;"); }
    if (!midGetTeams) { env->ExceptionClear(); midGetTeams = env->GetMethodID(cSB, "i", "()Ljava/util/Collection;"); }

    // for membership.contains
    jclass cSPT = env->FindClass("net/minecraft/scoreboard/ScorePlayerTeam"); if (!cSPT) { env->ExceptionClear(); cSPT = env->FindClass("bfe"); }
    jclass cCollection = env->FindClass("java/util/Collection");
    jmethodID midMembers = nullptr, midContains = nullptr;
    if (cSPT) {
        midMembers = env->GetMethodID(cSPT, "getMembershipCollection", "()Ljava/util/Collection;");
        if (!midMembers){ env->ExceptionClear(); midMembers = env->GetMethodID(cSPT, "getMembership", "()Ljava/util/Collection;"); }
        if (!midMembers){ env->ExceptionClear(); midMembers = env->GetMethodID(cSPT, "func_96670_d", "()Ljava/util/Collection;"); }
        if (!midMembers){ env->ExceptionClear(); midMembers = env->GetMethodID(cSPT, "d", "()Ljava/util/Collection;"); }
    }
    if (cCollection) midContains = env->GetMethodID(cCollection, "contains", "(Ljava/lang/Object;)Z");

    auto class_name = [&](jobject obj)->std::string{
        if (!obj) return {};
        jclass cObj = env->GetObjectClass(obj);
        jclass cClass = env->FindClass("java/lang/Class");
        jmethodID midGN = env->GetMethodID(cClass, "getName", "()Ljava/lang/String;");
        std::string r;
        if (midGN) {
            jstring jn = (jstring)env->CallObjectMethod(cObj, midGN);
            if (!env->ExceptionCheck() && jn) r = jstr(env, jn);
            else env->ExceptionClear();
            if (jn) env->DeleteLocalRef(jn);
        } else env->ExceptionClear();
        return r;
    };

    std::string out; char head[128];
    _snprintf_s(head, _TRUNCATE, "score: %s (%d)\n", title.c_str(), (int)n);
    out += head;

    for (jsize i=0; i<n; ++i) {
        jobject sc = env->GetObjectArrayElement(arr, i);
        if (!sc) continue;

        jstring jname = (jstring)env->CallObjectMethod(sc, midName);
        if (env->ExceptionCheck()) { env->ExceptionClear(); if (sc) env->DeleteLocalRef(sc); continue; }
        std::string entryLog = jname ? jstr(env, jname) : std::string();
        std::string entryVis = strip_mc_codes(entryLog);
        bool entryInvisible = (entryVis.empty() || (entryVis.size() <= 2 && (unsigned char)entryVis[0] >= 0x80));

        jobject team = nullptr;
        if (midGetPlayersTeam) {
            team = env->CallObjectMethod(scoreboard, midGetPlayersTeam, jname);
            if (env->ExceptionCheck()) { env->ExceptionClear(); team = nullptr; }
        }
        if (!team && midGetTeam) {
            team = env->CallObjectMethod(scoreboard, midGetTeam, jname);
            if (env->ExceptionCheck()) { env->ExceptionClear(); team = nullptr; }
        }
        if (!team && midGetTeams && midMembers && midContains) {
            jobject tColl = env->CallObjectMethod(scoreboard, midGetTeams);
            if (!env->ExceptionCheck() && tColl) {
                jobjectArray tArr = nullptr;
                if (midToArr && cObjCls) {
                    jobjectArray empty = env->NewObjectArray(0, cObjCls, nullptr);
                    tArr = (jobjectArray)env->CallObjectMethod(tColl, midToArr, empty);
                    env->DeleteLocalRef(empty);
                    if (env->ExceptionCheck()) { env->ExceptionClear(); tArr = nullptr; }
                }
                if (tArr) {
                    jsize tn = env->GetArrayLength(tArr);
                    for (jsize ti=0; ti<tn && !team; ++ti) {
                        jobject t = env->GetObjectArrayElement(tArr, ti);
                        if (!t) continue;
                        jobject mem = env->CallObjectMethod(t, midMembers);
                        if (!env->ExceptionCheck() && mem) {
                            jboolean has = env->CallBooleanMethod(mem, midContains, jname);
                            if (!env->ExceptionCheck() && has == JNI_TRUE) team = t;
                            if (!team) env->DeleteLocalRef(t);
                            env->DeleteLocalRef(mem);
                        } else { env->ExceptionClear(); env->DeleteLocalRef(t); }
                    }
                    env->DeleteLocalRef(tArr);
                }
                env->DeleteLocalRef(tColl);
            } else env->ExceptionClear();
        }

        std::string line;
        if (team) {
            jclass clsT = env->GetObjectClass(team);
            jmethodID midFmtDyn = nullptr;
            midFmtDyn = env->GetMethodID(clsT, "formatString", "(Ljava/lang/String;)Ljava/lang/String;");
            if (!midFmtDyn){ env->ExceptionClear(); midFmtDyn = env->GetMethodID(clsT, "func_96667_a", "(Ljava/lang/String;)Ljava/lang/String;"); }
            if (!midFmtDyn){ env->ExceptionClear(); midFmtDyn = env->GetMethodID(clsT, "a", "(Ljava/lang/String;)Ljava/lang/String;"); }
            if (midFmtDyn) {
                jstring js = (jstring)env->CallObjectMethod(team, midFmtDyn, jname);
                if (!env->ExceptionCheck() && js) { line = jstr(env, js); env->DeleteLocalRef(js); }
                else env->ExceptionClear();
            }
            if (line.empty()) {
                auto getStrDyn = [&](const char* n1,const char* n2,const char* n3,const char* n4)->std::string{
                    jmethodID m = env->GetMethodID(clsT, n1, "()Ljava/lang/String;");
                    if (!m) { env->ExceptionClear(); m = env->GetMethodID(clsT, n2, "()Ljava/lang/String;"); }
                    if (!m) { env->ExceptionClear(); m = env->GetMethodID(clsT, n3, "()Ljava/lang/String;"); }
                    if (!m) { env->ExceptionClear(); m = env->GetMethodID(clsT, n4, "()Ljava/lang/String;"); }
                    if (!m) return {};
                    jstring s = (jstring)env->CallObjectMethod(team, m);
                    if (env->ExceptionCheck()) { env->ExceptionClear(); return {}; }
                    std::string r = jstr(env, s);
                    if (s) env->DeleteLocalRef(s);
                    return r;
                };
                std::string pre = getStrDyn("getColorPrefix","getPrefix","func_96661_b","c");
                std::string suf = getStrDyn("getColorSuffix","getSuffix","func_96663_c","d");
                if (!entryInvisible) line = pre + entryVis + suf;
                else                 line = pre + suf;
            }
        } else {
            dlog("score: no team for entry '%s'", entryLog.c_str());
            line = entryVis;
        }

        int pts = 0;
        if (midPts) { pts = (int)env->CallIntMethod(sc, midPts); if (env->ExceptionCheck()) { env->ExceptionClear(); pts = 0; } }

        if (sc) env->DeleteLocalRef(sc);
        if (jname) env->DeleteLocalRef(jname);

        line = strip_mc_codes(line);
        char row[1024];
        _snprintf_s(row, _TRUNCATE, "%s : %d\n", line.c_str(), pts);
        out += row;
    }

    send_line(c, out.c_str(), true);
    dlog("score: dump_score -> %zu bytes (+NUL)", out.size()+2);
}

static bool dump_tab(JNIEnv* env, SOCKET c) {
    jobject mc = get_minecraft_singleton(env);
    if (!env || !mc) { dlog("tab: bad args"); return false; }
    if (!env || !mc) return false;
    std::string out; char head[128];

    // mc.thePlayer -> sendQueue (NetHandlerPlayClient)
    jclass clsMc = env->GetObjectClass(mc);
    jfieldID fidPl = env->GetFieldID(clsMc,
        "thePlayer", "Lnet/minecraft/client/entity/EntityPlayerSP;");
    if (!fidPl) { env->ExceptionClear(); fidPl = env->GetFieldID(clsMc, "h", "Lbew;"); }
    if (!fidPl) { dlog("tab: no player"); return false; }

    jobject player = env->GetObjectField(mc, fidPl);
    if (!player || env->ExceptionCheck()) { env->ExceptionClear(); dlog("tab: player null"); return false; }

    jclass cPl = env->GetObjectClass(player);
    jfieldID fidN = env->GetFieldID(cPl,
        "sendQueue", "Lnet/minecraft/client/network/NetHandlerPlayClient;");
    if (!fidN) { env->ExceptionClear(); fidN = env->GetFieldID(cPl, "c", "Lbcy;"); }
    if (!fidN) { env->ExceptionClear(); fidN = env->GetFieldID(cPl, "b", "Lbcy;"); }
    if (!fidN) { env->ExceptionClear(); fidN = env->GetFieldID(cPl, "a", "Lbcy;"); }
    if (!fidN) {
        // Reflection fallback: scan declared fields for type NetHandlerPlayClient (or obf bcy)
        jclass cClass = env->FindClass("java/lang/Class");
        jclass cField = env->FindClass("java/lang/reflect/Field");
        if (cClass && cField) {
            jmethodID mGetFields = env->GetMethodID(cClass, "getDeclaredFields", "()[Ljava/lang/reflect/Field;");
            jmethodID mGetType   = env->GetMethodID(cField, "getType", "()Ljava/lang/Class;");
            jmethodID mSetAcc    = env->GetMethodID(cField, "setAccessible", "(Z)V");
            jmethodID mGetName   = env->GetMethodID(cClass, "getName", "()Ljava/lang/String;");
            if (mGetFields && mGetType && mGetName) {
                jobjectArray flds = (jobjectArray)env->CallObjectMethod(cPl, mGetFields);
                if (!env->ExceptionCheck() && flds) {
                    jsize fn = env->GetArrayLength(flds);
                    for (jsize i = 0; i < fn && !fidN; ++i) {
                        jobject f = env->GetObjectArrayElement(flds, i);
                        if (!f) continue;
                        jobject t = env->CallObjectMethod(f, mGetType);
                        if (env->ExceptionCheck() || !t) { env->ExceptionClear(); env->DeleteLocalRef(f); continue; }
                        jstring jn = (jstring)env->CallObjectMethod(t, mGetName);
                        std::string tn = jn ? jstr(env, jn) : std::string();
                        if (jn) env->DeleteLocalRef(jn);
                        if (tn == "net.minecraft.client.network.NetHandlerPlayClient" || tn == "bcy") {
                            if (mSetAcc) env->CallVoidMethod(f, mSetAcc, JNI_TRUE);
                            fidN = env->FromReflectedField(f);
                        }
                        env->DeleteLocalRef(t);
                        env->DeleteLocalRef(f);
                    }
                    env->DeleteLocalRef(flds);
                } else env->ExceptionClear();
            } else env->ExceptionClear();
        } else env->ExceptionClear();
    }
    if (!fidN) { dlog("tab: net handler not found"); return false; }

    jobject net = env->GetObjectField(player, fidN);
    dlog("tab: net=%p", net);
    if (!net || env->ExceptionCheck()) { env->ExceptionClear(); dlog("tab: net null"); return false; }

    // Try: getPlayerInfoMap() -> Collection
    jclass cNet = env->GetObjectClass(net);
    jmethodID midList = env->GetMethodID(cNet, "getPlayerInfoMap", "()Ljava/util/Collection;");
    if (!midList) { env->ExceptionClear();
        // Fallback: scan any zero-arg method returning Collection
        jclass cClass = env->FindClass("java/lang/Class");
        jclass cMethod = env->FindClass("java/lang/reflect/Method");
        jclass cColl = env->FindClass("java/util/Collection");
        jmethodID m_getMethods = env->GetMethodID(cClass, "getMethods", "()[Ljava/lang/reflect/Method;");
        jmethodID m_getReturnType = env->GetMethodID(cMethod, "getReturnType", "()Ljava/lang/Class;");
        jmethodID m_getParamTypes = env->GetMethodID(cMethod, "getParameterTypes", "()[Ljava/lang/Class;");
        jmethodID m_isAssignableFrom = env->GetMethodID(cClass, "isAssignableFrom", "(Ljava/lang/Class;)Z");
        jobjectArray arr = (jobjectArray)env->CallObjectMethod(cNet, m_getMethods);
        if (!env->ExceptionCheck() && arr) {
            jsize n = env->GetArrayLength(arr);
            for (jsize i = 0; i < n && !midList; ++i) {
                jobject m = env->GetObjectArrayElement(arr, i);
                jobject rt = env->CallObjectMethod(m, m_getReturnType);
                jboolean isColl = env->CallBooleanMethod(cColl, m_isAssignableFrom, rt);
                jobjectArray pt = (jobjectArray)env->CallObjectMethod(m, m_getParamTypes);
                jsize pc = env->GetArrayLength(pt);
                if (isColl && pc == 0) {
                    midList = env->FromReflectedMethod(m);
                }
                env->DeleteLocalRef(pt);
                env->DeleteLocalRef(rt);
                env->DeleteLocalRef(m);
            }
            env->DeleteLocalRef(arr);
        } else env->ExceptionClear();
    }
    if (!midList) { dlog("tab: list method not found"); return false; }

    jobject coll = env->CallObjectMethod(net, midList);
    dlog("tab: coll=%p", coll);
    if (env->ExceptionCheck() || !coll) { env->ExceptionClear(); dlog("tab: list call failed"); return false; }

    // coll.toArray()
    jclass cColl = env->FindClass("java/util/Collection");
    jmethodID m_toArray = env->GetMethodID(cColl, "toArray", "()[Ljava/lang/Object;");
    jobjectArray arr = (jobjectArray)env->CallObjectMethod(coll, m_toArray);
    if (env->ExceptionCheck() || !arr) { env->ExceptionClear(); dlog("tab: toArray failed"); return false; }

    jsize n = env->GetArrayLength(arr);
    out.reserve(128 + n * 24);
    out += "tab: "; out += std::to_string((int)n); out.push_back((char)10);

    for (jsize i = 0; i < n; ++i) {
        jobject npi = env->GetObjectArrayElement(arr, i);
        if (!npi) continue;
        jclass cNpi = env->GetObjectClass(npi);
        // NetworkPlayerInfo.getGameProfile()
        jmethodID m_gp = env->GetMethodID(cNpi, "getGameProfile", "()Lcom/mojang/authlib/GameProfile;");
        if (!m_gp) { env->ExceptionClear();
            // fallback: scan zero-arg returning GameProfile
            jclass cClass = env->FindClass("java/lang/Class");
            jclass cMethod = env->FindClass("java/lang/reflect/Method");
            jclass cGP = env->FindClass("com/mojang/authlib/GameProfile");
            jmethodID m_getMethods = env->GetMethodID(cClass, "getMethods", "()[Ljava/lang/reflect/Method;");
            jmethodID m_getReturnType = env->GetMethodID(cMethod, "getReturnType", "()Ljava/lang/Class;");
            jmethodID m_getParamTypes = env->GetMethodID(cMethod, "getParameterTypes", "()[Ljava/lang/Class;");
            jmethodID m_isAssignableFrom = env->GetMethodID(cClass, "isAssignableFrom", "(Ljava/lang/Class;)Z");
            jobjectArray marr = (jobjectArray)env->CallObjectMethod(cNpi, m_getMethods);
            if (!env->ExceptionCheck() && marr) {
                jsize mn = env->GetArrayLength(marr);
                for (jsize k = 0; k < mn && !m_gp; ++k) {
                    jobject m = env->GetObjectArrayElement(marr, k);
                    jobject rt = env->CallObjectMethod(m, m_getReturnType);
                    jboolean ok = env->CallBooleanMethod(cGP, m_isAssignableFrom, rt);
                    jobjectArray pt = (jobjectArray)env->CallObjectMethod(m, m_getParamTypes);
                    jsize pc = env->GetArrayLength(pt);
                    if (ok && pc == 0) m_gp = env->FromReflectedMethod(m);
                    env->DeleteLocalRef(pt);
                    env->DeleteLocalRef(rt);
                    env->DeleteLocalRef(m);
                }
                env->DeleteLocalRef(marr);
            } else env->ExceptionClear();
        }
        jobject gp = m_gp ? env->CallObjectMethod(npi, m_gp) : nullptr;
        if (env->ExceptionCheck()) { env->ExceptionClear(); gp = nullptr; }
        std::string name;
        if (gp) {
            jclass cGP = env->GetObjectClass(gp);
            jmethodID m_name = env->GetMethodID(cGP, "getName", "()Ljava/lang/String;");
            if (!m_name) { env->ExceptionClear(); }
            if (m_name) {
                jstring jn = (jstring)env->CallObjectMethod(gp, m_name);
                if (!env->ExceptionCheck() && jn) name = jstr(env, jn);
                else env->ExceptionClear();
                if (jn) env->DeleteLocalRef(jn);
            }
        }
        if (name.empty()) name = "?";
        out += name; out.push_back((char)10);
        if (gp) env->DeleteLocalRef(gp);
        env->DeleteLocalRef(cNpi);
        env->DeleteLocalRef(npi);
    }
    send_line(c, out.c_str(), true);
    env->DeleteLocalRef(arr);
    return true;
}

static std::string get_self_name(JNIEnv* env, jobject mc) {
    std::string name;
    jclass clsMc = env->GetObjectClass(mc);
    flog("id: get_self_name start");

    // 1) Minecraft.getSession()/session フィールド
    jobject session = nullptr;
    jmethodID midGetSession = tryMethod(env, clsMc, {
        {"getSession",    "()Lnet/minecraft/util/Session;"},
        {"func_110432_I", "()Lnet/minecraft/util/Session;"},
        {"getSession",    "()Ljava/lang/Object;"},
        {"func_110432_I", "()Ljava/lang/Object;"},
        {"ac",            "()Lbhz;"},   // 1.8.9 obf 想定
        {"ab",            "()Lbhz;"},
    }, "id:getSession");
    if (midGetSession) {
        session = env->CallObjectMethod(mc, midGetSession);
        if (env->ExceptionCheck()) { env->ExceptionClear(); session = nullptr; }
    }
    flog("id: session via method %s", session ? "ok" : "null");
    if (!session) {
        jfieldID fid = env->GetFieldID(clsMc, "session", "Lnet/minecraft/util/Session;");
        if (!fid) { env->ExceptionClear(); fid = env->GetFieldID(clsMc, "ab", "Lbhz;"); }
        if (fid) {
            session = env->GetObjectField(mc, fid);
            if (env->ExceptionCheck()) { env->ExceptionClear(); session = nullptr; }
        }
    }
    flog("id: session via field %s", session ? "ok" : "null");

    if (session) {
        jclass cSess = env->GetObjectClass(session);

        // 1a) 直接 getUsername()
        jmethodID midUser = tryMethod(env, cSess, {
            {"getUsername",   "()Ljava/lang/String;"},
            {"func_111285_c", "()Ljava/lang/String;"},
            {"c",             "()Ljava/lang/String;"},
        }, "id:sessUser");
        if (midUser) {
            jstring js = (jstring)env->CallObjectMethod(session, midUser);
            if (!env->ExceptionCheck() && js) { name = jstr(env, js); env->DeleteLocalRef(js); dlog("id: session.getUsername -> '%s'", name.c_str()); flog("id: session.getUsername -> '%s'", name.c_str()); }
            else env->ExceptionClear();
        }

        // 1b) プロファイル経由
        if (name.empty()) {
            jmethodID midProf = tryMethod(env, cSess, {
                {"getProfile",     "()Lcom/mojang/authlib/GameProfile;"},
                {"func_148256_e",  "()Lcom/mojang/authlib/GameProfile;"},
                {"e",              "()Lcom/mojang/authlib/GameProfile;"},
            }, "id:sessProfile");
            if (midProf) {
                jobject gp = env->CallObjectMethod(session, midProf);
                if (!env->ExceptionCheck() && gp) {
                    jclass cGP = env->GetObjectClass(gp);
                    jmethodID midName = cGP ? env->GetMethodID(cGP, "getName", "()Ljava/lang/String;") : nullptr;
                    if (midName) {
                        jstring js = (jstring)env->CallObjectMethod(gp, midName);
                        if (!env->ExceptionCheck() && js) { name = jstr(env, js); env->DeleteLocalRef(js); dlog("id: session.getProfile.getName -> '%s'", name.c_str()); flog("id: session.getProfile.getName -> '%s'", name.c_str()); }
                        else env->ExceptionClear();
                    }
                    env->DeleteLocalRef(gp);
                } else env->ExceptionClear();
            }
        }
        env->DeleteLocalRef(cSess);
        env->DeleteLocalRef(session);
    }

    // 2) Fallback: thePlayer から
    if (name.empty()) {
        jfieldID fidPl = env->GetFieldID(clsMc, "thePlayer", "Lnet/minecraft/client/entity/EntityPlayerSP;");
        if (!fidPl) { env->ExceptionClear(); fidPl = env->GetFieldID(clsMc, "h", "Lbew;"); }
        if (fidPl) {
            jobject player = env->GetObjectField(mc, fidPl);
            if (!env->ExceptionCheck() && player) {
                jclass cPl = env->GetObjectClass(player);

                // getName() / getCommandSenderName()
                jmethodID midNm = tryMethod(env, cPl, {
                    {"getName",                "()Ljava/lang/String;"},
                    {"getCommandSenderName",   "()Ljava/lang/String;"},
                }, "id:playerName");
                if (midNm) {
                    jstring js = (jstring)env->CallObjectMethod(player, midNm);
                    if (!env->ExceptionCheck() && js) { name = jstr(env, js); env->DeleteLocalRef(js); dlog("id: player.getName -> '%s'", name.c_str()); flog("id: player.getName -> '%s'", name.c_str()); }
                    else env->ExceptionClear();
                }

                // GameProfile.getName()
                if (name.empty()) {
                    jmethodID midGP = tryMethod(env, cPl, {
                        {"getGameProfile", "()Lcom/mojang/authlib/GameProfile;"},
                        {"func_146103_bH", "()Lcom/mojang/authlib/GameProfile;"},
                    }, "id:playerGP");
                    if (midGP) {
                        jobject gp = env->CallObjectMethod(player, midGP);
                        if (!env->ExceptionCheck() && gp) {
                            jclass cGP = env->GetObjectClass(gp);
                            jmethodID midName = cGP ? env->GetMethodID(cGP, "getName", "()Ljava/lang/String;") : nullptr;
                            if (midName) {
                                jstring js = (jstring)env->CallObjectMethod(gp, midName);
                                if (!env->ExceptionCheck() && js) { name = jstr(env, js); env->DeleteLocalRef(js); dlog("id: player.GameProfile.getName -> '%s'", name.c_str()); flog("id: player.GameProfile.getName -> '%s'", name.c_str()); }
                                else env->ExceptionClear();
                            }
                            env->DeleteLocalRef(gp);
                        } else env->ExceptionClear();
                    }
                }

                env->DeleteLocalRef(cPl);
                env->DeleteLocalRef(player);
            } else env->ExceptionClear();
        }
    }

    // 3) Fallback: match own GameProfile UUID in NetHandler's player list
    if (name.empty()) {
        dlog("id: trying NetHandler UUID-match fallback");
        flog("id: trying NetHandler UUID-match fallback");
        jfieldID fidPl = env->GetFieldID(clsMc, "thePlayer", "Lnet/minecraft/client/entity/EntityPlayerSP;");
        if (!fidPl) { env->ExceptionClear(); fidPl = env->GetFieldID(clsMc, "h", "Lbew;"); }
        jobject player = fidPl ? env->GetObjectField(mc, fidPl) : nullptr;
        if (!player || env->ExceptionCheck()) { env->ExceptionClear(); player = nullptr; }
        jobject myGP = nullptr;
        if (player) {
            jclass cPl = env->GetObjectClass(player);
            jmethodID midGP = tryMethod(env, cPl, {
                {"getGameProfile", "()Lcom/mojang/authlib/GameProfile;"},
                {"func_146103_bH", "()Lcom/mojang/authlib/GameProfile;"},
            }, "id:playerGP2");
            if (midGP) {
                myGP = env->CallObjectMethod(player, midGP);
                if (env->ExceptionCheck()) { env->ExceptionClear(); myGP = nullptr; }
            }
            env->DeleteLocalRef(cPl);
        }
        // Obtain NetHandler
        jobject nh = nullptr;
        if (player) {
            jclass cPl = env->GetObjectClass(player);
            jfieldID fidSQ = env->GetFieldID(cPl, "sendQueue", "Lnet/minecraft/client/network/NetHandlerPlayClient;");
            if (!fidSQ) { env->ExceptionClear(); fidSQ = env->GetFieldID(cPl, "c", "Lbcy;"); }
            if (!fidSQ) { env->ExceptionClear(); fidSQ = env->GetFieldID(cPl, "b", "Lbcy;"); }
            if (!fidSQ) { env->ExceptionClear(); fidSQ = env->GetFieldID(cPl, "a", "Lbcy;"); }
            if (!fidSQ) {
                // reflection scan
                jclass cClass = env->FindClass("java/lang/Class");
                jclass cField = env->FindClass("java/lang/reflect/Field");
                if (cClass && cField) {
                    jmethodID mGetFields = env->GetMethodID(cClass, "getDeclaredFields", "()[Ljava/lang/reflect/Field;");
                    jmethodID mGetType   = env->GetMethodID(cField, "getType",           "()Ljava/lang/Class;");
                    jmethodID mSetAcc    = env->GetMethodID(cField, "setAccessible",     "(Z)V");
                    jmethodID mGetName   = env->GetMethodID(cClass, "getName",           "()Ljava/lang/String;");
                    if (mGetFields && mGetType && mGetName) {
                        jobjectArray flds = (jobjectArray)env->CallObjectMethod(cPl, mGetFields);
                        if (!env->ExceptionCheck() && flds) {
                            jsize fn = env->GetArrayLength(flds);
                            for (jsize i = 0; i < fn && !fidSQ; ++i) {
                                jobject f = env->GetObjectArrayElement(flds, i);
                                if (!f) continue;
                                jobject t = env->CallObjectMethod(f, mGetType);
                                if (env->ExceptionCheck() || !t) { env->ExceptionClear(); env->DeleteLocalRef(f); continue; }
                                jstring jn = (jstring)env->CallObjectMethod(t, mGetName);
                                std::string tn = jn ? jstr(env, jn) : std::string();
                                if (jn) env->DeleteLocalRef(jn);
                                if (tn == "net.minecraft.client.network.NetHandlerPlayClient" || tn == "bcy") {
                                    if (mSetAcc) env->CallVoidMethod(f, mSetAcc, JNI_TRUE);
                                    fidSQ = env->FromReflectedField(f);
                                }
                                env->DeleteLocalRef(t);
                                env->DeleteLocalRef(f);
                            }
                            env->DeleteLocalRef(flds);
                        } else env->ExceptionClear();
                    } else env->ExceptionClear();
                } else env->ExceptionClear();
            }
            if (fidSQ) {
                nh = env->GetObjectField(player, fidSQ);
                if (env->ExceptionCheck()) { env->ExceptionClear(); nh = nullptr; }
            }
            env->DeleteLocalRef(cPl);
        }
        if (nh && myGP) {
            jclass cNH = env->GetObjectClass(nh);
            // Direct method first: NetHandlerPlayClient.getPlayerInfo(UUID)
            if (name.empty()) {
                jobject myId0 = nullptr;
                if (myGP) {
                    jclass cMyGP = env->GetObjectClass(myGP);
                    jmethodID midGP_getId = cMyGP ? env->GetMethodID(cMyGP, "getId", "()Ljava/util/UUID;") : nullptr;
                    if (midGP_getId) {
                        myId0 = env->CallObjectMethod(myGP, midGP_getId);
                        if (env->ExceptionCheck()) { env->ExceptionClear(); myId0 = nullptr; }
                    } else env->ExceptionClear();
                }
                        if (myId0) { flog("id: have my UUID; trying getPlayerInfo(UUID)");
                            jmethodID midPI = tryMethod(env, cNH, {
                        {"getPlayerInfo", "(Ljava/util/UUID;)Lnet/minecraft/client/network/NetworkPlayerInfo;"},
                        {"func_175104_a", "(Ljava/util/UUID;)Lnet/minecraft/client/network/NetworkPlayerInfo;"},
                        {"getPlayerInfo", "(Ljava/util/UUID;)Ljava/lang/Object;"},
                        {"func_175104_a", "(Ljava/util/UUID;)Ljava/lang/Object;"},
                        {"a", "(Ljava/util/UUID;)Lbcz;"},
                        {"a", "(Ljava/util/UUID;)Ljava/lang/Object;"}
                    }, "id:getPI");
                    if (midPI) {
                        jobject info = env->CallObjectMethod(nh, midPI, myId0);
                        if (!env->ExceptionCheck() && info) {
                            jclass cInfo = env->GetObjectClass(info);
                            jmethodID midProf2 = tryMethod(env, cInfo, {
                                {"getGameProfile", "()Lcom/mojang/authlib/GameProfile;"},
                                {"func_178845_a", "()Lcom/mojang/authlib/GameProfile;"},
                                {"a", "()Lcom/mojang/authlib/GameProfile;"}
                            }, "id:infoGP2");
                            if (midProf2) {
                                jobject gp2 = env->CallObjectMethod(info, midProf2);
                                if (!env->ExceptionCheck() && gp2) {
                                    jclass cGP2 = env->GetObjectClass(gp2);
                                    jmethodID midName2 = cGP2 ? env->GetMethodID(cGP2, "getName", "()Ljava/lang/String;") : nullptr;
                                    if (midName2) {
                                        jstring js2 = (jstring)env->CallObjectMethod(gp2, midName2);
                                                 if (!env->ExceptionCheck() && js2) { name = jstr(env, js2); env->DeleteLocalRef(js2); dlog("id: getPlayerInfo(UUID) -> '%s'", name.c_str()); flog("id: getPlayerInfo(UUID) -> '%s'", name.c_str()); }
                                                 else env->ExceptionClear();
                                    }
                                    env->DeleteLocalRef(cGP2);
                                    env->DeleteLocalRef(gp2);
                                } else env->ExceptionClear();
                            }
                            env->DeleteLocalRef(cInfo);
                            env->DeleteLocalRef(info);
                        } else env->ExceptionClear();
                        env->DeleteLocalRef(myId0);
                    }
                }
            }
            // getPlayerInfoMap/list -> Collection
            jmethodID midList = tryMethod(env, cNH, {
                {"getPlayerInfoMap", "()Ljava/util/Collection;"},
                {"getPlayerInfoList", "()Ljava/util/Collection;"},
                {"func_175106_d", "()Ljava/util/Collection;"},
                {"b", "()Ljava/util/Collection;"}
            }, "id:list");
            if (!midList) {
                env->ExceptionClear();
            }
            jobject coll = midList ? env->CallObjectMethod(nh, midList) : nullptr;
            if (!env->ExceptionCheck() && coll) {
                jclass cCol = env->FindClass("java/util/Collection");
                jclass cObj = env->FindClass("java/lang/Object");
                jmethodID midToArr = cCol ? env->GetMethodID(cCol, "toArray", "([Ljava/lang/Object;)[Ljava/lang/Object;") : nullptr;
                if (midToArr && cObj) {
                    jobjectArray empty = env->NewObjectArray(0, cObj, nullptr);
                    jobjectArray arr = (jobjectArray)env->CallObjectMethod(coll, midToArr, empty);
                    env->DeleteLocalRef(empty);
                    if (!env->ExceptionCheck() && arr) {
                        jsize n = env->GetArrayLength(arr);
                        jobject myId = nullptr;
                        jmethodID midEquals = nullptr;
                        if (myGP) {
                            jclass cMyGP = env->GetObjectClass(myGP);
                            jmethodID midGetIdMy = cMyGP ? env->GetMethodID(cMyGP, "getId", "()Ljava/util/UUID;") : nullptr;
                            if (midGetIdMy) {
                                myId = env->CallObjectMethod(myGP, midGetIdMy);
                                if (env->ExceptionCheck()) { env->ExceptionClear(); myId = nullptr; }
                                if (myId) {
                                    jclass cUUIDid = env->GetObjectClass(myId);
                                    midEquals = cUUIDid ? env->GetMethodID(cUUIDid, "equals", "(Ljava/lang/Object;)Z") : nullptr;
                                }
                            } else env->ExceptionClear();
                        }
                        for (jsize i = 0; i < n && name.empty(); ++i) {
                            jobject info = env->GetObjectArrayElement(arr, i);
                            if (!info) continue;
                            jclass cInfo = env->GetObjectClass(info);
                            jmethodID midProf = tryMethod(env, cInfo, {
                                {"getGameProfile", "()Lcom/mojang/authlib/GameProfile;"},
                                {"func_178845_a", "()Lcom/mojang/authlib/GameProfile;"},
                                {"a", "()Lcom/mojang/authlib/GameProfile;"}
                            }, "id:infoGP");
                            jobject gp = midProf ? env->CallObjectMethod(info, midProf) : nullptr;
                            if (!env->ExceptionCheck() && gp && myId && midEquals) {
                                jclass cGPx = env->GetObjectClass(gp);
                                jmethodID midGetIdX = cGPx ? env->GetMethodID(cGPx, "getId", "()Ljava/util/UUID;") : nullptr;
                                jmethodID midGetNameX = cGPx ? env->GetMethodID(cGPx, "getName", "()Ljava/lang/String;") : nullptr;
                                if (midGetIdX) {
                                    jobject gid = env->CallObjectMethod(gp, midGetIdX);
                                    if (!env->ExceptionCheck() && gid) {
                                        jboolean eq = env->CallBooleanMethod(gid, midEquals, myId);
                                        if (!env->ExceptionCheck() && eq == JNI_TRUE && midGetNameX) {
                                            jstring js = (jstring)env->CallObjectMethod(gp, midGetNameX);
                                            if (!env->ExceptionCheck() && js) { name = jstr(env, js); env->DeleteLocalRef(js); dlog("id: matched UUID -> '%s'", name.c_str()); flog("id: matched UUID -> '%s'", name.c_str()); }
                                            else env->ExceptionClear();
                                        } else env->ExceptionClear();
                                        env->DeleteLocalRef(gid);
                                    } else env->ExceptionClear();
                                } else env->ExceptionClear();
                            } else env->ExceptionClear();
                            if (gp) env->DeleteLocalRef(gp);
                            env->DeleteLocalRef(cInfo);
                            env->DeleteLocalRef(info);
                        }
                        // As a last resort on some clients (e.g., when only self is present in tab), use the single entry name
                        if (name.empty() && n == 1) {
                            jobject info0 = env->GetObjectArrayElement(arr, 0);
                            if (info0) {
                                jclass cInfo0 = env->GetObjectClass(info0);
                                jmethodID midProf0 = tryMethod(env, cInfo0, {
                                    {"getGameProfile", "()Lcom/mojang/authlib/GameProfile;"},
                                    {"func_178845_a", "()Lcom/mojang/authlib/GameProfile;"},
                                    {"a", "()Lcom/mojang/authlib/GameProfile;"}
                                }, "id:infoGP1");
                                jobject gp0 = midProf0 ? env->CallObjectMethod(info0, midProf0) : nullptr;
                                if (!env->ExceptionCheck() && gp0) {
                                    jclass cGP0 = env->GetObjectClass(gp0);
                                    jmethodID midGetName0 = cGP0 ? env->GetMethodID(cGP0, "getName", "()Ljava/lang/String;") : nullptr;
                                    if (midGetName0) {
                                        jstring js0 = (jstring)env->CallObjectMethod(gp0, midGetName0);
                                        if (!env->ExceptionCheck() && js0) { name = jstr(env, js0); env->DeleteLocalRef(js0); dlog("id: single-tab fallback -> '%s'", name.c_str()); flog("id: single-tab fallback -> '%s'", name.c_str()); }
                                        else env->ExceptionClear();
                                    } else env->ExceptionClear();
                                    env->DeleteLocalRef(gp0);
                                } else env->ExceptionClear();
                                env->DeleteLocalRef(cInfo0);
                                env->DeleteLocalRef(info0);
                            }
                        }
                        if (myId) env->DeleteLocalRef(myId);
                        env->DeleteLocalRef(arr);
                    } else env->ExceptionClear();
                } else env->ExceptionClear();
                env->DeleteLocalRef(coll);
            } else env->ExceptionClear();
            env->DeleteLocalRef(cNH);
        }
        if (nh) env->DeleteLocalRef(nh);
        if (myGP) env->DeleteLocalRef(myGP);
        if (player) env->DeleteLocalRef(player);
    }

    if (name.empty()) { dlog("id: still empty after all paths"); flog("id: still empty after all paths"); }
    return strip_mc_codes(name);
}

// '§x' の最初のカラーコード (0-9a-f/A-F) を拾う。なければ '\0'
static char first_color_code(const std::string& s) {
    for (size_t i = 0; i + 2 < s.size(); ++i) {
        unsigned char a = (unsigned char)s[i];
        unsigned char b = (unsigned char)s[i + 1];
        if (a == 0xC2 && b == 0xA7) {          // UTF-8 の '§'
            unsigned char k = (unsigned char)s[i + 2];
            if ((k >= '0' && k <= '9') || (k >= 'a' && k <= 'f') || (k >= 'A' && k <= 'F'))
                return (char)((k >= 'A' && k <= 'F') ? (k - 'A' + 'a') : k);
        } else if (a == 0xA7) {                // 稀に生バイト
            unsigned char k = (unsigned char)b;
            if ((k >= '0' && k <= '9') || (k >= 'a' && k <= 'f') || (k >= 'A' && k <= 'F'))
                return (char)((k >= 'A' && k <= 'F') ? (k - 'A' + 'a') : k);
        }
    }
    return '\0';
}


// TAB (with color): "tabc" 用
static void dump_tab_with_color(JNIEnv* env, SOCKET c) {
    dlog("tabc: begin");

    jobject mc = get_minecraft_singleton(env);
    if (!mc) { send_line(c, "tabc: 0\n", true); return; }
    jclass clsMc = env->GetObjectClass(mc);

    // world / scoreboard を取っておく
    jobject world = nullptr, scoreboard = nullptr;
    {
        jfieldID fidWorld = env->GetFieldID(clsMc, "theWorld", "Lnet/minecraft/client/multiplayer/WorldClient;");
        if (!fidWorld) { env->ExceptionClear(); fidWorld = env->GetFieldID(clsMc, "f", "Lbdb;"); }
        if (fidWorld) world = env->GetObjectField(mc, fidWorld);
        if (env->ExceptionCheck()) { env->ExceptionClear(); world = nullptr; }
        if (world) {
            jclass cWorld = env->GetObjectClass(world);
            jmethodID midSB = env->GetMethodID(cWorld, "getScoreboard", "()Lnet/minecraft/scoreboard/Scoreboard;");
            if (!midSB) { env->ExceptionClear(); midSB = env->GetMethodID(cWorld, "aA", "()Lbfd;"); }
            if (midSB) {
                scoreboard = env->CallObjectMethod(world, midSB);
                if (env->ExceptionCheck()) { env->ExceptionClear(); scoreboard = nullptr; }
            }
        }
    }

    // Scoreboard helpers
    jclass cSB = scoreboard ? env->GetObjectClass(scoreboard) : nullptr;
    jmethodID midGetTeam = nullptr;     // getPlayersTeam(name)
    if (cSB) {
        midGetTeam = env->GetMethodID(cSB, "getPlayersTeam", "(Ljava/lang/String;)Lnet/minecraft/scoreboard/ScorePlayerTeam;");
        if (!midGetTeam) { env->ExceptionClear(); midGetTeam = env->GetMethodID(cSB, "h", "(Ljava/lang/String;)Lbfe;"); }
    }
    jclass cSPT = env->FindClass("net/minecraft/scoreboard/ScorePlayerTeam"); if (!cSPT) { env->ExceptionClear(); cSPT = env->FindClass("bfe"); }

    auto getStrDyn = [&](jobject team, const char* n1,const char* n2,const char* n3,const char* n4)->std::string{
        if (!team) return {};
        jclass clsT = env->GetObjectClass(team);
        jmethodID m = env->GetMethodID(clsT, n1, "()Ljava/lang/String;");
        if (!m) { env->ExceptionClear(); m = env->GetMethodID(clsT, n2, "()Ljava/lang/String;"); }
        if (!m) { env->ExceptionClear(); m = env->GetMethodID(clsT, n3, "()Ljava/lang/String;"); }
        if (!m) { env->ExceptionClear(); m = env->GetMethodID(clsT, n4, "()Ljava/lang/String;"); }
        if (!m) return {};
        jstring s = (jstring)env->CallObjectMethod(team, m);
        if (env->ExceptionCheck()) { env->ExceptionClear(); return {}; }
        std::string r = jstr(env, s);
        if (s) env->DeleteLocalRef(s);
        return r;
    };

    // NetHandler からプレイヤー一覧（既存 dump_tab と同じ取得）
    std::vector<std::pair<std::string,std::string>> items; // (plain_name, color_code)

    // NetHandler
    jobject nh = nullptr;
    jmethodID midNH = tryMethod(env, clsMc, {
        {"getNetHandler", "()Lnet/minecraft/client/network/NetHandlerPlayClient;"},
        {"func_147114_u", "()Lnet/minecraft/client/network/NetHandlerPlayClient;"},
    }, "tabc:getNH");
    if (midNH) {
        nh = env->CallObjectMethod(mc, midNH);
        if (env->ExceptionCheck()) { env->ExceptionClear(); nh = nullptr; }
    }
    if (!nh) {
        jfieldID fidPl = env->GetFieldID(clsMc, "thePlayer", "Lnet/minecraft/client/entity/EntityPlayerSP;");
        if (!fidPl) { env->ExceptionClear(); fidPl = env->GetFieldID(clsMc, "h", "Lbew;"); }
        if (fidPl) {
            jobject player = env->GetObjectField(mc, fidPl);
            if (!env->ExceptionCheck() && player) {
                jclass cPl = env->GetObjectClass(player);
                jfieldID fidSQ = env->GetFieldID(cPl, "sendQueue", "Lnet/minecraft/client/network/NetHandlerPlayClient;");
                if (!fidSQ) { env->ExceptionClear(); fidSQ = env->GetFieldID(cPl, "c", "Lbcy;"); }
                if (!fidSQ) { env->ExceptionClear(); fidSQ = env->GetFieldID(cPl, "b", "Lbcy;"); }
                if (!fidSQ) { env->ExceptionClear(); fidSQ = env->GetFieldID(cPl, "a", "Lbcy;"); }
                if (!fidSQ) {
                    // Reflection fallback: scan declared fields for type NetHandlerPlayClient (or obf bcy)
                    jclass cClass = env->FindClass("java/lang/Class");
                    jclass cField = env->FindClass("java/lang/reflect/Field");
                    if (cClass && cField) {
                        jmethodID mGetFields = env->GetMethodID(cClass, "getDeclaredFields", "()[Ljava/lang/reflect/Field;");
                        jmethodID mGetType   = env->GetMethodID(cField, "getType", "()Ljava/lang/Class;");
                        jmethodID mSetAcc    = env->GetMethodID(cField, "setAccessible", "(Z)V");
                        jmethodID mGetName   = env->GetMethodID(cClass, "getName", "()Ljava/lang/String;");
                        if (mGetFields && mGetType && mGetName) {
                            jobjectArray flds = (jobjectArray)env->CallObjectMethod(cPl, mGetFields);
                            if (!env->ExceptionCheck() && flds) {
                                jsize fn = env->GetArrayLength(flds);
                                for (jsize i = 0; i < fn && !fidSQ; ++i) {
                                    jobject f = env->GetObjectArrayElement(flds, i);
                                    if (!f) continue;
                                    jobject t = env->CallObjectMethod(f, mGetType);
                                    if (env->ExceptionCheck() || !t) { env->ExceptionClear(); env->DeleteLocalRef(f); continue; }
                                    jstring jn = (jstring)env->CallObjectMethod(t, mGetName);
                                    std::string tn = jn ? jstr(env, jn) : std::string();
                                    if (jn) env->DeleteLocalRef(jn);
                                    if (tn == "net.minecraft.client.network.NetHandlerPlayClient" || tn == "bcy") {
                                        if (mSetAcc) env->CallVoidMethod(f, mSetAcc, JNI_TRUE);
                                        fidSQ = env->FromReflectedField(f);
                                    }
                                    env->DeleteLocalRef(t);
                                    env->DeleteLocalRef(f);
                                }
                                env->DeleteLocalRef(flds);
                            } else env->ExceptionClear();
                        } else env->ExceptionClear();
                    } else env->ExceptionClear();
                }
                if (fidSQ) {
                    nh = env->GetObjectField(player, fidSQ);
                    if (env->ExceptionCheck()) { env->ExceptionClear(); nh = nullptr; }
                }
                env->DeleteLocalRef(cPl);
                env->DeleteLocalRef(player);
            } else env->ExceptionClear();
        }
    }

    if (nh) {
        jclass cNH  = env->GetObjectClass(nh);
        jclass cNPI = env->FindClass("net/minecraft/client/network/NetworkPlayerInfo");
        if (!cNPI) { env->ExceptionClear(); cNPI = env->FindClass("bcz"); }

        jmethodID midList = tryMethod(env, cNH, {
            {"getPlayerInfoMap", "()Ljava/util/Collection;"},
            {"getPlayerInfoList", "()Ljava/util/Collection;"},
            {"func_175106_d", "()Ljava/util/Collection;"},
            {"b", "()Ljava/util/Collection;"}
        }, "tabc:list");

        if (midList) {
            jobject coll = env->CallObjectMethod(nh, midList);
            if (!env->ExceptionCheck() && coll) {
                jclass cCol = env->FindClass("java/util/Collection");
                jclass cObj = env->FindClass("java/lang/Object");
                if (cCol && cObj) {
                    jmethodID midToArr = env->GetMethodID(cCol, "toArray", "([Ljava/lang/Object;)[Ljava/lang/Object;");
                    if (midToArr) {
                        jobjectArray empty = env->NewObjectArray(0, cObj, nullptr);
                        jobjectArray arr = (jobjectArray)env->CallObjectMethod(coll, midToArr, empty);
                        env->DeleteLocalRef(empty);
                        if (!env->ExceptionCheck() && arr) {
                            // displayName / profile name
                            jclass cICC = env->FindClass("net/minecraft/util/IChatComponent");
                            if (!cICC) { env->ExceptionClear(); cICC = env->FindClass("fj"); }
                            jmethodID midUnf = tryMethod(env, cICC, {
                                {"getUnformattedText", "()Ljava/lang/String;"},
                                {"func_150260_c", "()Ljava/lang/String;"},
                                {"e", "()Ljava/lang/String;"},
                            }, "tabc:unf");
                            jmethodID midDisp = tryMethod(env, cNPI, {
                                {"getDisplayName", "()Lnet/minecraft/util/IChatComponent;"},
                                {"func_178854_k", "()Lnet/minecraft/util/IChatComponent;"},
                                {"d", "()Lfj;"}
                            }, "tabc:displayName");
                            jmethodID midProf = tryMethod(env, cNPI, {
                                {"getGameProfile", "()Lcom/mojang/authlib/GameProfile;"},
                                {"func_178845_a", "()Lcom/mojang/authlib/GameProfile;"},
                                {"a", "()Lcom/mojang/authlib/GameProfile;"}
                            }, "tabc:gameProfile");
                            jclass cGP = env->FindClass("com/mojang/authlib/GameProfile");
                            jmethodID midName = cGP ? env->GetMethodID(cGP, "getName", "()Ljava/lang/String;") : nullptr;

                            jsize n = env->GetArrayLength(arr);
                            for (jsize i = 0; i < n; ++i) {
                                jobject info = env->GetObjectArrayElement(arr, i);
                                if (!info) continue;

                                // base name（無色）
                                std::string base;
                                if (midDisp && midUnf) {
                                    jobject cc = env->CallObjectMethod(info, midDisp);
                                    if (!env->ExceptionCheck() && cc) {
                                        jstring js = (jstring)env->CallObjectMethod(cc, midUnf);
                                        if (!env->ExceptionCheck() && js) { base = jstr(env, js); env->DeleteLocalRef(js); }
                                        else env->ExceptionClear();
                                        env->DeleteLocalRef(cc);
                                    } else env->ExceptionClear();
                                }
                                if ((base.empty() || base == "(blank)") && midProf && midName) {
                                    jobject gp = env->CallObjectMethod(info, midProf);
                                    if (!env->ExceptionCheck() && gp) {
                                        jstring js = (jstring)env->CallObjectMethod(gp, midName);
                                        if (!env->ExceptionCheck() && js) { base = jstr(env, js); env->DeleteLocalRef(js); }
                                        else env->ExceptionClear();
                                        env->DeleteLocalRef(gp);
                                    } else env->ExceptionClear();
                                }
                                if (base.empty()) base = "(blank)";

                                // team prefix/suffix → 色抽出
                                std::string colored = base; // デフォルト：そのまま
                                char colorChar = '\0';
                                if (scoreboard && midGetTeam && cSPT && midName) {
                                    // name（GameProfile）でもう一度取得（jstring が必要）
                                    jstring jn = env->NewStringUTF(base.c_str());
                                    jobject team = env->CallObjectMethod(scoreboard, midGetTeam, jn);
                                    if (env->ExceptionCheck()) { env->ExceptionClear(); team = nullptr; }
                                    if (team) {
                                        std::string pre = getStrDyn(team, "getColorPrefix","getPrefix","func_96661_b","c");
                                        std::string suf = getStrDyn(team, "getColorSuffix","getSuffix","func_96663_c","d");
                                        colored = pre + base + suf;
                                        colorChar = first_color_code(colored);
                                        env->DeleteLocalRef(team);
                                    }
                                    env->DeleteLocalRef(jn);
                                }

                                std::string plain = ascii_only(strip_mc_codes(colored));
                                std::string colorStr;
                                if (colorChar) { colorStr.push_back(colorChar); }
                                items.emplace_back(plain, colorStr);
                                env->DeleteLocalRef(info);
                            }
                        } else env->ExceptionClear();
                    }
                }
            } else env->ExceptionClear();
        }
    }

    // 出力
    std::string out; char head[64];
    _snprintf_s(head, _TRUNCATE, "tabc: %d\n", (int)items.size());
    out += head;
    for (auto& it : items) {
        out += it.first; out += "\t"; out += it.second; out += "\n";
    }
    send_line(c, out, true);
    dlog("tabc: dump -> %zu bytes (+NUL)", out.size() + 1);

    if (world) env->DeleteLocalRef(world);
    if (scoreboard) env->DeleteLocalRef(scoreboard);
}


static void handle_line(const char* line, SOCKET c) {
    if (!line) return;
    dlog("BUS: line='%s'", line);

    if (_strnicmp(line, "chat ", 5) == 0) {
        const char* payload = line + 5;
        if (JNIEnv* env = get_env()) {
            JniLocalFrame frame(env, 128); // ★ このコマンド中に作られたローカル参照を全部掃除
            jobject mc = get_minecraft_singleton(env);
            if (!mc) {
                dlog("ERR: no mc");
                send_line(c, "chat: no mc", true);
            } else {
                bool ok = send_chat(env, mc, payload);
                send_line(c, ok ? "ok" : "ng", true);
            }
        }
        return;
    }

    if (_stricmp(line, "tab") == 0) {
        if (JNIEnv* env = get_env()) {
            JniLocalFrame frame(env, 256);
            dump_tab(env, c);
        }
        return;
    }

    if (_stricmp(line, "tabc") == 0) {
        if (JNIEnv* env = get_env()) {
            JniLocalFrame frame(env, 256);
            dump_tab_with_color(env, c);
        }
        return;
    }

    if (_stricmp(line, "score") == 0) {
        if (JNIEnv* env = get_env()) {
            JniLocalFrame frame(env, 512);
            dump_score(env, c);
        }
        return;
    }

    if (_stricmp(line, "id") == 0) {
        if (JNIEnv* env = get_env()) {
            JniLocalFrame frame(env, 128);
            jobject mc = get_minecraft_singleton(env);
            if (!mc) {
                send_line(c, "id: (no mc)", true);
            } else {
                std::string nm = get_self_name(env, mc);
                flog("id: result -> %s", nm.empty() ? "(empty)" : nm.c_str());
                if (nm.empty()) send_line(c, "id: (unknown)", true);
                else            send_line(c, std::string("id: ") + nm, true);
            }
        }
        return;
    }

    send_line(c, "unknown", true);
}

static DWORD WINAPI bus_thread(LPVOID) {
    flog("BUS: thread start");
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        dlog("WSAStartup failed");
        flog("WSAStartup failed");
        return 0;
    }

dlog("BUS: client mode scanning %s ports (primary %d)", BUS_HOST, (int)BUS_PORTS[0]);
flog("BUS: client mode scanning %s ports (primary %d)", BUS_HOST, (int)BUS_PORTS[0]);

    size_t port_index = 0;
    for (;;) {
        SOCKET c = socket(AF_INET, SOCK_STREAM, 0);
        if (c == INVALID_SOCKET) {
            dlog("socket() failed (%d)", WSAGetLastError());
            break;
        }

        sockaddr_in addr;
        ZeroMemory(&addr, sizeof(addr));
        addr.sin_family = AF_INET;
        int port = BUS_PORTS[port_index % BUS_PORTS_COUNT];
        addr.sin_port   = htons(port);
        inet_pton(AF_INET, BUS_HOST, &addr.sin_addr);

        if (connect(c, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            int err = WSAGetLastError();
dlog("BUS: connect to %s:%d failed (%d), retry",
                 BUS_HOST, port, err);
flog("BUS: connect to %s:%d failed (%d), retry", BUS_HOST, port, err);
            closesocket(c);
            ++port_index; // try next candidate when current port is closed
            Sleep(1000); // 1秒待ってから再接続
            continue;
        }
        
dlog("BUS: connected to %s:%d", BUS_HOST, port);
flog("BUS: connected to %s:%d", BUS_HOST, port);

        // プロセスID付きで挨拶
        DWORD pid = GetCurrentProcessId();
        char greet[64];
        _snprintf_s(greet, _TRUNCATE, "hello %lu", (unsigned long)pid);
        send_line(c, greet, false);
        flog("BUS: sent %s", greet);

        char buf[2048];
        int  used = 0;

        for (;;) {
            int n = recv(c, buf + used, (int)sizeof(buf) - 1 - used, 0);
            if (n <= 0) {
                // 残りがあれば最後の 1 行だけ処理
                if (used > 0) {
                    buf[used] = 0;
                    size_t L = used;
                    if (L && buf[L - 1] == '\r') buf[--L] = 0;
                    handle_line(buf, c);
                }
                dlog("BUS: server closed (%d)", n);
                flog("BUS: server closed (%d)", n);
                break;
            }

            used += n;
            buf[used] = 0;

            char* start = buf;
            for (;;) {
                char* lf = (char*)memchr(start, '\n', buf + used - start);
                if (!lf) break;
                *lf = 0;
                size_t L = (size_t)(lf - start);
                if (L && start[L - 1] == '\r') start[L - 1] = 0;
                handle_line(start, c);
                start = lf + 1;
            }

            int remain = (int)(buf + used - start);
            if (remain > 0 && start != buf) {
                memmove(buf, start, remain);
            }
            used = remain;
        }

        closesocket(c);
        dlog("BUS: disconnected, retry after delay");
        flog("BUS: disconnected, retry after delay");
        Sleep(1000); // 再接続リトライ
    }

    WSACleanup();
    return 0;
}


// ============ entry ============
extern "C" __declspec(dllexport)
DWORD WINAPI StartAgent(LPVOID) {
    dlog("StartAgent called");
    flog("StartAgent called");
    HANDLE th = CreateThread(nullptr, 0, bus_thread, nullptr, 0, nullptr);
    if (th) CloseHandle(th);
    return 1;
}
