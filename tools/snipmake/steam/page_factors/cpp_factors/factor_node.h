#pragma once

#include "block_stats.h"

#include <library/cpp/domtree/domtree.h>
#include <library/cpp/html/spec/tags.h>

#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>
#include <util/generic/ymath.h>

namespace NSegmentator {

const ui32 BAD_ID = static_cast<ui32>(-1);

const TString& GetSegType(ui32 segCode);


bool IsInlineTag(const NHtml::TTag& tag);
bool IsMediaTag(const NHtml::TTag& tag);


class TBoolOption {
public:
    bool IsActive() const {
        return Active;
    }

    bool Set(bool value) {
        Active = true;
        Status = value;
        return Status;
    }

    bool Get() const {
        Y_ASSERT(Active);
        return Status;
    }

    bool Get(bool defaultValue) const {
        if (Active) {
            return Status;
        }
        return defaultValue;
    }

private:
    bool Active = false;
    bool Status = false;
};


struct TMetaFactors {
    ui32 RootDist = BAD_ID;
    float AvgLeavesDepth = nanf("");
    float MedLeavesDepth= nanf("");
    float MaxLeavesDepth = nanf("");
    float AvgLeavesBlockDepth = nanf("");
    float MedLeavesBlockDepth = nanf("");
    float MaxLeavesBlockDepth = nanf("");
    ui32 FakeGuidIdx = BAD_ID;
    TBoolOption NonEmptyBlockDeeper;
    TAutoPtr<TBlockStats> BlockStats = nullptr;
};


using TFactors = TVector<float>;

struct TFactorNode {
    const NDomTree::IDomNode* Node;
    TFactorNode* Parent = nullptr;
    TFactorNode* Next = nullptr;
    TFactorNode* FirstChild = nullptr;
    TBoolOption Splittable;
    ui32 MergeId = BAD_ID;
    ui32 TypeId = BAD_ID;
    TAutoPtr<TFactors> SplitFactors = nullptr;
    TAutoPtr<TFactors> MergeFactors = nullptr;
    TAutoPtr<TFactors> AnnotateFactors = nullptr;
    TMetaFactors MetaFactors;

    TFactorNode(const NDomTree::IDomNode* node)
        : Node(node)
    {}

    bool IsEmpty();
    TStringBuf GetGuid() const;

private:
    TBoolOption Empty;
};

}  // NSegmentator
