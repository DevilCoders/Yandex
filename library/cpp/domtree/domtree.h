#pragma once

#include "decl.h"
#include "iterators.h"

#include <library/cpp/html/spec/tags.h>
#include <library/cpp/html/face/parstypes.h>
#include <library/cpp/wordpos/wordpos.h>

#include <util/generic/strbuf.h>
#include <util/generic/ptr.h>

namespace NDomTree {
    struct IDomAttr {
        virtual TStringBuf Name() const = 0;
        virtual TStringBuf Value() const = 0;
        virtual bool IsBoolean() const = 0;
        virtual ui64 Line() const { return 0; }
        virtual ui64 Column() const { return 0; }
        virtual ~IDomAttr() = default;
    };

    class IDomNode;

    using IAttrTraverser = IAbstractTraverser<const IDomAttr*>;
    using INodeTraverser = IAbstractTraverser<const IDomNode*>;
    using TNodeIterator = TDomIterator<INodeTraverser>;

    template <class TNode>
    class TAttrRange: public TInputRangeAdaptor<TAttrRange<TNode>> {
        const TNode* Parent;
        size_t Index = 0;
        const size_t Count;

    public:
        TAttrRange(const TNode* parent)
            : Parent(parent)
            , Count(parent->AttrCount())
        {
        }

        const IDomAttr* Next() {
            if (Parent && Index < Count) {
                return Parent->GetAttr(Index++);
            } else {
                return nullptr;
            }
        }
    };

    template <class TNode>
    class TChildNodeRange: public TInputRangeAdaptor<TChildNodeRange<TNode>> {
        const TNode* Node;

    public:
        TChildNodeRange(const TNode* parent)
            : Node(parent->FirstChild())
        {
        }

        const TNode* Next() {
            if (Node) {
                const TNode* retval = Node;
                Node = Node->Next();
                return retval;
            } else {
                return nullptr;
            }
        }
    };

    struct INodeText;

    class IDomNode {
    public:
        // node properties
        virtual ui32 Id() const = 0;
        virtual EDomNodeType Type() const = 0;
        virtual bool IsElement() const = 0;
        virtual bool IsText() const = 0;
        virtual HT_TAG Tag() const = 0;
        virtual TStringBuf TagName() const = 0;
        virtual TEXT_WEIGHT Weight() const = 0;
        virtual TWtringBuf Text() const = 0;
        virtual TPosting PosBeg() const = 0;
        virtual TPosting PosEnd() const = 0;
        virtual ui64 Line() const { return 0; }
        virtual ui64 Column() const { return 0; }
        // attrs
        virtual size_t AttrCount() const = 0;
        virtual const IDomAttr* GetAttr(size_t index) const = 0;
        virtual bool HasAttr(const TStringBuf& attrName) const = 0;
        virtual TStringBuf Attr(const TStringBuf& attrName) const = 0;

        TAttrRange<IDomNode> Attrs() const {
            return TAttrRange<IDomNode>(this);
        }

        // siblings
        virtual const IDomNode* Next() const = 0;
        virtual const IDomNode* Prev() const = 0;
        virtual const IDomNode* Parent() const = 0;
        // children
        virtual size_t ChildCount() const = 0;
        virtual const IDomNode* FirstChild() const = 0;
        virtual const IDomNode* GetChild(size_t idx) const = 0;

        TChildNodeRange<IDomNode> Children() const {
            return TChildNodeRange<IDomNode>(this);
        }

        // text
        virtual const INodeText* NodeText() const = 0;

        virtual ~IDomNode() = default;
    };

    class IDomTree {
    public:
        virtual const IDomNode* GetRoot() const = 0;
        virtual ui32 NodeCount() const = 0;
        virtual TNodeIterator PreorderTraversal() const = 0;
        virtual ~IDomTree() = default;
    };

}
