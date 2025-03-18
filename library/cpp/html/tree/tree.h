#pragma once

#include <deque>
#include <library/cpp/html/face/parsface.h>
#include <library/cpp/html/face/event.h>
#include <library/cpp/html/storage/storage.h>
#include <util/memory/pool.h>
#include <util/generic/ptr.h>

namespace NHtmlTree {
    enum ENodeType {
        NODE_ELEMENT,
        NODE_TEXT,
        NODE_IRREG,
        NODE_MARKUP,
    };

    struct TTextNode;
    struct TElementNode;
    class TTree;

    struct TNode: public TPoolable {
    public:
        ENodeType Type;
        const THtmlChunk* Event;
        TElementNode* Parent;

    public:
        TNode(ENodeType type, TElementNode* p, const THtmlChunk* e = nullptr)
            : Type(type)
            , Event(e)
            , Parent(p)
        {
        }

        TElementNode* ToElementNode() {
            Y_ASSERT(Type == NODE_ELEMENT);
            return (TElementNode*)this;
        }
        const TElementNode* ToElementNode() const {
            Y_ASSERT(Type == NODE_ELEMENT);
            return (const TElementNode*)this;
        }
        TTextNode* ToTextNode() {
            Y_ASSERT(Type == NODE_TEXT);
            return (TTextNode*)this;
        }
        const TTextNode* ToTextNode() const {
            Y_ASSERT(Type == NODE_TEXT);
            return (const TTextNode*)this;
        }
    };

    struct TTextNode: public TNode {
    public:
        TTextNode(TElementNode* p, const THtmlChunk* e)
            : TNode(NODE_TEXT, p, e)
        {
        }
    };

    struct TElementNode: public TNode {
    public:
        typedef TPoolAlloc<TNode*> TAllocator;
        // TDeque lacks constructor by allocator
        // typedef std::deque<TNode*, TPoolAllocator > TChildren;
        // we should use more efficient data structure - vector is waste here
        typedef TVector<TNode*, TAllocator> TChildren;
        typedef TChildren::const_iterator TConstIterator;
        typedef TChildren::iterator TIterator;

    public:
        TChildren Children;
        const THtmlChunk* CloseEvent = nullptr;

    public:
        TElementNode(TElementNode* p, const THtmlChunk* e, TTree* t); // use tree's alloc
        TElementNode(TElementNode* p, const THtmlChunk* e, TAllocator* alloc)
            : TNode(NODE_ELEMENT, p, e)
            , Children(*alloc)
        {
        }

        void Add(TNode* n) {
            Children.push_back(n);
        }
        TIterator Begin() {
            return Children.begin();
        }
        TConstIterator Begin() const {
            return Children.begin();
        }
        TIterator End() {
            return Children.end();
        }
        TConstIterator End() const {
            return Children.end();
        }
    };

    using TNodeIter = TElementNode::TConstIterator;

    class TMetadata {
        /// @todo charset, url, frames, ...
    };

    class TTree {
        typedef TMemoryPool TStorage;

        THolder<TStorage> Storage;
        TElementNode::TAllocator NodeAllocator;

        TElementNode Root;
        TMetadata Metadata;
        NHtml::TStorage HtmlStorage;

    public:
        TTree()
            : Storage(new TMemoryPool(8 << 20))
            , NodeAllocator(Storage.Get())
            , Root(TElementNode(nullptr, nullptr, this))
        {
        }

        TElementNode* RootNode() {
            return &Root;
        }
        const TElementNode* RootNode() const {
            return &Root;
        }
        TMetadata* GetMetadata() {
            return &Metadata;
        }
        const TMetadata* GetMetadata() const {
            return &Metadata;
        }
        NHtml::TStorage& GetHtmlStorage() {
            return HtmlStorage;
        }

        // storage details
        TStorage* GetStorage() const {
            return Storage.Get();
        }
        TElementNode::TAllocator* GetAllocator() {
            return &NodeAllocator;
        };
    };

}
