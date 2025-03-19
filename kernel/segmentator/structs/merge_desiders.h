#pragma once

#include "classification.h"

namespace NSegm {

template<ui8 Level, EBlockStepType BlockStepType=BST_BLOCK, ui8 BlockLevel=Level>
struct TNodeMergeDesiderBase {
    static bool DesideMergePrev(TBlockDist pdist, TBlockDist ndist) {
        return pdist.Depth < ndist.Depth;
    }

    static bool DesideMergeNext(TBlockDist pdist, TBlockDist ndist) {
        return !DesideMergePrev(pdist, ndist);
    }

    static bool AcceptMergePrev(const TSegmentSpan * /*node*/, const TSegmentSpan * /*prev*/,
            TBlockDist pdist) {
        return AcceptMerge(pdist);
    }

    static bool AcceptMergeNext(const TSegmentSpan * /*node*/, const TSegmentSpan * /*next*/,
            TBlockDist ndist) {
        return AcceptMerge(ndist);
    }

    static bool AllowMerge(const TSegmentSpan * node) {
        return node;
    }

protected:
    static bool AcceptMerge(TBlockDist i) {
        switch (BlockStepType) {
        default:
            return i.Depth == 0;
        case BST_ITEM:
            return i.Blocks + i.Paragraphs == 0;
        case BST_PARAGRAPH:
            return i.Blocks == 0 && i.Paragraphs + i.Items <= Level;
        case BST_BLOCK:
            return i.Blocks + i.Paragraphs + i.Items <= Level && i.Blocks <= BlockLevel;
        }
    }
};

struct THeaderToHeaderMergeDesider: TNodeMergeDesiderBase<2, BST_ITEM> {
    typedef TNodeMergeDesiderBase<2, BST_ITEM> TParent;

    static bool AcceptMergePrev(const TSegmentSpan * node, const TSegmentSpan * prev, TBlockDist pdist) {
        return TParent::AcceptMergePrev(node, prev, pdist) && prev->IsHeader;
    }

    static bool AcceptMergeNext(const TSegmentSpan * node, const TSegmentSpan * next, TBlockDist ndist) {
        return TParent::AcceptMergeNext(node, next, ndist) && next->IsHeader;
    }

    static bool AllowMerge(const TSegmentSpan * node) {
        return TParent::AllowMerge(node) && node->IsHeader;
    }
};

template<ui8 Level, EBlockStepType BlockStepType=BST_BLOCK, ui8 BlockLevel = Level, bool AllowDupHeaders=true>
struct THeaderToNodeMergeDesider: TNodeMergeDesiderBase<Level, BlockStepType, BlockLevel> {
    typedef TNodeMergeDesiderBase<Level, BlockStepType, BlockLevel> TParent;

    static bool AcceptMergePrev(const TSegmentSpan *, const TSegmentSpan *, TBlockDist) {
        return false;
    }

    static bool AcceptMergeNext(const TSegmentSpan *, const TSegmentSpan * next, TBlockDist ndist) {
        return next && AcceptMerge(ndist) && !next->IsHeader && (AllowDupHeaders || !next->HasHeader);
    }

    static bool DesideMergeNext(TBlockDist, TBlockDist) {
        return true;
    }

    static bool AllowMerge(const TSegmentSpan * node) {
        return TParent::AllowMerge(node) && node->IsHeader;
    }

protected:
    static bool AcceptMerge(TBlockDist i) {
        switch (BlockStepType) {
        default:
            return i.Depth == 0;
        case BST_ITEM:
            return i.Blocks + i.Paragraphs == 0;
        case BST_PARAGRAPH:
            return i.Blocks == 0 && i.Paragraphs <= Level;
        case BST_BLOCK:
            return i.Blocks + i.Paragraphs <= Level && i.Blocks < BlockLevel;
        }
    }

};

template<ui8 Level, EBlockStepType BlockStepType = BST_BLOCK, ui8 BlockLevel=Level>
struct TNodeToNodeMergeDesider: TNodeMergeDesiderBase<Level, BlockStepType, BlockLevel> {
    typedef TNodeMergeDesiderBase<Level, BlockStepType, BlockLevel> TParent;

    static bool AcceptMergePrev(const TSegmentSpan * node, const TSegmentSpan * prev, TBlockDist pdist) {
        return TParent::AcceptMergePrev(node, prev, pdist) && !prev->IsHeader;
    }

    static bool AcceptMergeNext(const TSegmentSpan *, const TSegmentSpan *, TBlockDist) {
        return false;
    }

    static bool AllowMerge(const TSegmentSpan * node) {
        return TParent::AllowMerge(node) && !node->IsHeader;
    }
};

template<ui8 Level, EBlockStepType BlockStepType = BST_BLOCK, ui8 BlockLevel=Level>
struct TNodeToNodeTypedMergeDesider: TNodeToNodeMergeDesider<Level, BlockStepType, BlockLevel> {
    typedef TNodeMergeDesiderBase<Level, BlockStepType, BlockLevel> TParent;

    static bool AcceptMergePrev(const TSegmentSpan * node, const TSegmentSpan * prev, TBlockDist pdist) {
        return TParent::AcceptMergePrev(node, prev, pdist) && ClassifySpan(*prev) == ClassifySpan(*node);
    }
};

}
