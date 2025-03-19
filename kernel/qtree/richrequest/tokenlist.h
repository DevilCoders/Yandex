#pragma once

#include "richnode.h"

#include <util/generic/hash.h>
#include <util/generic/vector.h>


// Adaptor over rich tree: single-level vector of nodes, base (reference) tokens of request.
// Could be used for addressing arbitrary node intervals in richtree via indexes of base tokens.

class TTokenList {
public:
    TTokenList(const TRichRequestNode& root);

    // non-copyable
    TTokenList(const TTokenList&) = delete;
    TTokenList& operator=(const TTokenList&) = delete;

    // but moveable
    TTokenList(TTokenList&& o) noexcept {
        this->Swap(o);
    }

    TTokenList& operator=(TTokenList&& o) noexcept {
        this->Swap(o);
        return *this;
    }



    // Length of token list (number of base tokens)
    size_t Size() const {
        return BaseTokens.size();
    }

    const TRichRequestNode& operator[] (size_t index) const {
        return *BaseTokens[index].Node;
    }


    const TRichRequestNode& Root() const {
        return *Root_;
    }

    const TRichRequestNode* Parent(size_t index) const {
        return BaseTokens[index].Parent;    // null for root
    }

    const TRichRequestNode* FindParent(const TRichRequestNode& node) const {
        const TNodeInfo* info = FindInfo(node);
        return info != nullptr ? info->Parent : nullptr;
    }

public: // Utility methods

    // implicitly castable to TVector<const TRichRequestNode*>
    operator const TConstNodesVector& () const {
        Y_ASSERT(BaseNodes.size() == Size());
        return BaseNodes;
    }

    void Swap(TTokenList& o) noexcept {
        ::DoSwap(Root_, o.Root_);
        ::DoSwap(BaseTokens, o.BaseTokens);
        ::DoSwap(BaseNodes, o.BaseNodes);
        ::DoSwap(NodeIndex, o.NodeIndex);
    }

    // just print base tokens debug strings:
    // e.g. "г. санкт-петербург" -> ["г", "санкт", "петербург"]
    TString DebugString() const;


public: // Spans

    struct TSpan {
        size_t Begin = 0, End = 0;
        bool IsExactBeg = true, IsExactEnd = true;        // if this span addresses whole base token

        TSpan() {
        }

        TSpan(size_t begin, size_t end)
            : Begin(begin)
            , End(end)
        {
        }

        bool operator == (const TSpan& span) const {
            return Begin == span.Begin && End == span.End;
        }

        bool operator < (const TSpan& span) const {
            return (Begin != span.Begin) ? Begin < span.Begin : End < span.End;
        }

        bool IsWhole() const {
            return IsExactBeg && IsExactEnd;
        }

        void Merge(const TSpan& span);
    };

    // Token span of length 1, by it index (from 0 to Size()-1)
    const TSpan& TokenSpan(size_t index) const {
        return BaseTokens[index].Tokens;
    }

    // Find token span corresponding to specified @node (or NULL if @node is not from current tree)
    const TSpan* TokenSpan(const TRichRequestNode& node) const;

    // Map node range to token span, return false if impossible for some reason
    // In exact mode sub-tokens are not allowed to be rounded up to closest base-token.
    bool Map(const TRichRequestNode& node, TSpan& tokens, bool exact = false) const;
    bool Map(const TRichNodePtr* begin, const TRichNodePtr* end, TSpan& tokens, bool exact = false) const;
    bool Map(const TRichRequestNode& parent, const NSearchQuery::TRange& range, TSpan& tokens, bool exact = false) const;


    TSpan WholeSpan() const {
        return TSpan(0, Size());
    }

    bool IsWholeRequest(const TSpan& span) const {
        return span == WholeSpan();
    }


private:
    enum ENodeLevel {
        SUB_TOKEN = 0,       // child of base token node, fragment
        BASE_TOKEN = 1,      // base token (base element of tokenization)
        SUPER_TOKEN = 2      // group of adjacent base tokens
    };

    struct TNodeInfo {
        const TRichRequestNode* Node;
        const TRichRequestNode* Parent;
        TSpan Tokens;       // in terms of BaseTokens indices
        ENodeLevel Level;

        TNodeInfo(const TRichRequestNode* node = nullptr, const TRichRequestNode* parent = nullptr,
                  ENodeLevel level = BASE_TOKEN, size_t begToken = 0, size_t endToken = 0,
                  bool isBeg = true, bool isEnd = true);
    };

    typedef THashMap<const TRichRequestNode*, TNodeInfo> TNodeIndex;

private:
    const TNodeInfo* FindInfo(const TRichRequestNode& node) const;

    void CollectInfo(const TRichRequestNode& node, ENodeLevel level,
                     const TRichRequestNode* parent, bool isBeg, bool isEnd);

private:
    const TRichRequestNode* Root_;
    TVector<TNodeInfo> BaseTokens;
    TConstNodesVector BaseNodes;
    TNodeIndex NodeIndex;
};
