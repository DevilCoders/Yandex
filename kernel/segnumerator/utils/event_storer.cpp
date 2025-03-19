#include "event_storer.h"
#include <library/cpp/html/entity/htmlentity.h>

namespace NSegm {

const wchar16 nullspace[] = { 0x20 };

TRange TRange::SelectBySpan(const TBaseSpan& s) const {
    const TSegEvent* last = LowerBound(begin(), end(), s.Begin);
    return TRange(Storage, last, LowerBound(last, end(), s.End));
}

void TSimpleTextConcatenator::Do(const TRange& r) {
    size_t sz = 0;

    for (const TSegEvent* beg = r.begin(); beg != r.end(); ++beg)
        sz += beg->Text.size();

    Res.reserve(sz);
    Coords.reserve(r.size());

    for (const TSegEvent* s = r.begin(); s != r.end(); ++s) {
        if (s->IsA<SET_SPACE>() && !s->Text /* fake space */) {
            Coords.push_back(TPosCoord(s->Pos, Res.size(), Res.size() + 1));
            Res.append(' ');
        } else if (s->IsA<SET_TOKEN>() || s->IsA<SET_SPACE>()) {
            Coords.push_back(TPosCoord(s->Pos, Res.size(), Res.size() + s->Text.size()));
            Res.append(s->Text);
        }
    }
}

void TRange::SelectSpanEndsByAttr(TRanges& res, TStringBuf attr) const {
    const TSegEvent* last = begin();

    for (const TSegEvent* it = last; it != end(); ++it) {
        if (it->IsOnly<SET_SPAN_BEGIN>() && it->Attr == attr) {
            last = it;

            for (; it != end(); ++it) {
                if(it->IsOnly<SET_SPAN_END>() && it->Attr == attr)
                    break;
            }

            res.push_back(TRange(Storage, last, it));
        }
    }
}


TRange::TRange(const TEventStorage* s, const TSegEvent* b, const TSegEvent* e)
    : Storage(s)
    , Begin(b - s->begin())
    , End(e - s->begin())
{}

TSegEvent& TEventStorage::PushBack(const TSegEvent& e, TWtringBuf t, TStringBuf a, bool alloc) {
    push_back(e);
    back().Text = alloc ? Allocate(t.data(), t.size()) : t;
    back().Attr = alloc ? Allocate(a.data(), a.size()) : a;
    return back();
}

void TEventStorage::InsertSpan(const TBaseSpan& s, TStringBuf a, TWtringBuf bt, TWtringBuf et) {
    TSegEvent* b = LowerBound(begin(), end(), s.Begin);

    if (b == end())
        return;

    while (b != end() && b->IsA<SET_SPAN_END>())
        ++b;

    b = insert(b, TSegEvent(SET_SPAN_BEGIN, s.Begin));
    b->Text = Allocate(bt.data(), bt.size());
    b->Attr = a = Allocate(a.data(), a.size());

    TSegEvent* e = LowerBound(b + 1, end(), s.End); // b cannot be end()

    while (e != end() && e->IsA<SET_SPAN_END>())
        ++e;

    e = insert(e, TSegEvent(SET_SPAN_END, s.End));
    e->Text = Allocate(et.data(), et.size());
    e->Attr = a;
}

void TEventStorer::OnToken(bool title, const TWideToken& t, TAlignedPosting pos) {
    TWtringBuf txt(t.Token, t.Leng);

    for (ui32 i = 0; i < t.SubTokens.size(); ++i) {
        const TCharSpan& sp = t.SubTokens[i];

        TAlignedPosting tp(pos.Sent(), pos.Word() + i);

        OnSubToken(title, txt.data() + sp.Pos - sp.PrefixLen, sp.Len + sp.SuffixLen + sp.PrefixLen, tp);

        if (sp.TokenDelim != TOKDELIM_NULL) {
            wchar16 rtd = GetRightTokenDelim(t, i);

            if (rtd)
                OnSpaces(title, ST_NOBRK, &rtd, 1, tp);
            else
                OnSpaces(title, ST_NOBRK, nullptr, 0, tp);
        }
    }
}

void TEventStorer::OnSubToken(bool title, const wchar16* c, unsigned s, TAlignedPosting pos) {
    if (s)
        GetStorage(title).PushBack(TSegEvent(SET_TOKEN, TAlignedPosting(pos.Sent(), pos.Word())),
                                   TWtringBuf(c, s), TStringBuf(), AllocateTokenCopy);
}

void TEventStorer::OnSpaces(bool title, TBreakType b, const wchar16* c, unsigned s, TAlignedPosting pos) {
    GetStorage(title).PushBack(
                    TSegEvent(IsParaBrk(b) ? SET_PARABRK : IsSentBrk(b) ? SET_SENTBRK : SET_SPACE, pos),
                    (s ? TWtringBuf(c, s) : TWtringBuf(nullspace, 1)), TStringBuf(), AllocateTokenCopy);
}

void TEventStorer::OnImage(const TString& src, const TString& alt, ELinkType linktype, TAlignedPosting pos) {
    TTempArray<wchar16> buf(alt.size() * 2);
    if (alt.size()) {
        buf.Proceed(HtEntDecodeToChar(Parser->GetCharset(), alt.data(), alt.size(), buf.Data()));
    }
    GetStorage(false).PushBack(TSegEvent(linktype == LT_EXTERNAL_LINK ? SET_IMAGE_EXT : SET_IMAGE_INT, pos), TWtringBuf(buf.Data(), buf.Filled()), src);
}

void TEventStorer::OnTextEnd(TAlignedPosting pos) {
    GetStorage(false).PushBack(TSegEvent(SET_TEXT_END, pos));
}

}

