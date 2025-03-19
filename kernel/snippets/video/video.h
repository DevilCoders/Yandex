#pragma once

#include <kernel/tarc/docdescr/docdescr.h>
#include <kernel/snippets/strhl/zonedstring.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>

namespace NSnippets {
    class TSnipTitle;
    class TQueryy;
    class TConfig;
    class TArchiveView;
    class TMakeTitleOptions;
    class TLengthChooser;
    class TArchiveStorage;
    class TArc;

    bool GenerateVideoTitle(TSnipTitle& resTitle, const TArchiveView& textView, const TQueryy& query, const TMakeTitleOptions& options, const TDocInfos& docInfos, const TConfig& config, const TSnipTitle& naturalTitle);

    void HideVideoSnippets(TVector<TZonedString>& snipVec, TVector<TString>& attrVec, const TConfig& cfg, const TLengthChooser& lenCfg, TArchiveStorage& store, TArc& arc);

    double TreatVideoPassageAttrWeight(const TString& textAttr, const TConfig& cfg);
    double TreatVideoPassageAttrWeight(const TWtringBuf& attribute, const TConfig& cfg);
}
