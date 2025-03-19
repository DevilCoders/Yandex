#include "mutdocattrs.h"

#include "docattrs.h"

#include <util/generic/yexception.h>

namespace NGroupingAttrs {

void TMutableDocAttrs::Init() {
    Data.resize(Config_.AttrCount());
}

TMutableDocAttrs::TMutableDocAttrs(TConfig& config)
    : Config_(config)
{
    Y_ASSERT(Config_.UsageMode() == TConfig::Search);
    Init();
}

TMutableDocAttrs::TMutableDocAttrs(TConfig& config, ui32 docid)
    : Config_(config)
{
    Init();
    if (Config_.UsageMode() == TConfig::Index) {
        SetAttr(Config_.IndexAuxAttrName(), static_cast<TCateg>(docid));
    }
}

void TMutableDocAttrs::ResetConfig(const TConfig& newconfig) {
    TData newdata(newconfig.AttrCount());

    for (ui32 attrnum = 0; attrnum < newconfig.AttrCount(); ++attrnum) {
        const char* name = newconfig.AttrName(attrnum);
        if (Config_.HasAttr(name)) {
            newdata[attrnum] = Data[Config_.AttrNum(name)];
        }
    }

    Data.swap(newdata);
    Config_ = newconfig;
}

void TMutableDocAttrs::ResetIndexAuxAttr(ui32 docid) {
    Y_ASSERT(Config_.UsageMode() == TConfig::Index);
    SetAttr(Config_.IndexAuxAttrName(), static_cast<TCateg>(docid));
}

const TCategSeries* TMutableDocAttrs::Attrs(ui32 attrnum) const {
    if (attrnum == TConfig::NotFound || attrnum >= Data.size()) {
        return nullptr;
    }

    return &Data[attrnum];
}

const TCategSeries* TMutableDocAttrs::Attrs(const char* attrname) const {
    ui32 attrnum = Config_.AttrNum(attrname);
    return Attrs(attrnum);
}

TCategSeries* TMutableDocAttrs::Attrs(ui32 attrnum) {
    if (attrnum == TConfig::NotFound || attrnum >= Data.size()) {
        return nullptr;
    }

    return &Data[attrnum];
}

TCategSeries* TMutableDocAttrs::Attrs(const char* attrname) {
    ui32 attrnum = Config_.AttrNum(attrname);
    return Attrs(attrnum);
}

void TMutableDocAttrs::SetAttr(ui32 attrnum, TCateg value) {
    if (attrnum == TConfig::NotFound || attrnum >= Data.size()) {
        ythrow yexception() << "Can't set attribute " << attrnum;
    }

    if (value < 0 || value > Config().AttrMaxValue(attrnum)) {
        ythrow yexception() << "Attribute " << attrnum << " is out of range (" << value << ")";
    }

    Data[attrnum].AddCateg(value);
}

void TMutableDocAttrs::SetAttr(const char* attrname, TCateg value) {
    ui32 attrnum = Config_.AttrNum(attrname);
    SetAttr(attrnum, value);
}

// If changeMap provided, apply values from changeMap while copying attributes from TDocAttrs...
void TMutableDocAttrs::Merge(const TDocAttrs& other, const THashMap<TString, TCateg>* changeMap) {
    for (ui32 oattrnum = 0; oattrnum < other.Config().AttrCount(); ++oattrnum) {
        const char *attrname = other.Config().AttrName(oattrnum);

        if (Config_.UsageMode() == TConfig::Search && strcmp(attrname, Config_.IndexAuxAttrName()) == 0) {
            continue;
        }

        Y_ASSERT(Config().HasAttr(attrname));

        THashMap<TString, TCateg>::const_iterator itr;
        if (changeMap && (itr = changeMap->find(attrname)) != changeMap->end()) {
            // This attribute exists in the changeMap, use value from the map!
            // Note that if there were many values for this attribute, only one (new) value will exist
            SetAttr(attrname, itr->second);
        } else {
            const TCategSeries& categs = other.Attrs(oattrnum);
            for (size_t i = 0; i < categs.size(); ++i) {
                SetAttr(attrname, categs.GetCateg(i));
            }
        }
    }
}

bool TMutableDocAttrs::IsEmpty() const {
    for (TData::const_iterator it = Data.begin(); it != Data.end(); ++it) {
        if (it->size() > 0) {
            return false;
        }
    }

    return true;
}

void TMutableDocAttrs::Clear() {
    for (size_t attrnum = 0; attrnum < Data.size(); ++attrnum) {
        Data[attrnum].Clear();
    }
}

}
