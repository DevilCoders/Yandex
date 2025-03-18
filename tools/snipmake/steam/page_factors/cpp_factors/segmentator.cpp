#include "segmentator.h"

#include "annotate_applicator.h"
#include "factor_node.h"
#include "merge_applicator.h"
#include "split_applicator.h"

namespace NSegmentator {

static void DumpFactorNode(IOutputStream& out, ui32 ans, const TFactorNode& node,
        const TString& url, const TFactors& factors)
{
    TStringBuf guid = node.GetGuid();
    if (!guid.empty()) {
        out << guid;
    } else {
        out << "UNK";
    }
    out << "\t" << ans << "\t" << url << "\t" << "0";
    for (float factor : factors) {
        out << "\t" << factor;
    }
    out << Endl;
}


// TSegmentator
void TSegmentator::InitDomTreePtr(NDomTree::TDomTreePtr domTreePtr) {
    DomTreePtr = domTreePtr;
}

const TVector<TPageSegment>& TSegmentator::GetOrCalcSegments() {
    if (!Segments.Get())
        Segmentate();
    return *Segments.Get();
}

void TSegmentator::DumpSplitFactors(IOutputStream& out) {
    ResetFactorTree();
    TSplitApplicator splitter(*FactorTree, MnSplit, SplitResultBorder);
    splitter.Apply(false);
    TSplitNodeIterator splitNodes(*FactorTree);
    for (TFactorNode& node : splitNodes) {
        DumpFactorNode(out, 0, node, Url, *node.SplitFactors);
    }
}

void TSegmentator::DumpMergeFactors(IOutputStream& out) {
    ResetFactorTree();
    TSplitApplicator splitter(*FactorTree, MnSplit, SplitResultBorder);
    splitter.Apply();

    TMergeApplicator merger(*FactorTree, MnMerge, MergeResultBorder);
    merger.Apply(false);
    TMergeNodeIterator mergeNodes(*FactorTree);
    for (TFactorNode& node : mergeNodes) {
        DumpFactorNode(out, 0, node, Url, *node.MergeFactors);
    }
}

void TSegmentator::DumpAnnotateFactors(IOutputStream& out) {
    ResetFactorTree();
    TSplitApplicator splitter(*FactorTree, MnSplit, SplitResultBorder);
    splitter.Apply();

    TMergeApplicator merger(*FactorTree, MnMerge, MergeResultBorder);
    merger.Apply();

    TAnnotateApplicator annotator(*FactorTree, MnAnnotate);
    annotator.Apply(false);
    TAnnotateNodeIterator annotateNodes(*FactorTree);
    for (TFactorNode& node : annotateNodes) {
        DumpFactorNode(out, 0, node, Url, *node.AnnotateFactors);
    }
}

void TSegmentator::ResetFactorTree() {
    FactorTree.Reset(new TFactorTree(DomTreePtr));
}

void TSegmentator::DumpAwareSplitFactors(const NJson::TJsonValue& correctSegmentation, IOutputStream& out) {
    ResetFactorTree();
    TAwareSplitApplicator splitter(*FactorTree, MnSplit, SplitResultBorder, correctSegmentation);
    splitter.Apply(true);
    TSplitNodeIterator splitNodes(*FactorTree);
    for (TFactorNode& node : splitNodes) {
        ui32 ans = (node.Splittable.Get() ? 1 : 0);
        DumpFactorNode(out, ans, node, Url, *node.SplitFactors);
    }
}

void TSegmentator::DumpAwareMergeFactors(const NJson::TJsonValue& correctSegmentation, IOutputStream& out) {
    ResetFactorTree();
    TAwareSplitApplicator splitter(*FactorTree, MnSplit, SplitResultBorder, correctSegmentation);
    splitter.Apply(true);

    TAwareMergeApplicator merger(*FactorTree, MnMerge, MergeResultBorder, correctSegmentation);
    merger.Apply(true);
    TMergeNodeIterator mergeNodes(*FactorTree);
    for (TFactorNode& node : mergeNodes) {
        ui32 ans = node.MergeId;
        DumpFactorNode(out, ans, node, Url, *node.MergeFactors);
    }
}

void TSegmentator::DumpAwareAnnotateFactors(const NJson::TJsonValue& correctSegmentation,
                                            const TMap<TString, ui32>& classes2Codes, IOutputStream& out)
{
    ResetFactorTree();
    TAwareSplitApplicator splitter(*FactorTree, MnSplit, SplitResultBorder, correctSegmentation);
    splitter.Apply(true);

    TAwareMergeApplicator merger(*FactorTree, MnMerge, MergeResultBorder, correctSegmentation);
    merger.Apply(true);

    TAwareAnnotateApplicator annotator(*FactorTree, MnAnnotate, correctSegmentation, classes2Codes);
    annotator.Apply(true);
    TAnnotateNodeIterator annotateNodes(*FactorTree);
    for (TFactorNode& node : annotateNodes) {
        ui32 ans = node.TypeId;
        DumpFactorNode(out, ans, node, Url, *node.AnnotateFactors);
    }
}

void TSegmentator::Segmentate()
{
    ResetFactorTree();
    Segments.Reset(new TVector<TPageSegment>());

    TSplitApplicator splitter(*FactorTree, MnSplit, SplitResultBorder);
    splitter.Apply();

    TMergeApplicator merger(*FactorTree, MnMerge, MergeResultBorder);
    merger.Apply();

    TAnnotateApplicator annotator(*FactorTree, MnAnnotate);
    annotator.Apply();
    TAnnotateSegmentTraverser annotateSegmentsTraverser(*FactorTree);
    TPageSegment seg = annotateSegmentsTraverser.Next();
    while (!seg.Empty()) {
        Segments.Get()->push_back(seg);
        seg = annotateSegmentsTraverser.Next();
    }
}

}  // NSegmentator
