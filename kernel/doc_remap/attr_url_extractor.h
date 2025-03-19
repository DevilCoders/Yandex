#pragma once

#include <kernel/groupattrs/docsattrs.h>
#include <kernel/groupattrs/utils.h>

#include "id2string.h"

class TAttrUrlExtractor : public IId2String {
public:
    inline TAttrUrlExtractor(const TString& prefix, const TString& attrName, const TString& namesFile)
        : DocsAttrs(NGroupingAttrs::CommonMode)
        , AttrName(attrName)
    {
        Names = NGroupingAttrs::ReadCategToName(namesFile);
        DocsAttrs.InitCommonMode(false, prefix.data());

        AttrNum = DocsAttrs.Config().AttrNum(AttrName.data());
        if (AttrNum == NGroupingAttrs::TConfig::NotFound) {
            ythrow yexception() << prefix << "attributes file contain no attribute '" << AttrName << "'";
        }
    }

    inline TString GetString(ui32 docId) override {
        TCategSeries categs;
        DocsAttrs.DocCategs(docId, AttrNum, categs);

        if (categs.size() != 1) {
            if (categs.size() == 0) {
                ythrow yexception() << "Document " << docId << " contains no attrubute '" << AttrName << "'";
            } else {
                ythrow yexception() << "Document " << docId << " contains more than one attrubute '" << AttrName << "'";
            }
        }

        const TCateg c = categs.GetCateg(0);
        if (static_cast<ui64>(c) >= Names.size()) {
            ythrow yexception() << "There is no name for attribute '" << AttrName << "' value " << c;
        }
        return Names[c];
    }

private:
    TVector<TString> Names;
    NGroupingAttrs::TDocsAttrs DocsAttrs;
    TString AttrName;
    ui32 AttrNum;
};
