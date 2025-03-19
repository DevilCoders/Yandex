#pragma once

#include <kernel/feature_pool/feature_pool.h>

#include <library/cpp/regex/pcre/regexp.h>

#include <util/generic/set.h>

class TRegExBase;

namespace NMLPool {
    // index-expr = index-expr-part {, index-expr-part} | empty-string
    // index-expr-part = index-expr-single |
    //                   index-expr-range
    // index-expr-single = int-literal
    // index-expr-range = int-literal '-' int-literal
    class TIndexFilter {
    public:
        TIndexFilter(const TString& indexExp);

        bool Check(size_t index) const;

        Y_FORCE_INLINE bool operator()(size_t index) const {
            return Check(index);
        }

    private:
        TSet<size_t> Indexes;
    };

    class TFeatureFilter {
    public:
        // Slice name PCRE
        void SetSliceExpr(const TString& sliceExp);

        // Feature name PCRE
        void SetNameExpr(const TString& nameExp);

        // C-style logical expression that uses
        // tags as bool variables
        void SetTagExpr(const TString& tagExp);

        bool Check(const TFeatureInfo& info) const;

        Y_FORCE_INLINE bool operator()(const TFeatureInfo& info) const {
            return Check(info);
        }

    private:
        THolder<TRegExBase> SliceRE;
        THolder<TRegExBase> NameRE;
        TString TagExp;
    };
} // NMLPool
