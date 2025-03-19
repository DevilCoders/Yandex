#include "structfinder.h"
#include "doc_node.h"
#include "segmentator.h"
#include <util/generic/algorithm.h>
#include <util/generic/hash_set.h>


namespace NSegm {
namespace NPrivate {

static bool LeftOrContains(const TVector<TDocNode* >& l1, const TVector<TDocNode* >& l2) {
    if (l1.size() && l2.size()) {
        int l1Start = (*l1.begin())->NodeStart.Sent();
        int l1End = l1.back()->NodeEnd.Sent();
        int l2Start = (*l2.begin())->NodeStart.Sent();
        int l2End = l2.back()->NodeEnd.Sent();
        if (l1Start < l2Start) {
            return true;
        }
        if (l1Start == l2Start && l1End > l2End) {
            return true;
        }
        return false;
    } else if (l1.size() && !l2.size()) {
        return true;
    } else if (l2.size() && !l1.size()) {
        return false;
    }
    return false;
}

static bool CheckNode(const TDocNode* node) {
    return !IsInputTag(node->Props.Tag) && !IsListLeafTag(node->Props.Tag) &&
           !IsTableLeafTag(node->Props.Tag) && node->Props.Tag != HT_TR &&
           node->Props.Tag != HT_P && (node->NodeEnd.Sent() - node->NodeStart.Sent());
}



static void HasInnerTable(TDocNode* node, bool& has) {
    if (node->Type == DNT_BLOCK) {
        if (node->Props.Tag == HT_TABLE) {
            has = true;
        } else {
            for (TDocNode::iterator it = node->Begin(); it != node->End(); ++it) {
                HasInnerTable(&*it, has);
                if (has) {
                    break;
                }
            }
        }
    }
}

static void FillSpans(const TVector<TDocNode* >& srcSpans, TSpans& dstSpans) {
    for (size_t i = 0; i < srcSpans.size(); i++) {
        if (srcSpans[i]) {
            TSpan span(srcSpans[i]->NodeStart.SentAligned(), srcSpans[i]->NodeEnd.SentAligned());
            if (!span.NoSents()) {
                dstSpans.push_back(span);
            }
        }
    }
    Sort(dstSpans.begin(), dstSpans.end());
}

void TStructFinder::UpdateGroup() {
    if (!Repeats.size()) {
        Repeats.push_back(TVector<TDocNode* >());
        return;
    }
    if (Repeats.back().size()) {
        Repeats.push_back(TVector<TDocNode* >());
    }
}

void TStructFinder::Traverse(TDocNode* node) {
    UpdateGroup();
    TDocNode* prev = nullptr;
    const TDocNode* last = node->Back();
    bool prevAdded = false;
    for (TDocNode::iterator it = node->Begin(); it != node->End(); ++it) {
        TDocNode* child = &*it;
        if (child->Props.Tag == HT_TABLE && child->Type == DNT_BLOCK) {
            ExplicitTables.push_back(child);
        }
        if (IsListRootTag(child->Props.Tag) && child->Type == DNT_BLOCK) {
            ExplicitLists.push_back(child);
        }
        if (child->Type == DNT_BLOCK && prev) {
            if (prev->Props.Signature == child->Props.Signature && CheckNode(prev) && CheckNode(child)) {
                prevAdded = true;
                Repeats.back().push_back(prev);
                if (child == last) {
                    Repeats.back().push_back(child);
                }
            } else {
                if (prevAdded && CheckNode(prev)) {
                    Repeats.back().push_back(prev);
                    UpdateGroup();
                }
                prevAdded = false;
            }
        }
        prev = child;
    }
    for (TDocNode::iterator it = node->Begin(); it != node->End(); ++it) {
        Traverse(&*it);
    }
}

void TStructFinder::FindCells(TDocNode* row) {
    for (TDocNode::iterator it = row->Begin(); it != row->End(); ++it) {
        TDocNode* c = &*it;
        if (c->Type == DNT_BLOCK && IsTableLeafTag(c->Props.Tag)) {
            TableCells.push_back(c);
        }
    }
}

void TStructFinder::FindRows(TDocNode* node) {
    if (node->Type == DNT_BLOCK) {
        if (node->Props.Tag == HT_TR) {
            TableRows.push_back(node);
            FindCells(node);
        } else {
            for (TDocNode::iterator it = node->Begin(); it != node->End(); ++it) {
                FindRows(&*it);
            }
        }
    }
}

void TStructFinder::FilterTables() {
    for (size_t i = 0; i < ExplicitTables.size(); i++) {
        TDocNode* table = ExplicitTables[i];
        bool hasInnerTable = false;
        for (TDocNode::iterator it = table->Begin(); it != table->End(); ++it) {
            HasInnerTable(&*it, hasInnerTable);
        }
        if (hasInnerTable || (table->NodeStart.Sent() == table->NodeEnd.Sent())) {
            ExplicitTables[i] = nullptr;
        } else {
            for (TDocNode::iterator it = table->Begin(); it != table->End(); ++it) {
                FindRows(&*it);
            }
        }
    }
}

void TStructFinder::FillCtx() {
    // fill lists
    for (size_t i = 0; i < Repeats.size(); i++) {
        const size_t sz  = Repeats[i].size();
        if (sz) {
            TDocNode* first = Repeats[i].front();
            TDocNode* last  = Repeats[i].back();
            Ctx->ListSpans.push_back(TTypedSpan(TTypedSpan::ST_LIST, ListDepth[i], first->NodeStart.SentAligned(), last->NodeEnd.SentAligned()));
            for (size_t j = 0; j < sz; j++) {
                TTypedSpan liSpan(TTypedSpan::ST_LIST_ITEM, ListDepth[i], Repeats[i][j]->NodeStart.SentAligned(), Repeats[i][j]->NodeEnd.SentAligned());
                if (!liSpan.NoSents()) {
                    Ctx->ListItemSpans.push_back(liSpan);
                }
            }
        }
    }
    Sort(Ctx->ListSpans.begin(), Ctx->ListSpans.end());
    Sort(Ctx->ListItemSpans.begin(), Ctx->ListItemSpans.end());

    // fill tables
    FillSpans(ExplicitTables, Ctx->TableSpans);
    FillSpans(TableRows, Ctx->TableRowSpans);
    FillSpans(TableCells, Ctx->TableCellSpans);
}

void TStructFinder::MergeExplicitLists() {
    for (size_t i = 0; i < ExplicitLists.size(); i++) {
        TDocNode* list = ExplicitLists[i];
        if (list->NodeEnd.Sent() > list->NodeStart.Sent()) {
            UpdateGroup();
            for (TDocNode::iterator it = list->Begin(); it != list->End(); ++it) {
                TDocNode* li = &*it;
                if (li->NodeEnd.Sent() > li->NodeStart.Sent()) {
                    Repeats.back().push_back(li);
                }
            }
        }
    }
}

void TStructFinder::FilterRepeats() {
    THashSet<TDocNode*> tableSet(ExplicitTables.size());
    tableSet.insert(ExplicitTables.begin(), ExplicitTables.end());

    for (size_t i = 0; i < Repeats.size(); i++) {
        if (Repeats[i].size() == 1) {
            Repeats[i].clear();
        } else {
            bool hasInner = false;
            for (size_t j = 0; j < Repeats[i].size(); j++) {
                TDocNode* n = Repeats[i][j];
                if (n->Type == DNT_BLOCK && n->Props.Tag == HT_TABLE) {
                    hasInner |= !(tableSet.find(n) != tableSet.end());
                }
            }
            if (hasInner) {
                Repeats[i].clear();
            }
        }
    }
}

struct TListDepth {
    TVector<int> Depth;

