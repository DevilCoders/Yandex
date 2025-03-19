#include "supernode.h"

namespace NGzt {

using namespace NSearchQuery;

template <bool left>
static inline const TSuperNode* GetEdgeChild(const TSuperNode& node) {
    if (node.Children) {
        if (left)
            return node.Children[0];
        else
            return node.Children.back();
    }
    return nullptr;
}

template <bool left>
static inline const TSuperNode* GetEdgeSmall(const TSuperNode& node) {
    if (node.IsSmall())
        return &node;
    const TSuperNode* child = GetEdgeChild<left>(node);
    return child ? GetEdgeSmall<left>(*child) : nullptr;
}

const TSuperNode* TSuperNode::GetFirstSmall() const {
    return GetEdgeSmall<true>(*this);
}

const TSuperNode* TSuperNode::GetLastSmall() const {
    return GetEdgeSmall<false>(*this);
}

bool TSuperNode::StartsBig() const {
    if (!IsBig()) {
        bool all = true;
        for (const TSuperNode* node = this; node->Parent; node = node->Parent) {
            all = all && node->ChildIndex == 0;
            if (node->Parent->IsBig())
                return all;
        }
    }
    return true;    // no bigs above
}

bool TSuperNode::EndsBig() const {
    if (!IsBig()) {
        bool all = true;
        for (const TSuperNode* node = this; node->Parent; node = node->Parent) {
            all = all && node->ChildIndex + 1 == node->Parent->Children.size();
            if (node->Parent->IsBig())
                return all;
        }
    }
    return true;    // no bigs above
}

TString TSuperNode::DebugString() const {
    TStringStream str;
    str << Node->DebugString();
    return str.Str();
}

bool TSuperNode::TVectorType::LocateHere(const TSuperNode& node, size_t& index, size_t startFrom) const {
    for (size_t i = startFrom; i < Super.size(); ++i)
        if (Super[i]->IsEqual(node)) {
            index = i;
            return true;
        }
    return false;
}

bool TSuperNode::TVectorType::LocateHere(const TVectorType& vec, size_t& begin) const {
    size_t firstIndex = 0;
    if (vec.Size() == 0 || !LocateHere(*vec[0], firstIndex))
        return false;

    for (size_t i = 1; i < vec.Size(); ++i)
        if (firstIndex + i >= Size() || !Super[firstIndex + i]->IsEqual(*vec[i]))
            return false;

    begin = firstIndex;
    return true;
}

bool TSuperNode::TVectorType::LocateHere(const TSuperNode& first, const TSuperNode& last,
                                     size_t& begin, size_t& end, size_t startFrom) const {
    size_t firstIndex = 0, lastIndex = 0;
    if (!LocateHere(first, firstIndex, startFrom) || !LocateHere(last, lastIndex, firstIndex))
        return false;

    begin = firstIndex;
    end = lastIndex + 1;
    return true;
}

template <bool left>
bool TSuperNode::TVectorType::LocateEdge(const TSuperNode& node, size_t& index, size_t startFrom) const {
    if (LocateHere(node, index, startFrom))
        return true;

    for (const TSuperNode* p = node.Parent; p != nullptr; p = p->Parent)
        if (LocateHere(*p, index, startFrom))
            return true;

    for (const TSuperNode* n = GetEdgeChild<left>(node); n != nullptr; n = GetEdgeChild<left>(*n))
        if (LocateHere(*n, index, startFrom))
            return true;

    return false;
}

bool TSuperNode::TVectorType::LocateSmallSpan(const TSuperNode& first, const TSuperNode& last, size_t& begin, size_t& end) const {
    const TSuperNode* firstSmall = first.GetFirstSmall();
    const TSuperNode* lastSmall = last.GetLastSmall();
    return firstSmall && lastSmall && LocateHere(*firstSmall, *lastSmall, begin, end);
}

bool TSuperNode::TVectorType::LocateAnySpan(const TSuperNode& first, const TSuperNode& last, size_t& begin, size_t& end) const {
    size_t firstIndex = 0, lastIndex = 0;
    if (!LocateEdge<true>(first, firstIndex))               // left edge
        return false;
    if (!LocateEdge<false>(last, lastIndex, firstIndex))    // right edge
        return false;

    begin = firstIndex;
    end = lastIndex + 1;
    return true;
}

TSuperNode* TSuperNode::TIndex::InsertInt(TRichNodePtr node, const TSuperNode* parent, size_t childIndex) {
    TSuperNode* info = nullptr;
    std::pair<THash::iterator, bool> ins = Index.insert(std::make_pair(node->GetId(), info));
    if (ins.second) {
        info = new TSuperNode(node, parent, childIndex);
        ins.first->second = info;

        ProcessChildren(*info);
        ProcessSynonyms(*info);
    }

    return ins.first->second;
}

void TSuperNode::TIndex::Clear() {
    for (THash::iterator it = Index.begin(); it != Index.end(); ++it)
        delete it->second;
    Index.clear();
}

void TSuperNode::TIndex::ProcessChildren(TSuperNode& node) {
    for (size_t i = 0; i < node.Node->Children.size(); ++i)
        node.Children.push_back(InsertInt(node.Node->Children.MutableNode(i), &node, i));
}

static inline bool IsMarkSyn(const TMarkupItem& item) {
    return item.GetDataAs<TSynonym>().HasType(TE_MARK);
}

void TSuperNode::TIndex::ProcessSynonyms(TSuperNode& node) {
    // search for marks in synonyms
    if (node.Children) {
        // non-intersecting super-marks
        size_t lastMarkEnd = 0;
        TMarkup::TItems& synonyms = node.Node->MutableMarkup().GetItems<TSynonym>();
        TVector<const TSuperNode*> newChildren;
        for (size_t i = 0; i < synonyms.size(); ++i) {
            if (!IsMarkSyn(synonyms[i]))
                continue;
            TRichNodePtr mark = synonyms[i].GetDataAs<TSynonym>().SubTree;
            size_t start = synonyms[i].Range.Beg;
            size_t end = synonyms[i].Range.End + 1;
            if (start < lastMarkEnd || end > node.Children.size())
                continue;

            for (size_t i2 = lastMarkEnd; i2 < start; ++i2)
                newChildren.push_back(node.Children[i2]);

            TSuperNode* markNode = InsertInt(mark, &node, newChildren.size());
            markNode->IsMark = true;
            markNode->Children.clear();
            for (size_t i3 = start; i3 < end; ++i3) {
                TSuperNode* markChild = const_cast<TSuperNode*>(node.Children[i3]);
                markChild->Parent = markNode;
                markChild->ChildIndex = i3 - start;
                markNode->Children.push_back(markChild);
            }
            newChildren.push_back(markNode);
            lastMarkEnd = end;
        }
        // check if any super mark was found
        if (lastMarkEnd > 0) {
            for (size_t i = lastMarkEnd; i < node.Children.size(); ++i)
                newChildren.push_back(node.Children[i]);
            node.Children.swap(newChildren);
        }
    }
}


}   // namespace NGzt
