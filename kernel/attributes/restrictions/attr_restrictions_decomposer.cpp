#include "attr_restrictions_decomposer.h"

#include <kernel/qtree/richrequest/richnode.h>

#include <ysite/yandex/common/prepattr.h>

namespace NAttributes {

namespace {

inline bool IsAttributeNode(const TRichNodePtr& richNode) {
    return IsAttribute(*richNode) && richNode->Op() != oZone;
}

TSingleAttrRestriction_ECompareOper CmpOperToProtobufEnum(TCompareOper oper) {
    switch (oper) {
        case cmpLT:
            return TSingleAttrRestriction::cmpLT;
        case cmpLE:
            return TSingleAttrRestriction::cmpLE;
        case cmpEQ:
            return TSingleAttrRestriction::cmpEQ;
        case cmpGE:
            return TSingleAttrRestriction::cmpGE;
        case cmpGT:
            return TSingleAttrRestriction::cmpGT;
        default:
            Y_ASSERT(false);
    }
    return TSingleAttrRestriction::cmpEQ;
}

bool DecomposeAttribute(const TRichNodePtr& richNode, bool useUTF8Attrs, TSingleAttrRestriction* result) {
    const TString attr = PrepareAttrName(richNode->GetTextName());
    TString preparedValue;
    if (!PrepareAttrValue(richNode->GetText(), useUTF8Attrs, preparedValue)) {
        return false;
    }

    if (preparedValue.empty()) {
        return false;
    }

    TString secondValue;
    // segment attributes have the second value in GetAttrValueHi()
    if (!richNode->GetAttrValueHi().empty() && !PrepareAttrValue(richNode->GetAttrValueHi(), useUTF8Attrs, secondValue)) {
        return false;
    }
    result->SetCmpOper(CmpOperToProtobufEnum(richNode->OpInfo.CmpOper));
    if (!secondValue.empty()) {
        SetAttrKey(*result->MutableLeft(), attr, "=", preparedValue);
        SetAttrKey(*result->MutableRight(), attr, "=", secondValue);
    } else switch (result->GetCmpOper()) {
        case TSingleAttrRestriction::cmpEQ:
            SetAttrKey(*result->MutableLeft(), attr, "=", preparedValue);
            break;
        case TSingleAttrRestriction::cmpLE:
        case TSingleAttrRestriction::cmpLT:
            SetAttrKey(*result->MutableLeft(), attr, "=", "");
            SetAttrKey(*result->MutableRight(), attr, "=", preparedValue);
            break;
        case TSingleAttrRestriction::cmpGE:
        case TSingleAttrRestriction::cmpGT:
            SetAttrKey(*result->MutableLeft(), attr, "=", preparedValue);
            SetAttrKey(*result->MutableRight(), attr, "=", "\xff");
            break;
        default:
            Y_ASSERT(false);
    }
    return true;
}


bool DecomposeNode(const TRichNodePtr& richNode, bool useUTF8Attrs, TAttrRestrictions<>* attrRoot) {
    TSingleAttrRestriction current;

    // promise of being added
    attrRoot->AddNode(current);
    size_t curIndex = attrRoot->Size() - 1;

    bool result = false;

    if (IsAttributeNode(richNode)) {
        // leaf
        // it's a pity but attributes can have children
        result = DecomposeAttribute(richNode, useUTF8Attrs, &current);
    }

    if (IsAttributeNode(richNode) || IsOr(*richNode) || IsAndOp(*richNode) || richNode->IsAndNotOp()) {
        // internal node
        if (IsOr(*richNode)) {
            current.SetTreeOper(TSingleAttrRestriction::Or);
        } else if (richNode->IsAndNotOp()) {
            current.SetTreeOper(TSingleAttrRestriction::AndNot);
        } else if (IsAttributeNode(richNode)) {
            current.SetTreeOper(TSingleAttrRestriction::Leaf);
        } else {
            current.SetTreeOper(TSingleAttrRestriction::And);
        }

        for (const TRichNodePtr& child : richNode->Children) {
            if (child) {
                // index is the child itself, it will have index of current Size()
                size_t index = attrRoot->Size();
                if (DecomposeNode(child, useUTF8Attrs, attrRoot)) {
                    current.AddChildren(index);
                }
            }
        }
    }

    for (const TRichNodePtr& miscOp : richNode->MiscOps) {
        if (miscOp && (miscOp->Op() == oRestrDoc || miscOp->Op() == oAndNot) && miscOp->Children.size() == 1) {
            if (miscOp->Op() == oAndNot) {
                // index is the child itself, it will have index of current Size()
                size_t index = attrRoot->Size();
                if (DecomposeNode(miscOp, useUTF8Attrs, attrRoot)) {
                    current.AddChildren(index);
                }
            } else {
                TRichNodePtr restriction = miscOp->Children[0];
                // index is the child itself, it will have index of current Size()
                size_t index = attrRoot->Size();
                if (DecomposeNode(restriction, useUTF8Attrs, attrRoot)) {
                    current.AddChildren(index);
                    // miscOps are the requirenments in spite of operations
                    (*attrRoot)[index].SetRequired(true);
                }

            }
        }
    }
    result |= !current.GetChildren().empty();
    if (!result) {
        // break promise if nothing added
        attrRoot->PopBack();
    } else {
        (*attrRoot)[curIndex] = std::move(current);
    }
    return result;
}


} // namespace


TAttrRestrictions<> DecomposeAttrRestrictions(const TRichTreeConstPtr& richTree, bool useUTF8Attrs) {
    TAttrRestrictions<> attrRoot;
    if (richTree && richTree->Root) {
        DecomposeNode(richTree->Root, useUTF8Attrs, &attrRoot);
    }
    Y_ASSERT(IsValidAttrRestrictions(attrRoot));
    return attrRoot;
}

bool IsValidAttrRestrictions(const TAttrRestrictions<>& attrRestrictions) {
    size_t size = attrRestrictions.Size();
    for (size_t i = 0; i < size; ++i) {
        for (ui32 child : attrRestrictions[i].GetChildren()) {
            // out of range or acyclic graph
            if (child >= size || child <= i) {
                return false;
            }
        }
    }
    return true;
}


} // namespace NAttributes
