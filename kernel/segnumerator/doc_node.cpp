#include "doc_node.h"

namespace NSegm {
namespace NPrivate {


TDocNode::TDocNode(EDocNodeType type, TDocNode* parent, TAlignedPosting nodeStart, TAlignedPosting nodeEnd)
    : Parent(parent)
    , NodeStart(nodeStart)
    , NodeEnd(nodeEnd)
    , Type(type)
{
    Zero(Props);
    Y_VERIFY(Type > DNT_NONE && Type < DNT_COUNT, " ");
}

bool TDocNode::NodeEmpty() const {
    switch (Type) {
    default:
        return true;
    case DNT_LINK:
        return false;
    case DNT_TEXT:
        return !Props.NWords;
    case DNT_BLOCK:
        for (const_iterator it = Begin(); it != End(); ++it)
            if (!it->NodeEmpty())
                return false;
        return true;
    }
}

void TDocNode::MergeTextMarkers(TTextMarkers& m) const {
    if (DNT_TEXT == Type)
        m |= Props.TextMarkers;

    if (DNT_LINK != Type && DNT_BLOCK != Type)
        return;

    for (const_iterator it = Begin(); it != End(); ++it)
        it->MergeTextMarkers(m);
}

void TDocNode::GenerateSignature() {
    if (!Parent)
        return;
    ui32 sign[4] = { Parent->Props.Signature, Props.Class, Props.Width, static_cast<ui32>(Props.Tag) };
    Props.Signature = MurmurHash<ui32> (sign, sizeof(sign));
}

void TDocNode::Collapse() {
    Y_VERIFY(IsTelescopic(), " ");
    TDocNode* node = Front();

    Props.Tag = IsHxTag(Props.Tag) && HT_TABLE != node->Props.Tag && !IsListRootTag(
            node->Props.Tag) ? Props.Tag : node->Props.Tag;
    Props.BlockMarkers |= node->Props.BlockMarkers;

    //tell node's children who's the daddy
    for (TDocNode::iterator it = node->Begin(); it != node->End(); ++it)
        it->Parent = this;

    Clear();//clear the list
    Adopt(*node);//set the list to adopt node's children
}

void TDocNode::Replace() {
    Y_VERIFY(IsReplaceable(), " ");

    if (HT_TABLE == Props.Tag)
        Front()->Collapse();

    Collapse();
    Props.Tag = HT_DIV;
}

void TDocNode::ResetReadability() {
    Y_VERIFY(DNT_BLOCK == Type, " ");

    Props.CleanedOut = 0;
    Props.ContentScore = 0;
    Props.IsACandidate = 0;
    Props.AddedToContent = 0;
}

TDocNode* MakeNode(TDocNode* node, TDocNode* parent) {
    if (parent)
        parent->PushBack(node);
    return node;
}

TDocNode* MakeText(TMemoryPool& pool, TAlignedPosting begin, TAlignedPosting end, TDocNode* parent, ui32 nWords,
                   TBoldDistance dist) {
    TDocNode* node = new (pool) TDocNode(DNT_TEXT, parent, begin, end);

    node->Props.NWords = nWords;
    node->Props.BoldDistance = dist;

    return MakeNode(node, parent);
}

TDocNode* MakeBreak(TMemoryPool& pool, TAlignedPosting pos, TDocNode* parent, ETagBreakLevel lev) {
    TDocNode* node = new (pool) TDocNode(DNT_BREAK, parent, pos, pos);
    node->Props.Level = lev;

    return MakeNode(node, parent);
}

TDocNode* MakeBreak(TMemoryPool& pool, TAlignedPosting pos, TDocNode* parent, HT_TAG tag) {
    return MakeBreak(pool, pos, parent, GetBreakLevel(tag));
}

TDocNode* MakeInput(TMemoryPool& pool, TAlignedPosting pos, TDocNode* parent, ui32 nInputs) {
    TDocNode* node = new (pool) TDocNode(DNT_INPUT, parent, pos, pos);
    node->Props.NInputs = nInputs;
    return MakeNode(node, parent);
}

TDocNode* MakeBlock(TMemoryPool& pool, TAlignedPosting begin, TDocNode* parent, HT_TAG tag) {
    Y_VERIFY(!parent || IsA<DNT_BLOCK> (parent), " ");
    TDocNode* node = new (pool) TDocNode(DNT_BLOCK, parent, begin, begin);
    node->Props.Tag = tag;
    node->GenerateSignature();
    node->Props.NodeLevel = parent ? parent->Props.NodeLevel + 1 : 0;
    return MakeNode(node, parent);
}

TDocNode* MakeLink(TMemoryPool& pool, TAlignedPosting begin, TDocNode* parent, ELinkType linktype, ui32 domain) {
    Y_VERIFY(IsA<DNT_BLOCK> (parent), " ");
    TDocNode* node = new (pool) TDocNode(DNT_LINK, parent, begin, begin);
    node->Props.LinkType = linktype;
    node->Props.Domain = domain;
    return MakeNode(node, parent);
}

bool IsEmpty(TDocNode* node) {
    for (TDocNode::iterator it = node->Begin(); it != node->End(); ++it) {
        TDocNode* child = &*it;

        if (IsA<DNT_BLOCK> (child) && !IsEmpty(child))
            return false;

        if (IsA<DNT_INPUT> (child) || IsA<DNT_LINK> (child))
            return false;

        if (IsA<DNT_TEXT> (child)) {
            Y_VERIFY(child->Props.NWords, " ");
            return false;
        }
    }

    return true;
}

}
}
