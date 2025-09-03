#include "Localization.h"
#include "NppHelpers.h"
#include <windows.h>
#include <shlwapi.h>
#include <fstream>
#include <mutex>
#include <unordered_map>

// Minimal localization:
// - Language INI files live in <plugin>/languages/<lang>.ini (fallback relative ./languages/ during development)
// - Keys stored flat as "section.key" (all lowercase)
// - Lookup order: active language -> english -> original key text
// - No built-in strings; english.ini is authoritative. Missing english.ini causes raw keys to appear.
// - Thread-safe lazy load; no eviction.

static std::mutex g_langDataMutex; // protect map
static std::unordered_map<std::string, std::unordered_map<std::string, std::wstring>> g_langData;

static std::string toLower(const std::string &s){
    std::string r = s; for(char &c: r) c = (char)tolower((unsigned char)c); return r; }

// Resolve plugin directory to build absolute path to /languages even when CWD differs
static std::string getLanguagesDir() {
    static std::string cachedDir;
    if (!cachedDir.empty()) return cachedDir;
    HMODULE hm{};
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCSTR>(&getUILanguage), &hm)) {
        char pathBuf[MAX_PATH];
        if (GetModuleFileNameA(hm, pathBuf, sizeof(pathBuf))) {
            PathRemoveFileSpecA(pathBuf);
            std::string base(pathBuf);
            cachedDir = base + "/languages/";
            return cachedDir;
        }
    }
    // Fallback: relative (development scenario)
    cachedDir = "languages/";
    return cachedDir;
}

static bool g_warnedMissingEnglish = false;

static void loadLangIfNeeded(const std::string &lang){
    std::lock_guard<std::mutex> lock(g_langDataMutex);
    if(g_langData.find(lang)!=g_langData.end()) return;
    std::unordered_map<std::string, std::wstring> entries;

    std::string path = getLanguagesDir() + lang + ".ini";
    std::ifstream ifs(path, std::ios::binary);
    if(!ifs.is_open()){
        if(lang != "english") {
            // fallback silently to english (which may itself be missing -> then keys return raw)
            loadLangIfNeeded("english");
            auto it = g_langData.find("english");
            if(it != g_langData.end()) {
                g_langData[lang] = it->second; // copy english entries
            } else {
                g_langData[lang]; // empty map placeholder
            }
            return;
        } else {
            // english missing: leave empty; lookups will fall back to key names
            g_langData[lang];
            if(!g_warnedMissingEnglish) {
                OutputDebugStringA("[ComparePlus][Localization] Warning: english.ini not found; showing raw keys.\n");
                g_warnedMissingEnglish = true;
            }
            return;
        }
    }

    std::string currentSection;
    std::string line;
    while(std::getline(ifs,line)){
        while(!line.empty() && (line.back()=='\r' || line.back()=='\n')) line.pop_back();
        if(line.empty() || line[0]==';' || line[0]=='#') continue;
        if(line.front()=='[' && line.back()==']'){
            currentSection = toLower(line.substr(1,line.size()-2));
            continue;
        }
        size_t eq = line.find('=');
        if(eq==std::string::npos) continue;
        std::string key = toLower(line.substr(0,eq));
        std::string val = line.substr(eq+1);
        // trim spaces
        auto trim=[&](std::string &s){ while(!s.empty() && (s.back()==' '||s.back()=='\t')) s.pop_back(); size_t i=0; while(i<s.size() && (s[i]==' '||s[i]=='\t')) ++i; if(i) s=s.substr(i); };
        trim(key); trim(val);
        int wlen = MultiByteToWideChar(CP_UTF8,0,val.c_str(),(int)val.size(),nullptr,0);
        std::wstring wval; wval.resize(wlen);
        MultiByteToWideChar(CP_UTF8,0,val.c_str(),(int)val.size(), &wval[0], wlen);
        std::string flatKey = currentSection + "." + key;
        entries[flatKey]=wval;
    }
    g_langData[lang] = std::move(entries);
}

std::wstring getLocalizedString(const std::string& lang, const std::string& section, const std::string& key){
    const std::string langLow = toLower(lang);
    const std::string sectionLow = toLower(section);
    const std::string keyLow = toLower(key);
    const std::string flatKey = sectionLow + "." + keyLow;
    loadLangIfNeeded(langLow);
    std::lock_guard<std::mutex> lock(g_langDataMutex);
    auto l = g_langData.find(langLow);
    if (l != g_langData.end()) {
        auto kIt = l->second.find(flatKey);
        if (kIt != l->second.end()) return kIt->second;
    }
    if (langLow != "english") {
        // one-shot fallback to english (ensure loaded)
        loadLangIfNeeded("english");
        auto e = g_langData.find("english");
        if (e != g_langData.end()) {
            auto kIt = e->second.find(flatKey);
            if (kIt != e->second.end()) return kIt->second;
        }
    }
    std::wstring w; w.assign(key.begin(), key.end());
    return w;
}

// Try to query Notepad++ for active native language file.
// API expects char* buffer (ANSI). Returns length (not including null) or 0.
static std::string queryNppNativeLangFile(){
    if(!nppData._nppHandle) return {};
    char buf[512] = {0};
    LRESULT len = ::SendMessageA(nppData._nppHandle, NPPM_GETNATIVELANGFILENAME, (WPARAM)sizeof(buf), (LPARAM)buf);
    if(len <= 0 || (size_t)len >= sizeof(buf)) return {};
    return std::string(buf, (size_t)len);
}

// Removed best-match heuristic: we don't attempt partial/substring matches anymore.

std::string getUILanguage(bool forceRefresh){
    static std::string cached;
    if(forceRefresh) cached.clear();
    if(!cached.empty()) return cached;

    std::string nppFile = queryNppNativeLangFile();
    if(nppFile.empty()) {
        // Defer caching until we actually get a filename; return empty so caller can fallback at UI-time
        return std::string();
    }
    // Extract basename (strip path & extension)
    std::string lower = toLower(nppFile);
    size_t slash = lower.find_last_of("/\\");
    if(slash != std::string::npos) lower = lower.substr(slash+1);
    size_t dot = lower.find_last_of('.');
    if(dot != std::string::npos) lower = lower.substr(0,dot);
    if(lower.empty()) lower = "english";
    // Verify file exists (except for english which can be built-in)
    if(lower != "english") {
        std::string testPath = getLanguagesDir() + lower + ".ini";
        DWORD attr = GetFileAttributesA(testPath.c_str());
        if(attr == INVALID_FILE_ATTRIBUTES || (attr & FILE_ATTRIBUTE_DIRECTORY)) {
            lower = "english";
        }
    }
    cached = lower;
    return cached;
}
