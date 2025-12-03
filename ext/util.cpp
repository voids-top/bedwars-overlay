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
#include <cstdio>
#include <cstdarg>
#include <string>
#include <vector>
#include <algorithm>

#pragma comment(lib, "Ws2_32.lib")
#define BUS_HOST "127.0.0.1"
#define BUS_PORT 46001


static void dlog(const char* fmt, ...) {
    char buf[2048];
    va_list ap; va_start(ap, fmt);
    _vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, ap);
    va_end(ap);
    OutputDebugStringA("[AGENT] ");
    OutputDebugStringA(buf);
    OutputDebugStringA("\n");
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
    return nullptr;
}

static jobject get_minecraft_singleton(JNIEnv* env) {
    jclass mc = find_mc_class(env);
    if (!mc) { dlog("step0 NG: Minecraft class not found"); return nullptr; }
    struct M { const char* n; const char* s; } methods[] = {
        {"getMinecraft","()Lnet/minecraft/client/Minecraft;"},
        {"getMinecraft","()Ljava/lang/Object;"},
        {"func_71410_x","()Lnet/minecraft/client/Minecraft;"},
        {"func_71410_x","()Ljava/lang/Object;"},
        {"S","()Lave;"},
        {"S","()Ljava/lang/Object;"},
    };
    for (auto& m: methods) {
        jmethodID mid = env->GetStaticMethodID(mc, m.n, m.s);
        if (!mid) { env->ExceptionClear(); continue; }
        jobject inst = env->CallStaticObjectMethod(mc, mid);
        if (env->ExceptionCheck()) { env->ExceptionClear(); continue; }
        if (inst) { dlog("step0: got minecraft_singleton() via %s%s", m.n, m.s); return inst; }
    }
    struct F { const char* n; const char* s; } fields[] = {
        {"theMinecraft","Lnet/minecraft/client/Minecraft;"},
        {"theMinecraft","Ljava/lang/Object;"},
        {"M","Lave;"},
        {"M","Ljava/lang/Object;"},
    };
    for (auto& f: fields) {
        jfieldID fid = env->GetStaticFieldID(mc, f.n, f.s);
        if (!fid) { env->ExceptionClear(); continue; }
        jobject inst = env->GetStaticObjectField(mc, fid);
        if (env->ExceptionCheck()) { env->ExceptionClear(); continue; }
        if (inst) { dlog("step0: got minecraft_singleton field %s", f.n); return inst; }
    }
    dlog("step0 NG: no static method/field yielded instance");
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

static bool send_chat(JNIEnv* env, jobject mc, const char* text) {
    jclass clsMc = env->GetObjectClass(mc);
    jfieldID fidPl = env->GetFieldID(clsMc, "thePlayer", "Lnet/minecraft/client/entity/EntityPlayerSP;");
    if (!fidPl) { env->ExceptionClear(); fidPl = env->GetFieldID(clsMc, "h", "Lbew;"); }
    if (!fidPl) { dlog("player field not found"); return false; }
    jobject player = env->GetObjectField(mc, fidPl);
    if (!player || env->ExceptionCheck()) { env->ExceptionClear(); dlog("player null"); return false; }
    jclass clsPl = env->GetObjectClass(player);
    jmethodID midChat = env->GetMethodID(clsPl, "sendChatMessage", "(Ljava/lang/String;)V");
    if (!midChat) { env->ExceptionClear(); midChat = env->GetMethodID(clsPl, "a", "(Ljava/lang/String;)V"); }
    if (!midChat) { dlog("sendChatMessage not found"); return false; }
    jstring jmsg = env->NewStringUTF(text);
    env->CallVoidMethod(player, midChat, jmsg);
    if (env->ExceptionCheck()) { env->ExceptionDescribe(); env->ExceptionClear(); env->DeleteLocalRef(jmsg); dlog("chat send failed"); return false; }
    env->DeleteLocalRef(jmsg);
    dlog("chat sent (public path)");
    return true;
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
    if (!session) {
        jfieldID fid = env->GetFieldID(clsMc, "session", "Lnet/minecraft/util/Session;");
        if (!fid) { env->ExceptionClear(); fid = env->GetFieldID(clsMc, "ab", "Lbhz;"); }
        if (fid) {
            session = env->GetObjectField(mc, fid);
            if (env->ExceptionCheck()) { env->ExceptionClear(); session = nullptr; }
        }
    }

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
            if (!env->ExceptionCheck() && js) { name = jstr(env, js); env->DeleteLocalRef(js); }
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
                    jclass cGP = env->FindClass("com/mojang/authlib/GameProfile");
                    jmethodID midName = cGP ? env->GetMethodID(cGP, "getName", "()Ljava/lang/String;") : nullptr;
                    if (midName) {
                        jstring js = (jstring)env->CallObjectMethod(gp, midName);
                        if (!env->ExceptionCheck() && js) { name = jstr(env, js); env->DeleteLocalRef(js); }
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
                    if (!env->ExceptionCheck() && js) { name = jstr(env, js); env->DeleteLocalRef(js); }
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
                            jclass cGP = env->FindClass("com/mojang/authlib/GameProfile");
                            jmethodID midName = cGP ? env->GetMethodID(cGP, "getName", "()Ljava/lang/String;") : nullptr;
                            if (midName) {
                                jstring js = (jstring)env->CallObjectMethod(gp, midName);
                                if (!env->ExceptionCheck() && js) { name = jstr(env, js); env->DeleteLocalRef(js); }
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
                if (!fidSQ) { env->ExceptionClear(); fidSQ = env->GetFieldID(cPl, "b", "Lbcy;"); }
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
                if (nm.empty()) send_line(c, "id: (unknown)", true);
                else            send_line(c, std::string("id: ") + nm, true);
            }
        }
        return;
    }

    send_line(c, "unknown", true);
}

static DWORD WINAPI bus_thread(LPVOID) {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        dlog("WSAStartup failed");
        return 0;
    }

    dlog("BUS: client mode to %s:%d", BUS_HOST, BUS_PORT);

    for (;;) {
        SOCKET c = socket(AF_INET, SOCK_STREAM, 0);
        if (c == INVALID_SOCKET) {
            dlog("socket() failed (%d)", WSAGetLastError());
            break;
        }

        sockaddr_in addr;
        ZeroMemory(&addr, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port   = htons(BUS_PORT);
        inet_pton(AF_INET, BUS_HOST, &addr.sin_addr);

        if (connect(c, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            int err = WSAGetLastError();
            dlog("BUS: connect to %s:%d failed (%d), retry",
                 BUS_HOST, BUS_PORT, err);
            closesocket(c);
            Sleep(1000); // 1秒待ってから再接続
            continue;
        }
        
        dlog("BUS: connected to %s:%d", BUS_HOST, BUS_PORT);

        // プロセスID付きで挨拶
        DWORD pid = GetCurrentProcessId();
        char greet[64];
        _snprintf_s(greet, _TRUNCATE, "hello %lu", (unsigned long)pid);
        send_line(c, greet, false);

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
        Sleep(1000); // 再接続リトライ
    }

    WSACleanup();
    return 0;
}


// ============ entry ============
extern "C" __declspec(dllexport)
DWORD WINAPI StartAgent(LPVOID) {
    dlog("StartAgent called");
    HANDLE th = CreateThread(nullptr, 0, bus_thread, nullptr, 0, nullptr);
    if (th) CloseHandle(th);
    return 1;
}
