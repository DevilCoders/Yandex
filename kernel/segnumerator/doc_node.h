#pragma once

#include "html_functions.h"
#include "html_markers.h"
#include "list_utils.h"

#include <kernel/segmentator/structs/spans.h>

#include <util/generic/vector.h>
#include <util/generic/yexception.h>

namespace NSegm {
namespace NPrivate {

enum EDocNodeType {
    DNT_NONE = 0, DNT_BREAK, DNT_INPUT, DNT_TEXT, DNT_LINK, DNT_BLOCK, DNT_COUNT
};

class TDocNode: public  TIntrusiveListItem<TDocNode>
              , private TIntrusiveList<TDocNode>
              , public  TPoolable {

    typedef TIntrusiveListItem<TDocNode> TItem;
    typedef TIntrusiveList<TDocNode> TListType;

public:
    typedef TListType::TIterator iterator;
    typedef TListType::TConstIterator const_iterator;
    typedef TReverseListIterator<iterator> reverse_iterator;
    typedef TReverseListIterator<const_iterator> const_reverse_iterator;

    TDocNode* Parent;
    TAlignedPosting NodeStart;
    TAlignedPosting NodeEnd;

    union TProps {
        /// Input
        ui32 NInputs;

        /// Break
        ETagBreakLevel Level;

        /// Text
        struct {
            ui32 NWords;

            TBoldDistance BoldDistance;
            TTextMarkers TextMarkers;
        };

        /// Block
        struct {
            ui32 Class;
            ui32 Width;
            ui32 Signature;

            // readability field
            float ContentScore;

            TBlockMarkers BlockMarkers;

            ui16 NodeLevel;

            // readability fields
            struct {
                ui16 CleanedOut :1;
                ui16 IsACandidate :1;
                ui16 AddedToContent :1;
            };

            HT_TAG Tag;
        };

        /// Link
        struct {
            ui32 Domain;
            ELinkType LinkType;

            TLinkMarkers LinkMarkers;
        };
    };

    TProps Props;

    const EDocNodeType Type;

public:
    TDocNode(EDocNodeType type, TDocNode* parent, TAlignedPosting nodeStart, TAlignedPosting nodeEnd);

    friend bool operator==(const TDocNode& a, const TDocNode& b) {
        return &a == &b || (DNT_BLOCK == a.Type && DNT_BLOCK == b.Type && a.Props.Signature == b.Props.Signature);
    }

    friend bool operator!=(const TDocNode& a, const TDocNode& b) {
        return !(a == b);
    }

    bool NodeEmpty() const;

    TDocNode* GetBlock() {
        if (DNT_BLOCK == Type)
            return this;

        return Parent ? Parent->GetBlock() : nullptr;
    }

public:
    //list methods
    TDocNode* CloseList(TAlignedPosting end) {
        NodeEnd = end;
        return Parent;
    }

    ui16 SentBegin() const {
        return GetBreak(NodeStart) + (GetWord(NodeStart) > 1);
    }

    ui16 SentEnd() const {
        return GetBreak(NodeEnd) + (GetWord(NodeEnd) > 1);
    }

    bool ContainsSent(ui16 sent) const {
        return SentBegin() <= sent && SentEnd() > sent;
    }

    ui16 NSents() const {
        return SentEnd() - SentBegin();
    }

    void PushBack(TDocNode* item) {
        TListType::PushBack(CheckChild(item));
    }

    void PushFront(TDocNode* item) {
        TListType::PushFront(CheckChild(item));
    }

    void Adopt(TDocNode& node) {
        Append(node);
    }

    void Clear() {
        TListType::Clear();
    }

    bool ListEmpty() const {
        return TListType::Empty();
    }

    size_t Size() const {
        return TListType::Size();
    }

    TDocNode* Front() {
        return &*Begin();
    }

    TDocNode* Back() {
        return &*(--End());
    }

    iterator Begin() {
        return TListType::Begin();
    }

    const_iterator Begin() const {
        return TListType::Begin();
    }

    reverse_iterator RBegin() {
        return reverse_iterator(--End());
    }

    iterator End() {
        return TListType::End();
    }

    const_iterator End() const {
        return TListType::End();
    }

    reverse_iterator REnd() {
        return reverse_iterator(End());
    }

    bool HasSingleChild() {
        return (DNT_BLOCK == Type || DNT_LINK == Type) && !ListEmpty() && Front() == Back();
    }

    template<EDocNodeType AType>
    TDocNode* GetFirstChildOfType(bool nonempty) {
        if ((AType == Type && !nonempty) || !NodeEmpty())
            return this;

        if (DNT_LINK != Type && DNT_BLOCK != Type)
            return nullptr;

        for (iterator it = Begin(); it != End(); ++it)
            if (TDocNode* n = it->GetFirstChildOfType<AType>(nonempty))
                return n;

        return nullptr;
    }

    template<EDocNodeType AType>
    TDocNode* GetLastChildOfType(bool nonempty) {
        if ((AType == Type && !nonempty) || !NodeEmpty())
            return this;

        if (DNT_LINK != Type && DNT_BLOCK != Type)
            return nullptr;

        for (reverse_iterator it = RBegin(); it != REnd(); ++it)
            if (TDocNode* n = it->GetLastChildOfType<AType>(nonempty))
                return n;

        return nullptr;
    }

    void MergeTextMarkers(TTextMarkers& m) const;

public:
    //break methods
    void MergeBreak(HT_TAG tag) {
        MergeBreak(GetBreakLevel(tag));
    }

    void MergeBreak(ETagBreakLevel lev) {
        Y_VERIFY(DNT_BREAK == Type, " ");
        Props.Level = MergeBreaks(lev, Props.Level);
    }

    void MergeBreak(TDocNode* node) {
        Y_VERIFY(node && DNT_BREAK == node->Type, " ");
        MergeBreak(node->Props.Level);
    }

public:
    //block methods
    bool IsTelescopic() {
        /* parent <- me <- block <-... => parent <- me <-... */
        return DNT_BLOCK == Type && Parent && HasSingleChild() && DNT_BLOCK == Front()->Type;
    }

    bool IsCollapseable() {
        return IsTelescopic() && !IsTableTag(Props.Tag) && !IsListTag(Props.Tag)
                        && !IsTableTag(Front()->Props.Tag) && !IsListTag(Front()->Props.Tag);
    }

    bool IsReplaceable() {
        return IsTelescopic() && ((HT_TABLE == Props.Tag && HT_TR == Front()->Props.Tag && Front()->IsTelescopic())
                        || IsListRootTag(Props.Tag));
    }

    void GenerateSignature();
    void Collapse();
    void Replace();
    void ResetReadability();

private:
    TDocNode* CheckChild(TDocNode* item) {
        Y_VERIFY(item && (DNT_BLOCK == Type || (DNT_LINK == Type && DNT_BLOCK != item->Type)), " ");
        return item;
    }
};

typedef TListAccessor<TDocNode> TNodeAccessor;

template<EDocNodeType Type>
inline bool IsA(const TDocNode* node) {
    return node && Type == node->Type;
}

inline bool Iterateable(const TDocNode* node) {
    return IsA<DNT_BLOCK>(node) || IsA<DNT_LINK>(node);
}

bool IsEmpty(TDocNode* node);

template<EDocNodeType Type>
inline TDocNode* GetLastNode(TDocNode* currentNode) {
    return Iterateable(currentNode) && !currentNode->ListEmpty() && IsA<Type> (currentNode->Back())
                    ? currentNode->Back() : nullptr;
}

TDocNode* MakeNode (TDocNode* node, TDocNode* parent);
TDocNode* MakeText (TMemoryPool& pool, TAlignedPosting beg, TAlignedPosting end, TDocNode* parent, ui32 nWords,
                        TBoldDistance dist = TBoldDistance());
TDocNode* MakeBreak(TMemoryPool& pool, TAlignedPosting pos, TDocNode* parent, ETagBreakLevel lev);
TDocNode* MakeBreak(TMemoryPool& pool, TAlignedPosting pos, TDocNode* parent, HT_TAG tag);
TDocNode* MakeInput(TMemoryPool& pool, TAlignedPosting pos, TDocNode* parent, ui32 nInputs);
TDocNode* MakeBlock(TMemoryPool& pool, TAlignedPosting beg, TDocNode* parent, HT_TAG tag);
TDocNode* MakeLink (TMemoryPool& pool, TAlignedPosting beg, TDocNode* parent, ELinkType type, ui32 domain);


struct TTagItem {
    HT_TAG Tag;
    TDocNode* Block;

    explicit TTagItem(HT_TAG tag, TDocNode* block = nullptr)
        : Tag(tag)
        , Block(block)
    {
    }
};

}
}
