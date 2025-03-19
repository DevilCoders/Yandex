#pragma once

#include <kernel/qtree/richrequest/richnode.h>

#include <util/generic/vector.h>
#include <util/generic/hash.h>
#include <util/memory/pool.h>


namespace NGzt {

// richnode + parent + marks
class TSuperNode {
public:
    TRichNodePtr Node;
    const TSuperNode* Parent;
    size_t ChildIndex;      // index of this node in @Parent->Children

    TVector<const TSuperNode*> Children;

    bool IsMark;        // True if this is not a direct node of original tree but a mark node from synonyms.
                        // In this case Children are not empty and contains the mark pieces (leaf-nodes from original tree)

public:
    bool IsSmall() const {
        // essentially a leaf node
        return IsWordInfoNode(*Node) && Children.empty() && !IsAsciiEmojiPart(Node->GetText());
    }

    bool IsBig() const {
        // essentially a node with children
        return IsWordInfoNode(*Node) && !Children.empty() && !IsAsciiEmojiPart(Node->GetText());
    }


    const TSuperNode* GetFirstSmall() const;
    const TSuperNode* GetLastSmall() const;

    bool StartsBig() const;
    bool EndsBig() const;


    TString DebugString() const;

    bool IsEqual(const TSuperNode& sn) const {
        return HasSameId(*sn.Node);
    }

    bool HasSameId(const TRichRequestNode& node) const {
        return Node->GetId() == node.GetId();
    }


    class TVectorType: public TAtomicRefCount<TVectorType> {
    public:
        size_t Size() const {
            return Super.size();
        }

        const TSuperNode* operator[](size_t i) const {
            return Super[i];
        }

        const TVector<TRichNodePtr>& RichNodes() const {
            return Rich;
        }

        void PushBack(const TSuperNode& node) {
            Super.push_back(&node);
            Rich.push_back(node.Node);
        }

        void Clear() {
            Super.clear();
            Rich.clear();
        }

        void Swap(TVectorType& that) {
            Super.swap(that.Super);
            Rich.swap(that.Rich);
        }

        bool LocateSmallSpan(const TSuperNode& first, const TSuperNode& last, size_t& begin, size_t& end) const;
        bool LocateAnySpan(const TSuperNode& first, const TSuperNode& last, size_t& begin, size_t& end) const;

        bool LocateHere(const TVectorType& vec, size_t& begin) const;

    private:
        bool LocateHere(const TSuperNode& node, size_t& index, size_t startFrom = 0) const;
        bool LocateHere(const TSuperNode& first, const TSuperNode& last, size_t& begin, size_t& end, size_t startFrom = 0) const;

        template <bool left>
        bool LocateEdge(const TSuperNode& node, size_t& index, size_t startFrom = 0) const;

    private:
        TVector<const TSuperNode*> Super;
        TVector<TRichNodePtr> Rich;
    };


    // Indexer for super-nodes: mapping from TRichNode to super node
    class TIndex: public TAtomicRefCount<TVectorType> {
    public:
        ~TIndex() {
            Clear();
        }

        const TSuperNode* Get(const TRichRequestNode* node) const {
            THash::const_iterator it = Index.find(node->GetId());
            return it != Index.end() ? it->second : nullptr;
        }

        const TSuperNode& GetOrInsert(TRichNodePtr root) {
            return *InsertInt(root, nullptr, 0);
        }

        const TSuperNode& GetOrInsert(const TRichRequestNode& root) {
            return GetOrInsert(TRichNodePtr(const_cast<TRichRequestNode*>(&root)));
        }

        void Clear();

    private:
        TSuperNode* InsertInt(TRichNodePtr node, const TSuperNode* parent, size_t childIndex);
        void ProcessChildren(TSuperNode& info);
        void ProcessSynonyms(TSuperNode&);

    private:
        typedef THashMap<long, TSuperNode*> THash;      // owns all super-nodes
        THash Index;
    };

private:
    // created only via TIndex
    TSuperNode(TRichNodePtr node, const TSuperNode* parent = nullptr, size_t childIndex = 0)
        : Node(node)
        , Parent(parent)
        , ChildIndex(childIndex)
        , IsMark(false)
    {
    }
};

typedef TSuperNode::TVectorType TSuperNodeVector;
typedef TSuperNode::TIndex TSuperNodeIndex;


}   // namespace NGzt
