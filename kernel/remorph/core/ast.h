#pragma once

#include "literal.h"
#include "types.h"
#include "source_pos.h"

#include <util/generic/vector.h>
#include <util/generic/hash.h>
#include <util/generic/ptr.h>
#include <util/charset/wide.h>
#include <util/string/cast.h>

#define WRE_AST_NODE_TYPES_LIST                 \
    X(Submatch)                                 \
    X(Literal)                                  \
    X(Catenation)                               \
    X(Iteration)                                \
    X(Union)

namespace NRemorph {

template <class TData>
struct TTree {
    typedef TIntrusivePtr<TData> TDataPtr;

    struct TNode: public TSimpleRefCount< TNode > {
        typedef TIntrusivePtr< TNode > TPtr;
        typedef TIntrusivePtr<TData> TDataPtr;

        TNode* Parent;
        TPtr Left;
        TPtr Right;
        TDataPtr Data;
        TNode()
            : Parent(nullptr) {
        }
        TNode(TData* data)
            : Parent(nullptr)
            , Data(data) {
        }

        TNode* GetNextChild(TNode* child) {
            if (!child)
                return Left.Get();
            if (child == Left.Get())
                return Right.Get();
            return nullptr;
        }
        TNode* GetParent() {
            return Parent;
        }
    };

    typedef TNode TNode;
    typedef typename TNode::TPtr TNodePtr;

    template <class TVisitor>
    static void Traverse(TNode* root, TVisitor& visitor) {
        TNode* prev = nullptr;
        TNode* current = root;
        for (;;) {
            if (prev == current->GetParent() || !prev) { // come from parent
                if (!visitor.VisitBeforeChildren(current))
                    break;
                TNode* next = current->GetNextChild(nullptr);
                if (next) { // go to the first child
                    prev = current;
                    current = next;
                } else { // have no children, return to parent
                    if (!visitor.VisitAfterChildren(current))
                        break;
                    if (current == root)
                        break;
                    prev = current;
                    current = current->GetParent();
                }
            } else { // come form child
                TNode* next = current->GetNextChild(prev);
                if (next) { // go to next child
                    prev = current;
                    current = next;
                } else { // return to parent
                    if (!visitor.VisitAfterChildren(current))
                        break;
                    if (current == root)
                        break;
                    prev = current;
                    current = current->GetParent();
                }
            }
        }
    }

    static TNodePtr CloneR(TNode* node) {
        TNodePtr result = new TNode(node->Data->Clone().Get());
        if (node->Left.Get()) {
            result->Left = CloneR(node->Left.Get());
            result->Left->Parent = result.Get();
        }
        if (node->Right.Get()) {
            result->Right = CloneR(node->Right.Get());
            result->Right->Parent = result.Get();
        }
        return result;
    }

    static TNodePtr Clone(TNode* node) {
        return CloneR(node);
    }
};

class TAstData;

typedef TTree<TAstData> TAstTree;
typedef TIntrusivePtr<TAstData> TAstDataPtr;
typedef TAstTree::TNode TAstNode;
typedef TAstTree::TNodePtr TAstNodePtr;

class TAstData: public TSimpleRefCount<TAstData> {
public:
    enum Type {
#define X(A) A,
        WRE_AST_NODE_TYPES_LIST
#undef X
    };

private:
    Type Type_;

private:
    static const TString& TypeAsString(Type t) {
        static struct TMapType {
            THashMap<int, TString> Data;
            TMapType() {
#define X(T) Data[T] = #T;
                WRE_AST_NODE_TYPES_LIST
#undef X
            };
        } Map;
        return Map.Data[t];
    }

protected:
    TAstData(Type type)
        : Type_(type) {
    }

public:
    TSourcePosPtr SourcePos;

public:
    virtual TAstDataPtr Clone() = 0;
    virtual ~TAstData() {}
    Type GetType() const {
        return Type_;
    }
    const TString& TypeAsString() const {
        return TypeAsString(Type_);
    }

public:
    static Type GetType(const TAstNode* node) {
        return node->Data->GetType();
    }
};

#define WRE_STRUCT_AST_NODE(TYPE)                   \
    struct TAst##TYPE##_: public TAstData {         \
        TAst##TYPE##_():TAstData(TYPE) {}           \
    };                                              \
    struct TAst##TYPE: public TAst##TYPE##_ {       \
    virtual TAstDataPtr Clone() {                   \
        return new TAst##TYPE(*this);               \
    }

