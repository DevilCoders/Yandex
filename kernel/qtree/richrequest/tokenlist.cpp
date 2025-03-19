#include "tokenlist.h"

TTokenList::TTokenList(const TRichRequestNode& root)
    : Root_(&root)
{
    CollectInfo(*Root_, SUPER_TOKEN, nullptr, true, true);
    BaseNodes.reserve(BaseTokens.size());
    for (const TNodeInfo& tok : BaseTokens)
        BaseNodes.push_back(tok.Node);
}

inline void TTokenList::CollectInfo(const TRichRequestNode& node, ENodeLevel level, const TRichRequestNode* parent,
                                    bool isBeg, bool isEnd)
{
    if (level == SUPER_TOKEN && IsWordInfoNode(node))
        level = BASE_TOKEN;

    size_t begToken = BaseTokens.size();

    ENodeLevel sublevel = (level == SUPER_TOKEN) ? SUPER_TOKEN : SUB_TOKEN;
    for (size_t i = 0; i < node.Children.size(); ++i) {
        bool isSubBeg = isBeg && (sublevel != SUB_TOKEN || i == 0);
        bool isSubEnd = isEnd && (sublevel != SUB_TOKEN || i + 1 == node.Children.size());
        CollectInfo(*node.Children[i], sublevel, &node, isSubBeg, isSubEnd);
    }

    size_t endToken = BaseTokens.size() + (level == SUPER_TOKEN ? 0 : 1);
    TNodeInfo info(&node, parent, level, begToken, endToken, isBeg, isEnd);

    if (level == BASE_TOKEN)
        BaseTokens.push_back(info);
    NodeIndex[&node] = info;
}

inline TTokenList::TNodeInfo::TNodeInfo(const TRichRequestNode* node, const TRichRequestNode* parent,
                                        ENodeLevel level, size_t begToken, size_t endToken,
                                        bool isBeg, bool isEnd)
    : Node(node)
    , Parent(parent)
    , Tokens(begToken, endToken)
    , Level(level)
{
    Tokens.IsExactBeg = isBeg;
    Tokens.IsExactEnd = isEnd;
}

const TTokenList::TSpan* TTokenList::TokenSpan(const TRichRequestNode& node) const {
    const TNodeInfo* info = FindInfo(node);
    return info != nullptr ? &info->Tokens : nullptr;
}


const TTokenList::TNodeInfo* TTokenList::FindInfo(const TRichRequestNode& node) const {
    TNodeIndex::const_iterator it = NodeIndex.find(&node);
    return it != NodeIndex.end() ? &it->second : nullptr;
}


void TTokenList::TSpan::Merge(const TSpan& span) {
    // begin
    if (span.Begin < Begin) {
        Begin = span.Begin;
        IsExactBeg = span.IsExactBeg;
    } else if (!IsExactBeg && span.Begin == Begin && span.IsExactBeg)
        IsExactBeg = true;

    // end
    if (span.End > End) {
        End = span.End;
        IsExactEnd = span.IsExactEnd;
    } else if (!IsExactEnd && span.End == End && span.IsExactEnd)
        IsExactEnd = true;
}


bool TTokenList::Map(const TRichRequestNode& node, TSpan& tokens, bool exact) const {
    const TSpan* span = TokenSpan(node);
    if (!span || (exact && !span->IsWhole()))
        return false;
    tokens = *span;
    return true;
}

bool TTokenList::Map(const TRichNodePtr* begin, const TRichNodePtr* end, TSpan& tokens, bool exact) const {
    if (begin == end)
        return false;

    TSpan ret;

    // leftmost token
    if (!Map(**begin, ret, false))
        return false;

    for (++begin; begin != end; ++begin) {
        const TSpan* span = TokenSpan(**begin);
        if (!span)
            return false;
        // extend borders
        ret.Merge(*span);
    }

    if (exact && !ret.IsWhole())
        return false;

    tokens = ret;
    return true;
}

bool TTokenList::Map(const TRichRequestNode& parent, const NSearchQuery::TRange& range, TSpan& tokens, bool exact) const {
    if (range.Beg <= range.End && range.Beg < parent.Children.size()) {
        size_t end = Min<size_t>(range.End + 1, parent.Children.size());
        return Map(parent.Children.begin() + range.Beg, parent.Children.begin() + end, tokens, exact);
    } else {
        return Map(parent, tokens, exact);
    }
}


TString TTokenList::DebugString() const {
    TStringStream out;
    out << "[";
    for (size_t i = 0; i < Size(); ++i) {
        if (i > 0)
            out << ", ";
        out << (*this)[i].DebugString();
    }
    out << "]";
    return out.Str();
}

