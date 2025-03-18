#pragma once

#include <antirobot/idl/cbb_cache.pb.h>

#include <util/folder/path.h>
#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/system/mutex.h>

namespace NAntiRobot {

class TCbbCache {
public:
    using TCache = THashMap<TString, TString>;

private:
    TCache Cache;
    mutable TMutex Mutex;

public:
    explicit TCbbCache(const TFsPath& filename);

    TMaybe<TString> Get(const TString& req);
    void Set(const TString& req, const TString& content);
    void Load(const TFsPath& filename);
    TCache GetCopy() const;
};

void SaveCbbCache(const TCbbCache::TCache& cbbCache, const TFsPath& filename);

} // namespace NAntiRobot
