#pragma once

#include "config.h"
#include "metainfo.h"

#include <util/generic/noncopyable.h>
#include <util/generic/vector.h>
#include <util/generic/ptr.h>
#include <util/system/defaults.h>

namespace NGroupingAttrs {

class TMetainfos : TNonCopyable {
private:
    const TConfig& Config;

    TVector<TAutoPtr<TMetainfo> > Metainfos;

    bool DynamicC2N;

private:
    void ParseFile(const TString& indexdir, ui32 attrid, bool lockMemory = false);

protected:
    TMetainfos(bool dynamicC2N, const TConfig& config, size_t size)
        : Config(config)
        , Metainfos(size)
        , DynamicC2N(dynamicC2N)
    { }

    TMetainfo& ProvideMetainfo(ui32 attrNum) {
        Y_VERIFY(attrNum != TConfig::NotFound && attrNum < Metainfos.size(), "oops");
        if (!Metainfos[attrNum])
            Metainfos[attrNum].Reset(new TMetainfo(DynamicC2N));
        return *Metainfos[attrNum];
    }

public:
    TMetainfos(bool dynamicC2N, const TConfig& config)
        : Config(config)
        , DynamicC2N(dynamicC2N)
    {
    }

    void Init(const char* indexname, bool lockMemory = false);
    void Clear();

    const char* CategName(const TCateg& aclass, const char* attrname) const;
    TCateg CategParent(const TCateg& aclass, const char* attrname) const;
    bool IsLink(const char* attrname, const TCateg& parent, const TCateg& link) const;

    const TMetainfo* Metainfo(TStringBuf attrname) const {
        ui32 attrnum = Config.AttrNum(attrname);
        if (attrnum == TConfig::NotFound) {
            return nullptr;
        }

        return Metainfo(attrnum);
    }

    const TMetainfo* Metainfo(ui32 attrnum) const {
        if (attrnum == TConfig::NotFound || attrnum > Config.AttrCount()) {
            return nullptr;
        }

        return Metainfos[attrnum].Get();
    }
};

}
