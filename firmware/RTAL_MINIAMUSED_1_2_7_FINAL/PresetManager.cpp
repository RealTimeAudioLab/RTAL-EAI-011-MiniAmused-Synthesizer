#include "PresetManager.h"
#include "Board.h"
#include "Parameters.h"

#include <SD.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

namespace {
constexpr uint16_t kPresetSlots = 128;
constexpr const char *kRootDir = "/RTAL_MINIAMUSED";
constexpr const char *kPresetDir = "/RTAL_MINIAMUSED/presets";
constexpr size_t kParamCount = (size_t)ParameterID::COUNT;
constexpr size_t kPresetPathSize = 64;
static_assert(kParamCount <= 128, "Preset/UI dirty-mask supports up to 128 parameters");

static bool validSlot(uint16_t slot) { return slot < kPresetSlots; }

static void presetPath(char *dst, size_t n, uint16_t slot)
{ snprintf(dst, n, "/RTAL_MINIAMUSED/presets/%03u.rtal", (unsigned)slot); }
static void tempPath(char *dst, size_t n, uint16_t slot)
{ snprintf(dst, n, "/RTAL_MINIAMUSED/presets/%03u.tmp", (unsigned)slot); }
static void backupPath(char *dst, size_t n, uint16_t slot)
{ snprintf(dst, n, "/RTAL_MINIAMUSED/presets/%03u.bak", (unsigned)slot); }

static void cleanName(const char *src, char *dst, size_t n)
{
    if(!dst || n == 0) return;
    if(!src || !*src) src = "Preset";
    size_t w = 0;
    for(size_t r=0; src[r] && w+1<n; ++r) {
        unsigned char c=(unsigned char)src[r];
        if(c=='\r' || c=='\n' || c<32 || c==127) c=' ';
        dst[w++]=(char)c;
    }
    while(w && dst[w-1]==' ') --w;
    dst[w]=0;
    if(w==0) snprintf(dst,n,"Preset");
}

static bool readLine(File &f, char *line, size_t cap)
{
    if(!line || cap < 2 || !f.available()) return false;
    size_t n=f.readBytesUntil('\n',line,cap-1);
    line[n]=0;
    // A full buffer with more bytes before newline is treated as malformed.
    if(n==cap-1 && f.available()) {
        int c=f.peek();
        if(c!='\n' && c!='\r') return false;
    }
    while(n && (line[n-1]=='\r' || line[n-1]=='\n')) line[--n]=0;
    return true;
}
}

PresetManager& PresetManager::instance()
{
    static PresetManager p;
    return p;
}

void PresetManager::begin()
{
    _ready=false; _busy=false;
    if(!Board::sdReady()) return;
    if(!SD.exists(kRootDir) && !SD.mkdir(kRootDir)) return;
    if(!SD.exists(kPresetDir) && !SD.mkdir(kPresetDir)) return;
    _ready=true;
}

void PresetManager::service()
{
    // Deliberately no background SD traffic. UI requests stay deferred until
    // touch RELEASED, preserving the proven UI/SD architecture.
}

bool PresetManager::save(uint16_t slot, const char *name)
{
    if(!_ready || _busy || !validSlot(slot)) return false;
    _busy=true;

    char path[kPresetPathSize], tmp[kPresetPathSize], bak[kPresetPathSize], safeName[32];
    presetPath(path,sizeof(path),slot);
    tempPath(tmp,sizeof(tmp),slot);
    backupPath(bak,sizeof(bak),slot);
    cleanName(name,safeName,sizeof(safeName));

    if(SD.exists(tmp)) SD.remove(tmp);
    if(SD.exists(bak)) SD.remove(bak);

    File f=SD.open(tmp,FILE_WRITE);
    if(!f) { _busy=false; return false; }

    bool ok=true;
    ok &= f.println("RTAL_MINIMOOG_PRESET_V2") > 0;
    ok &= f.print("name=") > 0;
    ok &= f.println(safeName) > 0;
    ok &= f.printf("param_count=%u\n",(unsigned)kParamCount) > 0;

    Parameters &p=Parameters::instance();
    for(size_t i=0;i<kParamCount && ok;++i) {
        const float v=p.target((ParameterID)i);
        if(!isfinite(v) || f.printf("%u=%.7f\n",(unsigned)i,v)<=0) ok=false;
    }
    ok &= f.println("complete=1") > 0;
    f.flush();
    if(f.getWriteError()) ok=false;
    f.close();

    if(!ok) { SD.remove(tmp); _busy=false; return false; }

    // Transactional replacement: keep the old preset as .bak until the new
    // complete temporary file has become the live preset.
    const bool hadOld=SD.exists(path);
    if(hadOld && !SD.rename(path,bak)) {
        SD.remove(tmp); _busy=false; return false;
    }
    if(!SD.rename(tmp,path)) {
        if(hadOld) SD.rename(bak,path); // best-effort rollback
        SD.remove(tmp); _busy=false; return false;
    }
    if(hadOld && SD.exists(bak)) SD.remove(bak);

    _busy=false;
    return true;
}

