#pragma once

#include "categseries.h"
#include "config.h"

#include <util/generic/noncopyable.h>
#include <util/generic/yexception.h>
#include <util/system/defaults.h>

namespace NGroupingAttrs {

class TDocsAttrs;

class TDocAttrs : TNonCopyable {
private:
    friend class TDocsAttrs;
private:
    typedef TVector<TCategSeries> TData;

private:
    const TConfig& Config_;
    TData Data;

private:
    void SetAttr(const char* attrname, TCateg value) {
        SetAttr(Config().AttrNum(attrname), value);
    }

    void SetAttr(ui32 attrnum, TCateg value) {
        if (attrnum == TConfig::NotFound || attrnum >= Data.size()) {
            ythrow yexception() << "Can't set attribute " << attrnum;
        }

        if (value < 0 || value > Config().AttrMaxValue(attrnum)){
            ythrow yexception() << "Attribute " << attrnum << " is out of range (" << value << ")";
        }

        Data[attrnum].AddCateg(value);
    }

    TVector<TCategSeries> &MutableAttrs() {
        return Data;
    }

    TCategSeries& MutableAttrs(ui32 attrnum) {
        Y_ASSERT(attrnum != TConfig::NotFound && attrnum < Data.size());
        return Data[attrnum];
    }

public:
    TDocAttrs(const TConfig& config)
        : Config_(config)
        , Data(Config_.AttrCount())
    {
    }

    const TConfig& Config() const {
        return Config_;
    }

    const TCategSeries& Attrs(const char* attrname) const {
        return Attrs(Config().AttrNum(attrname));
    }

    const TCategSeries& Attrs(ui32 attrnum) const {
        Y_ASSERT(attrnum != TConfig::NotFound && attrnum < Data.size());
        return Data[attrnum];
    }

    size_t AttrValuesCount(const char* attrname) const {
        return AttrValuesCount(Config().AttrNum(attrname));
    }

    size_t AttrValuesCount(ui32 attrnum) const {
        Y_ASSERT(attrnum != TConfig::NotFound && attrnum < Data.size());
        return Data[attrnum].size();
    }

    void Clear() {
        for (size_t attrnum = 0; attrnum < Data.size(); ++attrnum) {
            Data[attrnum].Clear();
        }
    }
};

}

