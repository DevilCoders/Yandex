#include "glue.h"
#include "tsnip.h"
#include "zones_helper.h"

#include <kernel/snippets/sent_match/callback.h>

#include <kernel/snippets/archive/zone_checker/zone_checker.h>
#include <kernel/snippets/config/config.h>
#include <kernel/snippets/iface/archive/segments.h>
#include <kernel/snippets/sent_info/sent_info.h>
#include <kernel/snippets/simple_textproc/decapital/decapital.h>

#include <kernel/snippets/strhl/hilite_mark.h>
#include <kernel/snippets/strhl/glue_common.h>
#include <kernel/snippets/strhl/zs_transformer.h>

#include <util/generic/algorithm.h>
#include <util/generic/singleton.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/generic/queue.h>
#include <util/charset/wide.h>
#include <util/generic/strbuf.h>
#include <util/generic/hash.h>

namespace NSnippets {

const THiliteMark TGluer::ParaMark(u"\u00B6", TUtf16String()); //&para;, Pilcrow Sign, U+00B6
static const TUtf16String INTERNAL_PUNCT = u",:;-";

struct TGluer::TImpl {
    TZonedString Res;
    const THiliteMark* EllipsisMark;
    const TSingleSnip* Snip;
    TImpl(const TSingleSnip* ss, const THiliteMark* ellipsis)
      : Res(TUtf16String())
      , EllipsisMark(ellipsis)
      , Snip(ss)
    {
    }
};

void TGluer::MarkMatches(const THiliteMark* mark) {
    const TSentsInfo& si = Impl->Snip->GetSentsMatchInfo()->SentsInfo;
    TZonedString::TZone& res = Impl->Res.Zones[+TZonedString::ZONE_MATCH];
    res.Mark = mark;
    for (int i = Impl->Snip->GetFirstWord(); i <= Impl->Snip->GetLastWord(); ++i) {
        if (Impl->Snip->GetSentsMatchInfo()->IsMatch(i)) {
            res.Spans.push_back(TZonedString::TSpan(si.GetWordBuf(i)));
        }
    }
}

void TGluer::MarkMatchedPhones(const THiliteMark* mark) {
    const TSentsInfo& si = Impl->Snip->GetSentsMatchInfo()->SentsInfo;
    TZonedString::TZone& res = Impl->Res.Zones[+TZonedString::ZONE_MATCHED_PHONE];
    res.Mark = mark;
    for (int i = Impl->Snip->GetFirstWord(); i <= Impl->Snip->GetLastWord(); ++i) {
        if (Impl->Snip->GetSentsMatchInfo()->IsExactUserPhone(i)) {
            res.Spans.push_back(TZonedString::TSpan(si.GetWordBuf(i)));
        }
    }
}

void TGluer::MarkExt(const TVector<std::pair<int, int>>& shortSpans, const THiliteMark* mark) {
    const TSentsInfo& si = Impl->Snip->GetSentsMatchInfo()->SentsInfo;
    TZonedString::TZone& res = Impl->Res.Zones[+TZonedString::ZONE_EXTSNIP];
    res.Mark = mark;
    const int w0 = Impl->Snip->GetFirstWord();
    const int w1 = Impl->Snip->GetLastWord();
    for (const auto& span : shortSpans) {
        if (w0 <= span.first && span.second <= w1) {
            TWtringBuf spanBuf = si.GetTextBuf(span.first, span.second);
            // Expand zone to the ellipsis if needed
            if (Impl->EllipsisMark) {
                if (span.first == w0 && !si.IsWordIdFirstInSent(w0)) {
                    const size_t shift = Impl->EllipsisMark->OpenTag.size();
                    spanBuf = TWtringBuf(spanBuf.data() - shift, spanBuf.size() + shift);
                }
                if (span.second == w1 && !si.IsWordIdLastInSent(w1)) {
                    const size_t shift = Impl->EllipsisMark->CloseTag.size();
                    spanBuf = TWtringBuf(spanBuf.data(), spanBuf.size() + shift);
                }
            }
            res.Spans.push_back(TZonedString::TSpan(spanBuf));
        }
    }
}

void TGluer::MarkParabeg(const THiliteMark* mark) {
    const TSentsInfo& si = Impl->Snip->GetSentsMatchInfo()->SentsInfo;
    TZonedString::TZone& res = Impl->Res.Zones[+TZonedString::ZONE_PARABEG];
    res.Mark = mark;
    int s0 = Impl->Snip->GetFirstSent();
    int s1 = Impl->Snip->GetLastSent();
    if (Impl->Snip->GetFirstWord() != si.FirstWordIdInSent(s0)) {
        ++s0;
    }
    for (int sentId = s0; sentId <= s1; ++sentId) {
        if (si.IsSentIdFirstInPara(sentId)) {
            res.Spans.push_back(TZonedString::TSpan(si.GetSentBuf(sentId).Head(1)));
        }
    }
}

void TGluer::MarkPunctTrash(const THiliteMark* mark) {
    const TSentsInfo& si = Impl->Snip->GetSentsMatchInfo()->SentsInfo;
    TZonedString::TZone& res = Impl->Res.Zones[+TZonedString::ZONE_TRASH];
    res.Mark = mark;
    if (si.IsWordIdLastInSent(Impl->Snip->GetLastWord())) {
        TWtringBuf sentEndBlanks = si.GetSentEndBlanks(Impl->Snip->GetLastSent());
        while (sentEndBlanks) {
            if (INTERNAL_PUNCT.Contains(sentEndBlanks[0])) {
                res.Spans.push_back(TZonedString::TSpan(sentEndBlanks));
                break;
            }
            sentEndBlanks.Skip(1);
        }
    }
}

static void MarkArchiveZone(const TGluer::TImpl* impl, TZonedString::TZone& res, const EArchiveZone zoneName) {
    const TSentsInfo& si = impl->Snip->GetSentsMatchInfo()->SentsInfo;
    TVector<TZoneWords> borders;
    GetZones(*impl->Snip, zoneName, borders, false);
    for (size_t i = 0; i < borders.size(); ++i) {
        res.Spans.push_back(TZonedString::TSpan(si.GetWordSpanBuf(borders[i].FirstWordId, borders[i].LastWordId)));
    }
}

void TGluer::MarkFio(const THiliteMark* mark) {
    TZonedString::TZone& res = Impl->Res.Zones[+TZonedString::ZONE_FIO];
    res.Mark = mark;
    MarkArchiveZone(Impl.Get(), res, AZ_FIO);
}

void TGluer::MarkAllPhones(const THiliteMark* mark) {
    TZonedString::TZone& res = Impl->Res.Zones[+TZonedString::ZONE_PHONES];
    res.Mark = mark;
    MarkArchiveZone(Impl.Get(), res, AZ_TELEPHONE);
}

void TGluer::MarkSentences(const THiliteMark* mark) {
    const TSentsInfo& si = Impl->Snip->GetSentsMatchInfo()->SentsInfo;
    TZonedString::TZone& res = Impl->Res.Zones[+TZonedString::ZONE_SENTENCE];
    res.Mark = mark;
    int w0 = Impl->Snip->GetFirstWord();
    int w1 = Impl->Snip->GetLastWord();
    int s0 = Impl->Snip->GetFirstSent();
    int s1 = Impl->Snip->GetLastSent();
    for (int sentId = s0; sentId <= s1; ++sentId) {
        int firstWord = Max(w0, si.FirstWordIdInSent(sentId));
        int lastWord = Min(w1, si.LastWordIdInSent(sentId));
        res.Spans.push_back(TZonedString::TSpan(si.GetWordSentSpanBuf(firstWord, lastWord)));
    }
}

void TGluer::MarkMenuWords(const THiliteMark* mark) {
    const TSentsInfo& si = Impl->Snip->GetSentsMatchInfo()->SentsInfo;
    const NSegments::TSegmentsInfo *segments = si.GetSegments();
    if (!segments) {
        return;
    }
    TZonedString::TZone& res = Impl->Res.Zones[+TZonedString::ZONE_MENU];
    res.Mark = mark;
    for (int i = Impl->Snip->GetFirstWord(); i <= Impl->Snip->GetLastWord(); ++i) {
        const int sId = si.WordId2SentId(i);
        if (segments->GetType(segments->GetArchiveSegment(si.GetArchiveSent(sId))) == NSegm::STP_MENU) {
            res.Spans.push_back(TZonedString::TSpan(si.GetWordBuf(i)));
        }
    }
}

void TGluer::MarkLinks(const TVector<TZonedString::TSpan>& linkSpans, const THiliteMark* mark) {
    const TSentsInfo& si = Impl->Snip->GetSentsMatchInfo()->SentsInfo;
    TZonedString::TZone& res = Impl->Res.Zones[+TZonedString::ZONE_LINK];
    res.Mark = mark;
    if (linkSpans) {
        // find spans belonging to the current TSingleSnip
        TWtringBuf wordSpan = si.GetWordSpanBuf(Impl->Snip->GetFirstWord(), Impl->Snip->GetLastWord());
        for (size_t i = 0; i < linkSpans.size(); ++i) {
            if ((wordSpan.data() <= ~linkSpans[i]) &&
                (~linkSpans[i] + +linkSpans[i] <= wordSpan.data() + wordSpan.size())) {
                res.Spans.push_back(linkSpans[i]);
            }
        }
    }
}

void TGluer::MarkTableCells(const THiliteMark* mark) {
    const TSentsInfo& si = Impl->Snip->GetSentsMatchInfo()->SentsInfo;
    TZonedString::TZone& res = Impl->Res.Zones[+TZonedString::ZONE_TABLE_CELL];
    res.Mark = mark;
    // find spans belonging to the current TSingleSnip
    if (Impl->Snip->GetSnipType() == SPT_TABLE) {
        TVector<TZoneWords> borders;
        GetZones(*Impl->Snip, AZ_TABLE_CELL, borders, false, true);
        for (const TZoneWords& border : borders) {
            // In fact, the cell zone boundary is always coincide with some sentence boundary
            res.Spans.push_back(TZonedString::TSpan(si.GetWordSentSpanBuf(border.FirstWordId, border.LastWordId)));
        }
    }
}
static void DoMoveFromBuf(TZonedString& s, const TWtringBuf& src);
static void DoMarkPar(TZonedString& s, const TZonedString::TZone& par);
static void DoZone(TZonedString& s, const TZonedString::TZone& z);
static void DoMergeEllipsis(TZonedString& s, const THiliteMark* ellipsis);
static void DoCutTrash(TZonedString& s, const TZonedString::TSpans& trash);
static void DoDecapitalize(const TZonedString::TSpans& trash, TDecapitalizer& decapitalizer, bool byOneWord);
static void DecapitalizeZone(TZonedString& z, const TZonedString::EZoneName zoneName, TDecapitalizer& decapitalizer);

TZonedString TGluer::EmbedPara(const TZonedString& z) {
    TZonedString::TZones::const_iterator i = z.Zones.find(+TZonedString::ZONE_PARABEG);
    if (i == z.Zones.end()) {
        return z;
    }
    TZonedString res = z;
    if (!i->second.Spans.empty() && i->second.Mark) {
        TZonedString::TZone pbeg = i->second;
        DoMarkPar(res, pbeg);
    }
    res.Zones.erase(+TZonedString::ZONE_PARABEG);
    return res;
}

TZonedString TGluer::EmbedExtMarks(const TZonedString& z) {
    TZonedString::TZones::const_iterator i = z.Zones.find(+TZonedString::ZONE_EXTSNIP);
    if (i == z.Zones.end()) {
        return z;
    }
    TZonedString res = z;
    if (!i->second.Spans.empty() && i->second.Mark) {
        TZonedString::TZone ext = i->second;
        DoZone(res, ext);
    }
    res.Zones.erase(+TZonedString::ZONE_EXTSNIP);
    return res;
}

TZonedString TGluer::EmbedTableCellMarks(const TZonedString& z) {
    TZonedString::TZones::const_iterator i = z.Zones.find(+TZonedString::ZONE_TABLE_CELL);
    if (i == z.Zones.end()) {
        return z;
    }
    TZonedString res = z;
    if (!i->second.Spans.empty() && i->second.Mark) {
        TZonedString::TZone cell = i->second;
        DoZone(res, cell);
    }
    res.Zones.erase(+TZonedString::ZONE_TABLE_CELL);
    return res;
}

TStringBuf CutLastSlash(TStringBuf url) {
    if (url.empty()) {
        return url;
    }
    if (url.back() == '/') {
        return TStringBuf(url.data(), url.size() - 1);
    }
    return url;
}

bool LinkIsGood(const TString& link, const TString& docUrl) {
    const TString normUrl = TString(CutLastSlash(CutWWWPrefix(CutHttpPrefix(docUrl)))); // poor attempt of url normalizing
    const TString normLink = TString(CutLastSlash(CutWWWPrefix(CutHttpPrefix(link))));
    const TString host = TString(GetHost(normUrl));
    return normLink != normUrl && normLink != host;
}

TZonedString TGluer::EmbedLinkMarks(const TZonedString& z, TVector<TString>& links, const TString& docUrl) {
    links.clear();
    TZonedString::TZones::const_iterator i = z.Zones.find(+TZonedString::ZONE_LINK);
    if (i == z.Zones.end()) {
        return z;
    }
    TZonedString res = z;
    if (!i->second.Spans.empty() && i->second.Mark) {
        TZonedString::TZone linkZone = i->second;
        // filter and save links
        int cnt = 0;
        for (size_t j = 0; j < linkZone.Spans.size(); ++j) {
            const TZonedString::TSpan& zone = linkZone.Spans[j];
            THashMap<TString, TUtf16String>::const_iterator attrsIter = zone.Attrs.find(NArchiveZoneAttr::NAnchorInt::SEGMLINKINT);
            Y_ASSERT(attrsIter != zone.Attrs.end());
            const TString link = WideToUTF8(attrsIter->second);
            if (LinkIsGood(link, docUrl)) { // we don't want to collect links to the same document and to home page
                links.push_back(link);
                DoSwap(linkZone.Spans[j], linkZone.Spans[cnt]);
                ++cnt;
            }
        }
        linkZone.Spans.resize(cnt);

        // mark links
        DoZone(res, linkZone);
    }
    res.Zones.erase(+TZonedString::ZONE_LINK);
    return res;
}

TZonedString TGluer::Decapitalize(const TZonedString& z, ELanguage lang,
    ISnippetDebugOutputHandler* debug) {
    TZonedString res = z;
    TDecapitalizer decapitalizer(res.String, lang);
    DecapitalizeZone(res, TZonedString::ZONE_MENU, decapitalizer);
    DecapitalizeZone(res, TZonedString::ZONE_SENTENCE, decapitalizer);

    // Hope nobody hasn't erase ZONE_FIO zone yet...
    TZonedString::TZones::const_iterator zIt = z.Zones.find(+TZonedString::ZONE_FIO);
    if (zIt != z.Zones.end()) {
        TZonedString::TZone sent = zIt->second;
        for (size_t i = 0; i < sent.Spans.size(); ++i) {
            decapitalizer.RevertFioChanges(~sent.Spans[i], +sent.Spans[i]);
        }
    }
    decapitalizer.Complete(res);

    if (debug) {
        if (z.String != res.String) {
            debug->Print(false, "Snippet was decapitalized. Origin value was: \"%s\"", WideToUTF8(z.String).data());
        }
    }

    return res;
}

TZonedString TGluer::CutTrash(const TZonedString& z) {
    TZonedString::TZones::const_iterator i = z.Zones.find(+TZonedString::ZONE_TRASH);
    if (i == z.Zones.end()) {
        return z;
    }
    TZonedString res = z;
    if (!i->second.Spans.empty()) {
        TZonedString::TZone trash = i->second;
        DoCutTrash(res, trash.Spans);
    }
    res.Zones.erase(+TZonedString::ZONE_TRASH);
    return res;
}

static inline TZonedString MergeEllipsis(TGluer::TImpl& gluer) {
    DoMoveFromBuf(gluer.Res, gluer.Snip->GetTextBuf());
    DoMergeEllipsis(gluer.Res, gluer.EllipsisMark);
    gluer.Snip = nullptr;
    gluer.EllipsisMark = nullptr;
    return gluer.Res;
}


TZonedString TGluer::GlueToZonedString() const {
    return MergeEllipsis(*Impl);
}

TGluer::TGluer(const TSingleSnip* ss, const THiliteMark* ellipsis)
  : Impl(new TImpl(ss, ellipsis))
{
}

TGluer::~TGluer() {
}

static void DoMoveFromBuf(TZonedString& s, const TWtringBuf& src) {
    s.String = TUtf16String(src.data(), src.size());
    s.ShiftZonesSpans(s.String.data() - src.data());
}

static void DoMergeEllipsis(TZonedString& s, const THiliteMark* ellipsis) {
    if (!ellipsis) {
        return;
    }
    const TWtringBuf src = s.String;
    s.String = ellipsis->OpenTag + s.String + ellipsis->CloseTag;
    s.ShiftZonesSpans((s.String.data() + ellipsis->OpenTag.size()) - src.data());
}

static void DoMarkPar(TZonedString& s, const TZonedString::TZone& par) {
    TZonedStringTransformer t(s);
    size_t j = 0;
    for (size_t i = 0; i < s.String.size(); ++i) {
        while (j < par.Spans.size() && ~par.Spans[j] < s.String.data() + i) {
            ++j;
        }
        if (j < par.Spans.size() && ~par.Spans[j] == s.String.data() + i) {
            t.InsertBeforeNext(par.Mark->OpenTag);
        }
        t.Step();
    }
    t.Complete();
}

static void DoZone(TZonedString& s, const TZonedString::TZone& z) {
    TZonedStringTransformer t(s);
    size_t j = 0;
    for (size_t i = 0; i < s.String.size(); ++i) {
        while (j < z.Spans.size() && ~z.Spans[j] + +z.Spans[j] <= s.String.data() + i) {
            ++j;
        }
        if (j < z.Spans.size() && ~z.Spans[j] == s.String.data() + i) {
            t.InsertBeforeNext(z.Mark->OpenTag);
        }
        t.Step();
        if (j < z.Spans.size() && ~z.Spans[j] + +z.Spans[j] == s.String.data() + i + 1) {
            t.InsertBeforeNext(z.Mark->CloseTag);
        }
    }
    t.Complete();
}

static void DoCutTrash(TZonedString& s, const TZonedString::TSpans& trash) {
    TZonedStringTransformer t(s);
    size_t j = 0;
    for (size_t i = 0; i < s.String.size(); ++i) {
        while (j < trash.size() && ~trash[j] + +trash[j] <= s.String.data() + i) {
            ++j;
        }
        if (j < trash.size() && ~trash[j] <= s.String.data() + i) {
            t.Delete(1);
        } else {
            t.Step();
        }
    }
    t.Complete();
}

static void DoDecapitalize(const TZonedString::TSpans& sent, TDecapitalizer& decapitalizer, bool byOneWord) {
    for (size_t i = 0; i < sent.size(); ++i) {
        if (byOneWord) {
            decapitalizer.DecapitalSingleWord(~sent[i], +sent[i]);
        } else {
            decapitalizer.DecapitalSentence(~sent[i], +sent[i]);
        }
    }
}

static void DecapitalizeZone(TZonedString& z, const TZonedString::EZoneName zoneName, TDecapitalizer& decapitalizer) {
    TZonedString::TZones::const_iterator i = z.Zones.find(+zoneName);
    if (i == z.Zones.end()) {
        return;
    }
    if (!i->second.Spans.empty()) {
        TZonedString::TZone sent = i->second;
        DoDecapitalize(sent.Spans, decapitalizer,  zoneName == TZonedString::ZONE_MENU);
    }
    z.Zones.erase(zoneName);
}

TUtf16String TGluer::GlueToString(const TZonedString& z) {
    return MergedGlue(z);
}

TString TGluer::GlueToHtmlEscapedUTF8String(const TZonedString& z) {
    return WideToUTF8(MergedGlue(HtmlEscape(z)));
}

static inline TUtf16String JustGlueWithSpace(const TVector<TUtf16String>& v) {
    const wchar16 SPACE_CHAR = ' ';
    TUtf16String res;
    for (size_t i = 0; i < v.size(); ++i) {
        if (v[i].size()) {
            if (res.size())
                res.append(SPACE_CHAR);
            res.append(v[i]);
        }
    }
    return res;
}

TUtf16String GlueTitle(const TVector<TUtf16String>& titles) {
    return JustGlueWithSpace(titles);
}

TUtf16String GlueHeadline(const TVector<TUtf16String>& headlines) {
    return JustGlueWithSpace(headlines);
}

// Whatever you do, this function is not supposed to change the string length
void FixWeirdChars(TUtf16String& s) {
    wchar16* p = s.begin();
    for (size_t i = 0; i < s.size(); ++i) {
        if (p[i] == wchar16(0xA0) || p[i] == wchar16(0x202E))
            p[i] = wchar16(' ');
    }
}

}
