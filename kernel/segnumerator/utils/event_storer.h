#pragma once

#include "storer.h"

#include <util/generic/algorithm.h>
#include <util/memory/pool.h>

namespace NSegm {

enum ESegEventType {
    SET_NONE = 0,

    SET_SPACE = 0x1,
    SET_SENTBRK = 0x2 | SET_SPACE,
    SET_PARABRK = 0x4 | SET_SENTBRK,
    SET_TREEBRK = 0x8 | SET_SPACE,

    SET_TOKEN = 0x10,
    SET_IMAGE_INT = 0x20,
    SET_IMAGE_EXT = 0x40,

    SET_SPAN_BEGIN = 0x100,
    SET_SPAN_END = 0x200,

    SET_LINKINT_OPEN = 0x1000 | SET_SPAN_BEGIN,
    SET_LINKEXT_OPEN = 0x2000 | SET_SPAN_BEGIN,
    SET_LINK_CLOSE = 0x3000 | SET_SPAN_END,

    SET_BLOCK_OPEN = 0x4000 | SET_SPAN_BEGIN | SET_TREEBRK,
    SET_BLOCK_CLOSE = 0x8000 | SET_SPAN_END | SET_TREEBRK,

    SET_TEXT_END = 0x10000 | SET_PARABRK
};

struct TSegEvent {
    ESegEventType Type;
    TWtringBuf Text;
    TStringBuf Attr;

    TAlignedPosting Pos;

    TSegEvent(ESegEventType t = SET_NONE, TAlignedPosting pos = 0)
        : Type(t)
        , Pos(pos)
    {
    }

    template <ESegEventType t>
    bool IsA() const {
        return (Type & t) == t;
    }

    template <ESegEventType t>
    bool IsOnly() const {
        return Type == t;
    }

    operator TPosting() const {
        return Pos;
    }
};

class TEventStorage;
struct TRange;
typedef TVector<TRange> TRanges;

struct TSimpleTextConcatenator {
    TUtf16String& Res;
    TPosCoords& Coords;

    TSimpleTextConcatenator(TUtf16String& res, TPosCoords& pos)
        : Res(res)
        , Coords(pos)
    {}

    void Do(const TRange& r);
};

struct TRange {
    const TEventStorage* Storage;
    size_t Begin;
    size_t End;

    TRange()
        : Storage()
        , Begin()
        , End()
    {}

    TRange(const TEventStorage* s, size_t b, size_t e)
        : Storage(s)
        , Begin(b)
        , End(e)
    {}

    TRange(const TEventStorage* s, const TSegEvent* b, const TSegEvent* e);

    template <typename TTextConcatenator>
    void ConcatenateText(TTextConcatenator& c) const {
        c.Do(*this);
    }

    void ConcatenateText(TUtf16String& res, TPosCoords& pos) const {
        TSimpleTextConcatenator c(res, pos);
        ConcatenateText(c);
    }

    TUtf16String ConcatenateText() const {
        TUtf16String w;
        TPosCoords pos;
        ConcatenateText(w, pos);
        return w;
    }

    TRange SelectBySpan(const TBaseSpan& s) const;

    void SelectSpanEndsByAttr(TRanges& res, TStringBuf attr) const;

    template <ESegEventType type>
    inline void SelectByBreak(TRanges& res) const;

    template <ESegEventType type>
    inline void SelectWithBreaks(TRanges& res) const;

    template <typename TIter>
    inline void SelectBySortedSpans(TIter bs, TIter es, TRanges& res) const;

    inline const TSegEvent* begin() const;
    inline const TSegEvent* end() const;

    inline const TSegEvent& front() const;
    inline const TSegEvent& back() const;

    size_t size() const {
        return End - Begin;
    }

    bool empty() const {
        return !size();
    }
};


class TEventStorage : public TVector<TSegEvent> {
    TMemoryPool Pool { 100000 };

public:
    TSegEvent& PushBack(const TSegEvent& e = TSegEvent(), TWtringBuf t = TWtringBuf(), TStringBuf a = TStringBuf(), bool alloc = true);

    void InsertSpan(const TBaseSpan& s, TStringBuf a, TWtringBuf bt = TWtringBuf(), TWtringBuf et = TWtringBuf());

    TRange AsRange() const {
        return TRange(this, 0, size());
    }

