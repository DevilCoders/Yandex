#pragma once

// Helper classes for forums tracker:
// recognize chains of nested tags following a given pattern
#include <library/cpp/html/face/event.h>
#include <util/generic/vector.h>
#include <util/system/yassert.h>

struct TTagDescriptor
{
    HT_TAG Tag;
    unsigned Flags; // bitfield using members of the next enum
    enum {
        WaitForParentClose = 1, // <parent>...<tag>area starts here</tag>and continues here</parent>
        WaitForPrevClose = 2, // valid only for non-first tags in TTagChainDescriptor
        NoDescent = 4, // <parent>...<tag>no area here</tag>may be here</parent>
    };
    const char* AttrName;
    size_t AttrNameLen;
    const char* AttrValue;
    size_t AttrValueLen;
    enum { ExactMatch, PrefixWithoutDigits, Prefix, WordPrefix, Substring, NotPrefix } AttrValueMatch;
    bool Match(const THtmlChunk* chunk) const;
};

struct TTagChainDescriptor
{
    unsigned NumTags;
    const TTagDescriptor* Chain;
};

class TTagChainTracker
{
    struct TLocation {
        int Depth;
        const TTagDescriptor* Tag;
        TLocation() {}
        TLocation(int depth, const TTagDescriptor* tag)
            : Depth(depth)
            , Tag(tag)
        {}
    };
    TVector<TLocation> Found;
    const TTagDescriptor* NextTag;
    const TTagChainDescriptor* Associated;
    bool Full;
    bool WasFull;
public:
    enum EStateChange {
        NotChanged,
        AreaEntered,
        AreaLeft,
    };
    void Clear()
    {
        Found.clear();
        NextTag = nullptr;
        Associated = nullptr;
        Full = false;
        WasFull = false;
    }
    TTagChainTracker()
    {
        Clear();
    }
    operator bool() const
    {
        return Full;
    }
    EStateChange OnOpenTag(const THtmlChunk* chunk, int depth, HT_TAG& watchOpenTag, HT_TAG& watchCloseTag);
    EStateChange OnCloseTag(const THtmlChunk* chunk, int depth, HT_TAG& watchOpenTag, HT_TAG& watchCloseTag);
    void SetAssociatedChain(const TTagChainDescriptor* associated)
    {
        Y_ASSERT(!Associated && !NextTag);
        Associated = associated;
        NextTag = associated->Chain;
    }
    HT_TAG GetFirstTag() const
    {
        Y_ASSERT(Associated);
        if (!Associated->NumTags)
            return HT_TagCount;
        Y_ASSERT(!(Associated->Chain[0].Flags & TTagDescriptor::NoDescent));
        return Associated->Chain[0].Tag;
    }
    void Restart()
    {
        Found.clear();
        NextTag = Associated->Chain;
        Full = false;
        WasFull = false;
    }
};

