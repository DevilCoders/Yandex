#include "segmentator.h"

namespace NSegm {
namespace NPrivate {

static void CollapseBreaks(TDocNode* node) {
    for (TDocNode::iterator it = node->Begin(); it != node->End(); ++it) {
        TDocNode * child = &*it;
        TDocNode * prev = TListAccessor<TDocNode>::GetPrev(it, node->Begin());
        TDocNode * next = TListAccessor<TDocNode>::GetNext(it, node->End());

        if (IsA<DNT_BLOCK> (child))
            CollapseBreaks(child);

        if (IsA<DNT_BREAK> (child)
                    && (child == node->Front()
                                || child == node->Back()
                                || (prev && IsA<DNT_BLOCK> (prev)) || (next && IsA<DNT_BLOCK> (next))
                       )
           )
        {
            it = TListAccessor<TDocNode>::EraseBack(it);
            continue;
        }

        if (IsA<DNT_BREAK> (child) && prev && IsA<DNT_BREAK> (prev)) {
            prev->MergeBreak(child);
            it = TListAccessor<TDocNode>::EraseBack(it);
            continue;
        }
    }
}

static void CollapseEmptyBlocks(TDocNode* node, TMemoryPool& pool) {
    for (TDocNode::iterator it = node->Begin(); it != node->End(); ++it) {
        TDocNode * child = &*it;
        TDocNode * prev = TNodeAccessor::GetPrev(it, node->Begin());
        TDocNode * next = TNodeAccessor::GetNext(it, node->End());

        if (!IsA<DNT_BLOCK> (child))
            continue;

        CollapseEmptyBlocks(child, pool);

        if (IsEmpty(child)) {
            if (TBL_INLINE == GetBreakLevel(child->Props.Tag)) {
                it = TNodeAccessor::EraseBack(it);
                continue;
            }

            if (prev && IsA<DNT_BREAK> (prev)) {
                prev->MergeBreak(child->Props.Tag);
                it = TNodeAccessor::EraseBack(it);
                continue;
            }

            if (next && IsA<DNT_BREAK> (next)) {
                next->MergeBreak(child->Props.Tag);
                it = TNodeAccessor::EraseBack(it);
                continue;
            }

            TDocNode * br = MakeBreak(pool, child->Node()->NodeStart, nullptr, child->Node()->Props.Tag);

            br->LinkBeforeNoUnlink(child);
            br->Parent = node;
            it = TNodeAccessor::EraseBack(it);
        }
    }
}

/*! Collapses redundant folding.
 * Intended to remove nonsignificant folding caused by styling-only markup.
 * @par
 * <div><div><div>foo</div></div></div> -> <div>foo</div>
 * <table><tr><td>bar</table> -> <div>bar</div>
 */
static void CollapseTelescopicHierarchy(TDocNode* node) {
    Y_VERIFY(!node->ListEmpty() || HT_any == node->Props.Tag, " ");

    if (node->ListEmpty())
        return;

    for (TDocNode::iterator it = node->Begin(); it != node->End(); ++it) {
        /* both Replace() and Collapse() do conceptually the same:
         <node><child:p0>   <block:p1>x y z</block></child></node>
         v
         <node><child:p0+p1>          x y z        </child></node>
         Replace() takes into consideration singular lists and tables
         Collapse() considers a general case of singular block structure
         */
        TDocNode * child = &*it;

        if (IsA<DNT_BLOCK> (child)) {
            if (child->IsReplaceable())
                child->Replace();
            if (child->IsCollapseable())
                child->Collapse();

            child->Props.NodeLevel = node->Props.NodeLevel + 1;
            CollapseTelescopicHierarchy(child);
        }
    }
}

void TSegmentator::ProcessTree() {
    CollapseBreaks(Root);
    CollapseEmptyBlocks(Root, Ctx->Pool);
    CollapseTelescopicHierarchy(Root);

}

}
}