    void Clear() {
        clear();
        Pool.Clear();
    }

    TEventStorage& operator=(const TEventStorage& s) {
        Pool.ClearKeepFirstChunk();
        clear();
        for (TEventStorage::const_iterator it = s.begin(); it != s.end(); ++it)
            PushBack(*it, it->Text, it->Attr, true);
        return *this;
    }

private:
    template <typename Char>
    TBasicStringBuf<Char> Allocate(const Char* c, const size_t s) {
        return TBasicStringBuf<Char>(Pool.Append(c, s), s);
    }
};

class TEventStorer : public IStorer {
    bool AllocateTokenCopy = true;

    TEventStorage Title;
    TEventStorage Body;

public:
    void Clear() {
        Title.Clear();
        Body.Clear();
    }

    void SetAllocateTokenCopy(bool a) {
        AllocateTokenCopy = a;
    }

    TEventStorage& GetStorage(bool title) {
        return title ? Title : Body;
    }

    const TEventStorage& GetStorage(bool title) const {
        return const_cast<TEventStorer*>(this)->GetStorage(title);
    }

    void OnToken(bool title, const TWideToken& t, TAlignedPosting pos) override;

    void OnSubToken(bool title, const wchar16* c, unsigned s, TAlignedPosting pos);

    void OnSpaces(bool title, TBreakType b, const wchar16* c, unsigned s, TAlignedPosting pos) override;

    void OnTextEnd(TAlignedPosting) override;

    // tree builder events

    void OnLinkOpen(const TString& url, ELinkType linktype, TAlignedPosting pos) override {
        GetStorage(false).PushBack(TSegEvent(linktype == LT_EXTERNAL_LINK ? SET_LINKEXT_OPEN : SET_LINKINT_OPEN, pos), TWtringBuf(), url);
    }

    void OnLinkClose(TAlignedPosting pos) override {
        GetStorage(false).PushBack(TSegEvent(SET_LINK_CLOSE, pos));
    }

    void OnBlockOpen(HT_TAG t, TAlignedPosting p) override {
        GetStorage(false).PushBack(TSegEvent(SET_BLOCK_OPEN, p)).Attr = NHtml::FindTag(t).lowerName;
    }

    void OnBlockClose(HT_TAG t, TAlignedPosting p) override {
        GetStorage(false).PushBack(TSegEvent(SET_BLOCK_CLOSE, p)).Attr = NHtml::FindTag(t).lowerName;
    }

    void OnBreak(HT_TAG t, TAlignedPosting p) override {
        GetStorage(false).PushBack(TSegEvent(SET_TREEBRK, p)).Attr = NHtml::FindTag(t).lowerName;
    }

    void OnImage(const TString& src, const TString& alt, ELinkType linktype, TAlignedPosting pos) override;
};

const TSegEvent* TRange::begin() const {
    return Storage->begin() + Begin;
}

const TSegEvent* TRange::end() const {
    return Storage->begin() + End;
}

const TSegEvent& TRange::front() const {
    return *Storage->begin();
}

const TSegEvent& TRange::back() const {
    return *(Storage->end() - 1);
}

template <ESegEventType type>
void TRange::SelectByBreak(TRanges& res) const {
    const TSegEvent* last = begin();

    for(const TSegEvent* it = begin(); it != end(); ++it) {
        if (it->IsA<type>()) {
            res.push_back(TRange(Storage, last, it));
            last = it;
        }
    }

    if (last != end())
        res.push_back(TRange(Storage, last, end()));
}

template <ESegEventType type>
void TRange::SelectWithBreaks(TRanges& res) const {
    const TSegEvent* it = begin();
    while (it != end()) {
        const TSegEvent* start = it;
        while (it != end() && !it->IsA<type>()) {
            ++it;
        }
        while (it != end() && it->IsA<type>()) {
            ++it;
        }
        res.push_back(TRange(Storage, start, it));
    }
}

template <typename TIter>
void TRange::SelectBySortedSpans(TIter bs, TIter es, TRanges& res) const {
    const TSegEvent* last = begin();

    for (; bs != es; ++bs) {
        last = LowerBound(last, end(), bs->Begin);
        TEventStorage::const_iterator e = LowerBound(last, end(), bs->End);
        res.push_back(TRange(Storage, last, e));
        last = e;
    }
}

}
