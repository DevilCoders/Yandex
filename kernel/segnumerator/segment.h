#pragma once

#include "doc_node.h"

#include <kernel/segmentator/structs/structs.h>

#include <functional>
#include <algorithm>
#include <bitset>

namespace NSegm {
namespace NPrivate {

struct TSegmentStatistics {
private:
    typedef std::bitset<256> THashedHash;

    enum {
        DomainHashMask = 0xff,
        MaxDomains = 128
    };

    THashedHash DomainSet = 0;

public:
    ui32 Words = 0;
    ui32 LinkWords = 0;
    ui32 Inputs = 0;
    ui32 Breaks = 0;
    ui32 SelfLinks = 0;
    ui32 FragmentLinks = 0;
    ui32 LocalLinks = 0;
    ui32 Links = 0;
    TTextMarkers TextMarkers = TTextMarkers::New();
    TLinkMarkers LinkMarkers = TLinkMarkers::New();

public:
    ui32 Domains() const {
        return DomainSet.count();
    }

    void Add(ETagBreakLevel tbl) {
        Breaks += tbl > TBL_BR;
    }

    void Add(TDocNode& src, bool inLink = false);

    bool IsEmpty() const {
        Y_VERIFY(Words >= LinkWords && Links >= LocalLinks, " ");
        return !Words && !Links && !Inputs;
    }
};

class TSegmentNode: public TIntrusiveListItem<TSegmentNode>, public TPoolable {
    typedef TIntrusiveListItem<TSegmentNode> TItemBase;

public:
    TSegmentStatistics Stats;

    TDocNode * FrontBlock = nullptr;
    TDocNode * BackBlock = nullptr;
    bool IncludesFrontBlock = 0;
    bool IncludesBackBlock = 0;
    bool HasHeader = 0;

public:
    friend bool operator==(const TSegmentNode& a, const TSegmentNode& b) {
        return &a == &b || (a.IncludesFrontBlock == b.IncludesFrontBlock
                && a.IncludesBackBlock == b.IncludesBackBlock && *a.FrontBlock == *b.FrontBlock
                && *a.BackBlock == *b.BackBlock);
    }

    friend bool operator!=(const TSegmentNode& a, const TSegmentNode& b) {
        return !(a == b);
    }

public:
    void Add(TDocNode * node) {
        Y_VERIFY(!IsA<DNT_BLOCK> (node), " ");
        Stats.Add(*node);
    }

    void SetFrontBlock(TDocNode * node, bool include = true) {
        FrontBlock = node;
        IncludesFrontBlock = include;
    }

    void SetBackBlock(TDocNode * node, bool include = true) {
        BackBlock = node;
        IncludesBackBlock = include;
    }

    TAlignedPosting GetFrontOffset() const {
        Y_VERIFY(FrontBlock, " ");
        return IncludesFrontBlock ? FrontBlock->NodeStart.SentAligned() : FrontBlock->NodeEnd.SentAligned();
    }

    TAlignedPosting GetBackOffset() const {
        Y_VERIFY(BackBlock, " ");
        return IncludesBackBlock ? BackBlock->NodeEnd.SentAligned() : BackBlock->NodeStart.SentAligned();
    }

    bool IsEmpty() const {
        return Stats.IsEmpty();
    }

    TDocNode* GetRealFrontBlock() const;
    TDocNode* GetRealBackBlock() const;
    TBlockMarkers FrontBlockMarkers() const;

    TSegmentSpan GetZone() const;
};

EBlockStepType GetStep(HT_TAG t);

void IncrementStep(TBlockInfo& d, TDocNode *& n);
void AddStep(TBlockInfo& d1, TBlockInfo& d2, TDocNode*n1, TDocNode*n2);

TBlockDist SumDist(TBlockInfo& d1, TBlockInfo& d2, TDocNode* n1, TDocNode* n2);
TBlockDist SumDist(TBlockInfo& d1, TBlockInfo& d2, TSegmentNode& prev, TSegmentNode& next);
TBlockDist SumDist(TSegmentNode&s1, TSegmentNode&s2);

typedef TIntrusiveList<TSegmentNode> TSegmentList;

struct TSegmentAccessor : TListAccessor<TSegmentNode, TSegmentList> {
    static TBlockDist GetBlockDist(TSegmentNode * first, TSegmentNode * second) {
        return first && second ? SumDist(*first, *second) : TBlockDist::Max();
    }
};

template <typename tlist>
void MakeSegmentList(tlist& lst, TSegmentList& segments) {
    for (TSegmentList::iterator it = segments.Begin(); it != segments.End(); ++it) {
        TSegmentNode* prev = TSegmentAccessor::GetPrev(it, segments.Begin());
        TSegmentSpan z = it->GetZone();

        if (z.NoSents())
            continue;

        if (!lst.empty())
            SumDist(lst.back().LastBlock, z.FirstBlock, *prev, *it);

        lst.push_back(z);
    }
}

}
}
