#include "feature_filter.h"

#include <library/cpp/expression/expression.h>

#include <util/generic/xrange.h>

namespace NMLPool {
    TIndexFilter::TIndexFilter(const TString& indexExp)
    {
         TStringBuf buf = indexExp;
         TStringBuf part;
         while(buf.NextTok(',', part)) {
             TStringBuf firstNum;
             part.NextTok('-', firstNum);
             size_t begin = FromString<size_t>(firstNum);
             size_t end = begin;
             if (!!part) {
                 end = FromString<size_t>(part);
             }
             for (size_t i : xrange(begin, end + 1)) {
                 Indexes.insert(i);
             }
        }
    }

    bool TIndexFilter::Check(size_t index) const
    {
        return Indexes.contains(index);
    }

    void TFeatureFilter::SetSliceExpr(const TString& sliceExp)
    {
         SliceRE.Reset(!!sliceExp ? new TRegExBase(sliceExp) : nullptr);
    }

    void TFeatureFilter::SetNameExpr(const TString& nameExp)
    {
         NameRE.Reset(!!nameExp ? new TRegExBase(nameExp) : nullptr);
    }

    void TFeatureFilter::SetTagExpr(const TString& tagExp)
    {
        TagExp = tagExp;
    }

    bool TFeatureFilter::Check(const TFeatureInfo& info) const
    {
        regmatch_t match;

        if (!!SliceRE && 0 != SliceRE->Exec(info.GetSlice().data(), &match, 0, 1)) {
             return false;
        }

        if (!!NameRE && 0 != NameRE->Exec(info.GetName().data(), &match, 0, 1)) {
             return false;
        }

        if (!!TagExp) {
            THashMap<TString, TString> tagDict;
            for (const auto& tag : info.GetTags()) {
                tagDict[tag] = "1";
            }
            if (1 != CalcExpression(TagExp.data(), tagDict)) {
                return false;
            }
        }

        return true;
    }
} // NMLPool
