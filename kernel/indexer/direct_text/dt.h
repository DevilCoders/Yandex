#pragma once

#include "fl.h"
//#include <kernel/indexer/face/inserter.h>

#include <library/cpp/token/nlptypes.h>

#include <util/generic/string.h>
#include <util/generic/bitmap.h>
#include <util/system/defaults.h>

struct TDocInfoEx;
struct THtmlChunk;
class TFullDocAttrs;
class IDocumentDataInserter;

namespace NIndexerCore {

struct TDisambMask {
    typedef TBitMap<64, ui64> TMask; //double declaration (second one in zel_disamb_model)
    TMask Mask;
    int BestLemma;
    float Weight;
    TDisambMask()
        : BestLemma(-1)
        , Weight(0)
    {}
};


struct TDirectTextSpace {
    const wchar16* Space;
    ui32 Length;
    TBreakType Type;

    TDirectTextSpace()
        : Space(nullptr)
        , Length(0)
        , Type(0)
    {
    }

    TDirectTextSpace(const wchar16* space, ui32 length, TBreakType type = 0)
        : Space(space)
        , Length(length)
        , Type(type)
    {
    }
};

struct TDirectTextEntry2 {
    TPosting          Posting;    // position of the first form in multitoken
    ui32              OrigOffset; // оффсет в байтах от начала исходного документа
    TWtringBuf        Token;      // original token as it appears in document
    const TLemmatizedToken* LemmatizedToken; // it can be NULL if lemmer returned invalid lemma, points to cache so can't be modified
    TDirectTextSpace* Spaces;
    ui32              LemmatizedTokenCount;
    ui32              SpaceCount;

    TDirectTextEntry2()
        : Posting(0)
        , OrigOffset(0)
        , Token()
        , LemmatizedToken(nullptr)
        , Spaces(nullptr)
        , LemmatizedTokenCount(0)
        , SpaceCount(0)
    {
    }
};

inline TPosting GetPosting(const TDirectTextEntry2& e) {
    return e.Posting;
}

struct TZoneSpan {
    ui16 SentBeg;
    ui16 WordBeg;
    ui16 SentEnd;
    ui16 WordEnd;

    TZoneSpan()
        : SentBeg(0)
        , WordBeg(0)
        , SentEnd(0)
        , WordEnd(0)
    {
    }
    TZoneSpan(TPosting beg, TPosting end)
        : SentBeg(GetBreak(beg))
        , WordBeg(GetWord(beg))
        , SentEnd(GetBreak(end))
        , WordEnd(GetWord(end))
    {
    }
    bool Contains(ui16 sent, ui16 word) const {
        if (sent < SentBeg || sent == SentBeg && word < WordBeg)
            return false;
        if (sent > SentEnd || sent == SentEnd && word >= WordEnd)
            return false;
        return true;
    }
    bool Contains(TPosting pos) const {
        return Contains(GetBreak(pos), GetWord(pos));
    }
    bool operator==(TPosting pos) const {
        return Contains(pos);
    }
    bool operator<(TPosting pos) const {
        const ui16 wd = GetWord(pos);
        const ui16 br = GetBreak(pos);
        return (br > SentEnd || br == SentEnd && wd >= WordEnd);
    }
    bool operator>(TPosting pos) const {
        const ui16 wd = GetWord(pos);
        const ui16 br = GetBreak(pos);
        return (br < SentBeg || br == SentBeg && wd < WordBeg);
    }
    bool operator<(const TZoneSpan& s2) const {
        return
            SentBeg < s2.SentBeg
         || SentBeg == s2.SentBeg && WordBeg < s2.WordBeg
         || SentBeg == s2.SentBeg && WordBeg == s2.WordBeg && SentEnd < s2.SentEnd
         || SentBeg == s2.SentBeg && WordBeg == s2.WordBeg && SentEnd == s2.SentEnd && WordEnd < s2.WordEnd;
    }
    bool operator==(const TZoneSpan& s2) const {
        return SentBeg == s2.SentBeg && WordBeg == s2.WordBeg && SentEnd == s2.SentEnd && WordEnd == s2.WordEnd;
    }
    bool IsEmpty() const {
        return SentBeg == SentEnd && WordBeg == WordEnd;
    }
};

enum EDTZoneType {
    DTZoneSearch = 1, // should be inserted into key/inv
    DTZoneText = 2,   // should be inserted into text archive
};

struct TDirectTextZone {
    TStringBuf Zone;
    const TZoneSpan* Spans;
    ui32 SpanCount;
    ui8 ZoneType;

    TDirectTextZone()
        : Zone()
        , Spans(nullptr)
        , SpanCount(0)
        , ZoneType(0)
    {
    }
};

struct TDirectAttrEntry {
    ui16 Sent;
    ui16 Word;
    bool NoFollow;
    bool Sponsored;
    bool Ugc;
    TWtringBuf AttrValue;

