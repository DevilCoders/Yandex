#pragma once

#include "parsefunc.h"
#include <library/cpp/html/face/parstypes.h>

#include <library/cpp/charset/ci_string.h>
#include <util/generic/hash_set.h>
#include <util/generic/yexception.h>
#include <util/memory/segmented_string_pool.h>

class THtConfigError: public yexception {
};

struct THtAttrConf {
    ATTR_POS Pos;
    ATTR_TYPE Type;
    TString Name;
    TString LocalName; // if not empty this should be used when link is local
    TCiString YxCondZone;
    TCiString YxLocalCondZone;
    HTATTR_PARSE_FUNC Func;
    bool Ignore;
};

struct THtAttrExtConf {
    // for links (attribute values) with specific extensions;
    typedef THashMap<const char*, THtAttrConf*> TExt;

    TExt Ext; // keys always in lowercase
    bool HasValueMatch;

    THtAttrExtConf();
    ~THtAttrExtConf();
};

struct THtElemConf {
    typedef THashMap<const char*, THtAttrExtConf*> TAttrs;

    TAttrs Attrs; // keys always in lowercase
    TString AnyAttr;
    THashMap<TCiString, TCiString> CondAttrZones;
    THtAttrExtConf* wildAttrConf;
    THtAttrExtConf* superWildAttrConf;

    THtElemConf();
    ~THtElemConf();
};

class THtConfigurator {
private:
    TVector<THtElemConf*> Conf;
    segmented_string_pool Pool;

    bool UriFilterIsOff;
    bool UnknownUtf;

public:
    void AddZoneConf(const TCiString& key, const TString& value);
    void AddAttrConf(const TCiString& key, const TString& value);

public:
    THtConfigurator();
    ~THtConfigurator();

    void Configure(const char* filename);
    void LoadDefaultConf();

    bool IsUriFilterOff() const {
        return UriFilterIsOff;
    }

    THtElemConf* GetConf(size_t index) const {
        return Conf[index];
    }

private:
    void ClearConf();
    bool CheckConfig(const char* key, const char* value) const;
};