    void Put(int first, int second) {
        while (Depth.size() && Depth.back() <= first) {
            Depth.pop_back();
        }
        Depth.push_back(second);
    }

    size_t GetDepth() {
        Y_ASSERT(Depth.size() != 0);
        return Depth.size() - 1;
    }
};

void TStructFinder::DetectInnerLists() {
    Sort(Repeats.begin(), Repeats.end(), LeftOrContains);
    ListDepth.resize(Repeats.size(), 0);
    TListDepth depthStack;
    size_t maxLevel = 0;

    for (size_t i = 0; i < Repeats.size(); i++) {
        if (Repeats[i].size()) {
            depthStack.Put(Repeats[i].front()->NodeStart.Sent(), Repeats[i].back()->NodeEnd.Sent());
            ListDepth[i] = depthStack.GetDepth();
            maxLevel = Max(maxLevel, ListDepth[i]);
            if (maxLevel > 5) {
                Repeats.clear();
                break;
            }
        }
    }
}

TStructFinder::TStructFinder(TSegContext* ctx) : Ctx(ctx) {}

void TStructFinder::FindAndFillCtx(TDocNode* node) {
    Repeats.clear();
    ExplicitLists.clear();
    ExplicitTables.clear();
    TableRows.clear();
    TableCells.clear();
    ListDepth.clear();

    Traverse(node);
    FilterTables();
    MergeExplicitLists();
    FilterRepeats();
    DetectInnerLists();
    FillCtx();
}

}
}
