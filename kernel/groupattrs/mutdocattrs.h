#pragma once

#include "categseries.h"
#include "config.h"

#include <util/generic/noncopyable.h>
#include <util/generic/hash.h>
#include <util/generic/ptr.h>
#include <util/system/defaults.h>

namespace NGroupingAttrs {

class TDocAttrs;

class TMutableDocAttrs : TNonCopyable {
private:
    typedef TVector<TCategSeries> TData;

private:
    TConfig& Config_;
    TData Data;

private:
    void Init();

public:
    TMutableDocAttrs(TConfig& config);
    TMutableDocAttrs(TConfig& config, ui32 docid);

    const TConfig& Config() const {
        return Config_;
    }

    void ResetConfig(const TConfig& newconfig);
    void ResetIndexAuxAttr(ui32 docid);

    const TCategSeries* Attrs(ui32 attrnum) const;
    const TCategSeries* Attrs(const char* attrname) const;

    TCategSeries* Attrs(ui32 attrnum);
    TCategSeries* Attrs(const char* attrname);

    void SetAttr(ui32 attrnum, TCateg value);
    void SetAttr(const char* attrname, TCateg value);

    void Merge(const TDocAttrs& other, const THashMap<TString, TCateg> *changeMap = nullptr);

    bool IsEmpty() const;

    void Clear();
};

}
