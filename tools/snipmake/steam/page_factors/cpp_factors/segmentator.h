#pragma once

#include "factor_tree.h"
#include "page_segment.h"

#include <kernel/matrixnet/mn_multi_categ.h>
#include <kernel/matrixnet/mn_sse.h>
#include <library/cpp/domtree/decl.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/output.h>

extern const NMatrixnet::TMnSseInfo staticMnSplit;
extern const NMatrixnet::TMnSseInfo staticMnMerge;
extern const NMatrixnet::TMnMultiCateg staticMnAnnotate;

// TODO: calibrate consts
static const double SPLIT_RESULT_BORDER = 0.0;
static const double MERGE_RESULT_BORDER = 0.0;

namespace NJson {
class TJsonValue;
}

namespace NSegmentator {

class TSegmentator {
public:
    TSegmentator(
            const TString& url,
            NDomTree::TDomTreePtr domTreePtr = nullptr,
            const NMatrixnet::TMnSseInfo& mnSplit = staticMnSplit,
            double splitResultBorder = SPLIT_RESULT_BORDER,
            const NMatrixnet::TMnSseInfo& mnMerge = staticMnMerge,
            double mergeResultBorder = MERGE_RESULT_BORDER,
            const NMatrixnet::TMnMultiCateg& mnAnnotate = staticMnAnnotate)
        : DomTreePtr(domTreePtr)
        , Url(url)
        , MnSplit(mnSplit)
        , SplitResultBorder(splitResultBorder)
        , MnMerge(mnMerge)
        , MergeResultBorder(mergeResultBorder)
        , MnAnnotate(mnAnnotate)
    {}

    void InitDomTreePtr(NDomTree::TDomTreePtr domTreePtr);
    const TVector<TPageSegment>& GetOrCalcSegments();
    void DumpSplitFactors(IOutputStream& out = Cout);
    void DumpMergeFactors(IOutputStream& out = Cout);
    void DumpAnnotateFactors(IOutputStream& out = Cout);

    void DumpAwareSplitFactors(const NJson::TJsonValue& correctSegmentation, IOutputStream& out = Cout);
    void DumpAwareMergeFactors(const NJson::TJsonValue& correctSegmentation, IOutputStream& out = Cout);
    void DumpAwareAnnotateFactors(const NJson::TJsonValue& correctSegmentation,
                                  const TMap<TString, ui32>& classes2Codes, IOutputStream& out = Cout);
private:
    void ResetFactorTree();
    void Segmentate();

private:
    THolder<TFactorTree> FactorTree = nullptr;
    NDomTree::TDomTreePtr DomTreePtr;
    TString Url;
    const NMatrixnet::TMnSseInfo& MnSplit;
    const double SplitResultBorder;
    const NMatrixnet::TMnSseInfo& MnMerge;
    const double MergeResultBorder;
    const NMatrixnet::TMnMultiCateg& MnAnnotate;
    THolder<TVector<TPageSegment>> Segments;
};

}  // NSegmentator
