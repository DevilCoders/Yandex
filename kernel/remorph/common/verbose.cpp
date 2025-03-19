#include "verbose.h"

#include <util/stream/null.h>
#include <util/generic/singleton.h>
#include <util/generic/hash.h>
#include <util/generic/yexception.h>
#include <util/string/cast.h>

namespace NVerbosePrivate {

struct TInfoStreamHolder {
    IOutputStream* Out;
    int Level;

    TInfoStreamHolder()
        : Out(&Cnull)
        , Level(0)
    {
#ifdef Y_ENABLE_TRACE
        SetLevel(StdDbgLevel());
#endif
    }

    void SetLevel(int level) {
        Level = level;
        Out = Level > 0 ? &Cerr : &Cnull;
    }
};

} // NVerbosePrivate

void SetVerbosityLevel(int level) {
    Singleton<NVerbosePrivate::TInfoStreamHolder>()->SetLevel(level);
}

int GetVerbosityLevel() {
    return Singleton<NVerbosePrivate::TInfoStreamHolder>()->Level;
}

IOutputStream& GetInfoStream() {
    return *Singleton<NVerbosePrivate::TInfoStreamHolder>()->Out;
}

struct TLevelMap: public THashMap<TString, int> {
    TLevelMap() {
        insert(std::pair<TString, int>("err", TRACE_ERR));
        insert(std::pair<TString, int>("error", TRACE_ERR));
        insert(std::pair<TString, int>("warn", TRACE_WARN));
        insert(std::pair<TString, int>("warning", TRACE_WARN));
        insert(std::pair<TString, int>("notice", TRACE_NOTICE));
        insert(std::pair<TString, int>("info", TRACE_INFO));
        insert(std::pair<TString, int>("debug", TRACE_DEBUG));
        insert(std::pair<TString, int>("detail", TRACE_DETAIL));
        insert(std::pair<TString, int>("verbose", TRACE_VERBOSE));
    }

    int FromString(const TString& str) const {
        const_iterator i = find(str);
        if (i != end())
            return i->second;
        throw yexception() << "Bad verbose level value " << str;
    }
};

int VerbosityLevelFromString(const TString& str) {
    try {
        return ::FromString<int>(str);
    } catch (const TFromStringException& error) {
        Y_UNUSED(error);
        return Default<TLevelMap>().FromString(to_lower(str));
    }
}