bool PresetManager::load(uint16_t slot)
{
    if(!_ready || _busy || !validSlot(slot)) return false;
    _busy=true;

    char path[kPresetPathSize]; presetPath(path,sizeof(path),slot);
    File f=SD.open(path,FILE_READ);
    if(!f) { _busy=false; return false; }

    char line[128];
    if(!readLine(f,line,sizeof(line))) { f.close(); _busy=false; return false; }
    const bool v1=strcmp(line,"RTAL_MINIMOOG_PRESET_V1")==0;
    const bool v2=strcmp(line,"RTAL_MINIMOOG_PRESET_V2")==0;
    if(!v1 && !v2) { f.close(); _busy=false; return false; }

    float values[kParamCount] = {};
    bool seen[kParamCount] = {};
    size_t declaredCount=0;
    bool haveCount=false, complete=false, malformed=false;

    while(f.available() && !malformed) {
        if(!readLine(f,line,sizeof(line))) { malformed=true; break; }
        if(line[0]==0 || strncmp(line,"name=",5)==0) continue;
        if(strncmp(line,"param_count=",12)==0) {
            char *end=nullptr; unsigned long c=strtoul(line+12,&end,10);
            if(end==line+12 || *end!=0 || c>kParamCount) malformed=true;
            else { declaredCount=(size_t)c; haveCount=true; }
            continue;
        }
        if(strcmp(line,"complete=1")==0) { complete=true; continue; }

        char *eq=strchr(line,'=');
        if(!eq) { if(v2) malformed=true; continue; }
        *eq=0;
        char *endId=nullptr, *endVal=nullptr;
        long id=strtol(line,&endId,10);
        float v=strtof(eq+1,&endVal);
        if(endId==line || *endId!=0 || endVal==eq+1 || *endVal!=0 || !isfinite(v) ||
           id<0 || id>=(long)kParamCount || v<0.0f || v>1.0f) {
            if(v2) malformed=true;
            continue;
        }
        values[id]=v; seen[id]=true;
    }
    f.close();

    if(v2) {
        if(malformed || !haveCount || !complete || declaredCount==0) {
            _busy=false; return false;
        }
        for(size_t i=0;i<declaredCount;++i) {
            if(!seen[i]) { _busy=false; return false; }
        }
    }

    // Apply only after the whole file has validated. INIT first gives legacy
    // V1 presets deterministic defaults for parameters added in later builds
    // (notably TRIGGER_MODE and MASTER_TUNE), rather than inheriting whatever
    // happened to be active before LOAD.
    Parameters &p=Parameters::instance();
    p.applyInitPatch();
    for(size_t i=0;i<kParamCount;++i)
        if(seen[i] && (ParameterID)i != ParameterID::OVERLAY_TIMEOUT)
            p.set((ParameterID)i,values[i],ParameterSource::PRESET);

    _busy=false;
    return true;
}

bool PresetManager::exists(uint16_t slot) const
{
    if(!_ready || !validSlot(slot)) return false;
    char path[kPresetPathSize]; presetPath(path,sizeof(path),slot);
    return SD.exists(path);
}

bool PresetManager::readName(uint16_t slot, char *dst, size_t n) const
{
    if(!dst || n==0) return false;
    dst[0]=0;
    if(!_ready || !validSlot(slot)) return false;

    char path[kPresetPathSize]; presetPath(path,sizeof(path),slot);
    File f=SD.open(path,FILE_READ);
    if(!f) return false;

    char line[128];
    if(!readLine(f,line,sizeof(line))) { f.close(); return false; }
    if(strcmp(line,"RTAL_MINIMOOG_PRESET_V1")!=0 &&
       strcmp(line,"RTAL_MINIMOOG_PRESET_V2")!=0) { f.close(); return false; }

    while(f.available()) {
        if(!readLine(f,line,sizeof(line))) break;
        if(strncmp(line,"name=",5)==0) {
            cleanName(line+5,dst,n); f.close(); return true;
        }
    }
    f.close(); return false;
}

bool PresetManager::remove(uint16_t slot)
{
    if(!_ready || _busy || !validSlot(slot)) return false;
    _busy=true;
    char path[kPresetPathSize],tmp[kPresetPathSize],bak[kPresetPathSize];
    presetPath(path,sizeof(path),slot);
    tempPath(tmp,sizeof(tmp),slot);
    backupPath(bak,sizeof(bak),slot);
    if(SD.exists(tmp)) SD.remove(tmp);
    if(SD.exists(bak)) SD.remove(bak);
    bool ok=true;
    if(SD.exists(path)) ok=SD.remove(path);
    _busy=false; return ok;
}