WRE_STRUCT_AST_NODE(Submatch)
    TTagId Id;
    TString Name;
    TAstSubmatch(const TString& name, TSourcePosPtr sourcePos)
        : Id(0)
        , Name(name)
    {
        SourcePos = sourcePos;
    }
    static TAstNodePtr Create(TAstNode* expr, const TString& name, TSourcePosPtr sourcePos) {
        TAstNodePtr result = new TAstNode();
        result->Data = new TAstSubmatch(name, sourcePos);
        result->Left = expr;
        result->Left->Parent = result.Get();
        return result;
    }
};

WRE_STRUCT_AST_NODE(Literal)
    TLiteral Lit;
    TAstLiteral(TLiteral literal)
        : Lit(literal) {
    }
    static TAstNodePtr Create(TLiteral literal) {
        TAstNodePtr result = new TAstNode();
        result->Data = new TAstLiteral(literal);
        return result;
    }
};

WRE_STRUCT_AST_NODE(Catenation)
    static TAstNodePtr Create(TAstNode* left, TAstNode* right) {
        TAstNodePtr result = new TAstNode();
        result->Data = new TAstCatenation();
        result->Left = left;
        result->Left->Parent = result.Get();
        result->Right = right;
        result->Right->Parent = result.Get();
        return result;
    }
};

WRE_STRUCT_AST_NODE(Iteration)
    int Min;
    int Max;
    bool Greedy;
    TAstIteration(size_t min, size_t max, bool greedy)
        : Min(min)
        , Max(max)
        , Greedy(greedy) {
    }
    static TAstNodePtr Create(TAstNode* atom, size_t min, size_t max, bool greedy) {
        TAstNodePtr result = new TAstNode();
        result->Data = new TAstIteration(min, max, greedy);
        result->Left = atom;
        result->Left->Parent = result.Get();
        return result;
    }
};

WRE_STRUCT_AST_NODE(Union)
    static TAstNodePtr Create(TAstNode* left, TAstNode* right) {
        TAstNodePtr result = new TAstNode();
        result->Data = new TAstUnion();
        result->Left = left;
        result->Left->Parent = result.Get();
        result->Right = right;
        result->Right->Parent = result.Get();
        return result;
    }
};

#define WRE_AST_DATA_TO_STRING(T)                                           \
    template <class TLiteralTable>                                      \
    inline TString ToStringAst##T(const TLiteralTable& lt, const TAst##T& d)

WRE_AST_DATA_TO_STRING(Submatch) {
    Y_UNUSED(lt);
    return  d.TypeAsString() + '[' + ::ToString(d.Id)  + (!d.Name ? TString() : "-" + d.Name) + ']';
}

WRE_AST_DATA_TO_STRING(Literal) {
    return d.TypeAsString() + '[' + ToString(lt, d.Lit) + ']';
}

WRE_AST_DATA_TO_STRING(Iteration) {
    Y_UNUSED(lt);
    return d.TypeAsString() + '{'
        + ::ToString(d.Min)
        + ','
        + ::ToString(d.Max)
        + (d.Greedy ? "" : ",NG")
        + '}';
}

WRE_AST_DATA_TO_STRING(Catenation) {
    Y_UNUSED(lt);
    return d.TypeAsString();
}

WRE_AST_DATA_TO_STRING(Union) {
    Y_UNUSED(lt);
    return d.TypeAsString();
}

template <class TLiteralTable>
inline TString ToString(const TLiteralTable& lt, const TAstData& d) {
    switch (d.GetType()) {
#define X(T)                                    \
        case TAstData::T:                       \
            return ToStringAst##T(lt, static_cast<const TAst##T&>(d));
WRE_AST_NODE_TYPES_LIST
#undef X
    }
    return TString();
}

inline void PrintIndent(IOutputStream& out, size_t level) {
    for (size_t i = 0; i < level; ++i) { out << "  "; }
}

template <class TLiteralTable>
inline void PrintNode(IOutputStream& out, const TLiteralTable& lt, const TAstNode* node, size_t level) {
    PrintIndent(out, level);
    out << ToString(lt, *node->Data) << Endl;
    if (node->Left.Get())
        PrintNode(out, lt, node->Left.Get(), level + 1);
    if (node->Right.Get())
        PrintNode(out, lt, node->Right.Get(), level + 1);
}

template <class TLiteralTable>
inline void Print(IOutputStream& out, const TLiteralTable& lt, const NRemorph::TAstNode& node) {
    PrintNode(out, lt, &node, 0);
}

} // NRemorph