    TDirectAttrEntry()
        : Sent(0)
        , Word(0)
        , NoFollow(false)
        , Sponsored(false)
        , Ugc(false)
        , AttrValue()
    {
    }
};

enum EDTAttrType {
    DTAttrSearchLiteral = 1,
    DTAttrSearchUrl     = 2,
    DTAttrSearchDate    = 3,
    DTAttrSearchInteger = 4,
    DTAttrSearchMask    = 7,
    DTAttrText          = 8,
};

struct TDirectTextZoneAttr {
    TStringBuf AttrName;
    const TDirectAttrEntry* Entries;
    ui32 EntryCount;
    ui8 AttrType;

    TDirectTextZoneAttr()
        : AttrName()
        , Entries(nullptr)
        , EntryCount(0)
        , AttrType(0)
    {
    }
};

struct TDirectTextSentAttr {
    TString Attr;
    TString Value;
    ui16 Sent;

    TDirectTextSentAttr()
        : Sent(0)
    {
    }

    bool operator <(const TDirectTextSentAttr& attr) const {
        return Sent < attr.Sent;
    }
};

struct TDirectTextData2 {
    const TDirectTextEntry2* Entries = nullptr;
    size_t EntryCount = 0;
    const TDirectTextZone* Zones = nullptr;
    size_t ZoneCount = 0;
    const TDirectTextZoneAttr* ZoneAttrs = nullptr;
    size_t ZoneAttrCount = 0;
    const TDirectTextSentAttr* SentAttrs = nullptr;
    size_t SentAttrCount = 0;
};

//! this class is returned as non-const accessor to the direct text entries of TDirectText2.
//! TDirectText2 can be used because it has a bunch ot non-const methods that intended for
//! insertion of forms, zones and attributes.
class TDirectTextEntries {
    TDirectTextEntry2* Entries;
    size_t EntryCount;
public:
    explicit TDirectTextEntries(TDirectTextEntry2* entries = nullptr, size_t entryCount = 0)
        : Entries(entries)
        , EntryCount(entryCount)
    {
    }
    const TDirectTextEntry2* GetEntries() const {
        return Entries;
    }
    size_t GetEntryCount() const {
        return EntryCount;
    }
    void SetRelevLevel(size_t index, RelevLevel relevLevel) {
        Y_ASSERT(index < EntryCount);
        SetPostingRelevLevel(Entries[index].Posting, relevLevel);
    }
};

class IDirectTextCallback2 {
public:
    virtual ~IDirectTextCallback2() {}
    virtual void SetCurrentDoc(const TDocInfoEx& /*docInfo*/) {
    }
    virtual void ProcessDirectText2(IDocumentDataInserter* inserter, const TDirectTextData2& directText, ui32 docId) = 0;
    virtual void MakePortion() {
    }
    virtual void Finish() {
    }
};

/**
 * Unlike `IDirectTextCallback2`, which processes document's direct text data in
 * one pass, this callback processes it chunk by chunk. Users can also supply
 * additional data with each chunk (e.g. stream type).
 */
class IDirectTextCallback5 {
public:
    virtual ~IDirectTextCallback5() {}

    /**
     * Starts processing of next document.
     *
     * @param docId                     Id of the next document.
     */
    virtual void AddDoc(ui32 docId) = 0;

    /**
     * Processes part of text.
     *
     * @param entries                   Pointer to an array of new direct text entries.
     * @param entryCount                Size of the aforementioned array.
     * @param callbackV5Data            Pointer to callback-specific data. It's up to
     *                                  implementation how to interpret it.
     */
    virtual void ProcessDirectText(const TDirectTextEntry2* entries, size_t entryCount, const void* callbackV5Data) = 0;
    virtual void MakePortion() {
    }
    virtual void Finish() {
    }
};

class IDisambDirectText {
public:
    virtual ~IDisambDirectText() {}
    virtual void ProcessText(const TDirectTextEntry2* entries, size_t entCount, TVector<TDisambMask>* masks) = 0;
};

class IModifyDirectText {
public:
    virtual ~IModifyDirectText() {}
    virtual void SetCurrentDoc(const TDocInfoEx& /*docInfo*/) {
    }
    virtual void ProcessDirectText2(IDocumentDataInserter* inserter, TDirectTextEntries& entries, const TDisambMask* masks, ui32 docId) = 0;
    virtual void Finish() {
    }
};

class IDirectTextCallback3 {
public:
    virtual ~IDirectTextCallback3() {}
    virtual bool ProcessDirectText(const TDirectTextData2& directText, const TDocInfoEx* docInfo, const TFullDocAttrs* docAttrs, ui32 docId) = 0;
    virtual void MakePortion() = 0;
};

class IDirectTextCallback4 : public IDirectTextCallback3 {
    using IDirectTextCallback3::ProcessDirectText;
public:
    virtual bool ProcessDirectText(const TDirectTextData2& directText, const TDocInfoEx* docInfo, const TFullDocAttrs* docAttrs, ui32 docId, const TDisambMask* masks) = 0;
    using IDirectTextCallback3::MakePortion;
};

}
