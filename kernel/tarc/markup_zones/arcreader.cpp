#include "arcreader.h"
#include "text_markup.h"

#include <kernel/tarc/docdescr/docdescr.h>
#include <kernel/tarc/iface/tarcface.h>

#include <kernel/keyinv/invkeypos/keyconv.h>
#include <kernel/segmentator/structs/segment_span_decl.h>
#include <kernel/xref/xref_types.h>

#include <library/cpp/charset/doccodes.h>
#include <library/cpp/charset/wide.h>
#include <library/cpp/containers/mh_heap/mh_heap.h>
#include <library/cpp/langs/langs.h>

#include <util/datetime/base.h>
#include <util/stream/str.h>


struct TDocInfosValueTypeLess {
    bool operator()(const TDocInfos::value_type* x, const TDocInfos::value_type* y) const {
        return strcmp(x->first, y->first) < 0;
    }
};

void PrintExtInfo(IOutputStream& os, const TBlob& extInfo, bool forceUTF8) {
    TDocDescr docDescr;
    docDescr.UseBlob(extInfo.Data(), (unsigned int)extInfo.Size());
    if (!docDescr.IsAvailable()) {
        os << '\n';
        return;
    }

    ui32 id = docDescr.get_hostid();
    if (id)
        os << "HOST_ID=" <<  id << '\n';
    id = docDescr.get_urlid();
    if (id)
        os << "URL_ID=" <<  id << '\n';
    os << "SIZE=" <<  (ui32)docDescr.get_size() << '\n';

    time_t t = docDescr.get_mtime();
    if (t) {
        char buf[26];
        const char* tt = ctime_r(&t, buf);
        if (tt)
            os << "TIME=" << tt;
    }

    const char *h = docDescr.get_host();
    if (h && *h)
        os << "HOST=" << h << '\n';

    os << "URL=" << docDescr.get_url() << '\n';

    const char* mime_str  = strByMime(docDescr.get_mimetype());
    if (mime_str)
        os << "MIMETYPE=" << mime_str << '\n';

    ECharset e = docDescr.get_encoding();
    if (CODES_UNKNOWN < e && e < CODES_MAX)
        os << "CHARSET=" <<  NameByCharset(e) << '\n';

    TBuildInfoExt* buildInfo = (TBuildInfoExt*) docDescr.GetExtension(BuildInfo);
    if (buildInfo) {
        os << "BUILD_INFO=" << buildInfo->SvnUrl << ".r" << buildInfo->Revision << '\n';
    }

    TDocInfos docInfos;
    docDescr.ConfigureDocInfos(docInfos);

    typedef TVector<const TDocInfos::value_type*> TSortedDocInfos;
    TSortedDocInfos sortedDocInfos;
    sortedDocInfos.reserve(docInfos.size());
    for (TDocInfos::const_iterator it = docInfos.begin(); it != docInfos.end(); ++it)
        sortedDocInfos.push_back(&(*it));
    Sort(sortedDocInfos.begin(), sortedDocInfos.end(), TDocInfosValueTypeLess());

    for (TSortedDocInfos::const_iterator i = sortedDocInfos.begin(); i != sortedDocInfos.end(); ++i) {
        TString value;
        if (e == CODES_YANDEX && forceUTF8)
            ConvertFromYandexToUTF8((*i)->second, value);
        else
            value = (*i)->second;

        os << TString((*i)->first) << "=" << value << '\n';
    }
}

class TPrintZoneSpanIterator : public TZoneSpanIterator
{
private:
    const TUtf16String BegMark;
    const TUtf16String EndMark;
    const ui8 SegVersion;
    const ui32 ZoneId;
public:
    TPrintZoneSpanIterator(TArchiveZoneSpan* cur, TArchiveZoneSpan* end, const TString& bm, const TString& em, ui32 zoneId, ui8 segVersion, TArchiveZoneAttrs* attrs = nullptr)
        : TZoneSpanIterator(cur, end, attrs)
        , BegMark(CharToWide(bm, csYandex))
        , EndMark(CharToWide(em, csYandex))
        , SegVersion(segVersion)
        , ZoneId(zoneId)
    {}

    void AppendAttribute(const char* attrName, const TUtf16String& attrValue, TUtf16String& result) const
    {
        static const wchar16 STYLE[] = {' ', '=', '\"'};

        result.insert(result.end() - 1, STYLE[0]);
        result.insert(result.size() - 1, ASCIIToWide(attrName));
        result.insert(result.end() - 1, STYLE[1]);
        result.insert(result.end() - 1, STYLE[2]);
        result.insert(result.size() - 1, attrValue);
        result.insert(result.end() - 1, STYLE[2]);
    }

    TUtf16String GetMark() const {
        if (BegState) {
            TUtf16String res(BegMark);
            TSpanAttributes spanAttributes = GetCurrentAttrs();
            THashMap<TString, TUtf16String>* attrs = spanAttributes.AttrsHash;
            if (attrs) {
                for (THashMap<TString, TUtf16String>::const_iterator it = attrs->begin(); it != attrs->end(); ++it) {
                    TUtf16String attrValue = it->second;
                    if (it->first == NArchiveZoneAttr::NSegm::STATISTICS) {
                        NSegm::TSegmentSpan sp;
                        NSegm::DecodeSegmentSpanFromAttr(sp, it->second.data(), SegVersion);
                        attrValue.assign(CharToWide(sp.ToString(false), csYandex));
                    }
                    AppendAttribute(it->first.data(), attrValue, res);
                }
            }
            TString* segAttrs = spanAttributes.SegmentAttrs;
            if (segAttrs) {
                NSegm::TSegmentSpan sp;
                NSegm::DecodeSegmentSpan(sp, *segAttrs, SegVersion);
                AppendAttribute(NArchiveZoneAttr::NSegm::STATISTICS, CharToWide(sp.ToString(false), csYandex), res);
            }
            return res;
        }
        else {
            return EndMark;
        }
    }

    ui32 GetZoneId() const {
        return ZoneId;
    }
};

struct TPrintZoneSpanIteratorLess {
    bool operator()(const TPrintZoneSpanIterator* x, const TPrintZoneSpanIterator* y) const {
        return x->Current() < y->Current() || (x->Current() == y->Current() && x->GetZoneId() < y->GetZoneId());
    }
};

//
// Simple vector which stores pointers to objects
// and responsible for deleting them
//
template <class T>
class TPointerHolderVector : public TNonCopyable
{
private:
    TVector<T*> Iters;

public:
    typedef typename TVector<T*>::size_type size_type;

    ~TPointerHolderVector() {
        for (T** it = Iters.begin(); it != Iters.end(); ++it)
            delete *it;
    }

    size_type operator+ () const noexcept {
        return Iters.size();
    }

    T** operator~ () {
        return Iters.data();
    }

    void push_back(T* p) {
        Iters.push_back(p);
    }
};

TUtf16String TSentReader::GetText(const TArchiveSent& sent, ui8 /*segVersion*/) const {
    return ReadAttrs ? sent.Text : sent.OnlyText;
}

void PrintDocText(IDocTextOutput& output, const TBlob& docText, bool mZone, bool wZone,
    bool xSent, bool stripTitle, const ISentReader& sentReader, bool mZoneAttrs)
{
    static const wchar16 PARAGRAPH[] = { '<', 'p', '>', '\n' };
    static const wchar16 NEW_LINE[] = { '\n' };

    static const TUtf16String paragraph(PARAGRAPH, Y_ARRAY_SIZE(PARAGRAPH));
    static const TUtf16String newLine(NEW_LINE, Y_ARRAY_SIZE(NEW_LINE));

    TVector<int> sentNumbers;
    TVector<TArchiveSent> outSents;
    TArchiveMarkupZones mZones;
    TArchiveWeightZones wZones;
    unsigned firstTSent = 1, lastTSent = 0; // Полагаем что весь заголовок находится целиков в одном Span-е (это же предполагается в TSearchArchive::FindDoc)
    if (mZone || stripTitle)
        GetArchiveMarkupZones((const ui8*)docText.Data(), &mZones);
    if (stripTitle) {
        TArchiveZoneSpan titleSpan;
        TArchiveZone& z = mZones.GetZone(AZ_TITLE);
        if (!z.Spans.empty()) {
            titleSpan = z.Spans.front();
            firstTSent = titleSpan.SentBeg - 1; // (first|last)TSent - номера предложений от 0 (в Span-е - от 1)
            lastTSent = titleSpan.SentEnd - 1;
        }
    }

    GetSentencesByNumbers((const ui8*)docText.Data(), sentNumbers, &outSents, wZone ? &wZones : nullptr, xSent);

    output.SetSentenceCount((ui32)outSents.size());

    TPointerHolderVector<TPrintZoneSpanIterator> iters;

    if (mZone) {
        for (ui32 id = 0; id < AZ_COUNT; id++) {
            TArchiveZone& z = mZones.GetZone(id);
            if (!z.Spans.empty())
                iters.push_back(new TPrintZoneSpanIterator(z.Spans.data(), z.Spans.data() + z.Spans.size(), Sprintf("<%s>", ToString(static_cast<EArchiveZone>(id))),
                    Sprintf("</%s>", ToString(static_cast<EArchiveZone>(id))), id, mZones.GetSegVersion(), mZoneAttrs ? &mZones.GetZoneAttrs(id) : nullptr));
        }
    }
    if (!wZones.LowZone.Spans.empty())
        iters.push_back(new TPrintZoneSpanIterator(wZones.LowZone.Spans.data(), wZones.LowZone.Spans.data() + wZones.LowZone.Spans.size(), "<w0>", "</w0>", AZ_COUNT, mZones.GetSegVersion()));
    if (!wZones.HighZone.Spans.empty())
        iters.push_back(new TPrintZoneSpanIterator(wZones.HighZone.Spans.data(), wZones.HighZone.Spans.data() + wZones.HighZone.Spans.size(), "<w2>", "</w2>", AZ_COUNT + 1, mZones.GetSegVersion()));
    if (!wZones.BestZone.Spans.empty())
        iters.push_back(new TPrintZoneSpanIterator(wZones.BestZone.Spans.data(), wZones.BestZone.Spans.data() + wZones.BestZone.Spans.size(), "<w3>", "</w3>", AZ_COUNT + 2, mZones.GetSegVersion()));

    MultHitHeap<TPrintZoneSpanIterator, TPrintZoneSpanIteratorLess> zoneHeap(~iters, (ui32)+iters);
    zoneHeap.Restart();

    size_t lastSentence = Min(outSents.size(), output.GetLastSentence());

    for (size_t i = output.GetFirstSentence(); i < lastSentence; i++) {
        if (stripTitle && firstTSent <= i && lastTSent >= i)
            continue;

        const TArchiveSent& as = outSents[i];
        TUtf16String w = sentReader.GetText(as, mZones.GetSegVersion());
        if (zoneHeap.Valid()) {
            ui32 topSent = zoneHeap.Current() >> 16;
            ui32 offOff = 0;
            while (topSent == as.Number) {
                ui32 off = offOff + (zoneHeap.Current() & 0xffff);
                const TUtf16String& ins = zoneHeap.TopIter()->GetMark();
                w.insert(off, ins);
                offOff += (ui32)ins.size();
                ++zoneHeap;
                if (!zoneHeap.Valid())
                    break;
                topSent = zoneHeap.Current() >> 16;
            }
        }

        if (mZone && (as.Flag & SENT_IS_PARABEG)) {
            output.WriteString(paragraph, i);
        }

        w.append(newLine);
        output.WriteString(w, i);
    }
}

class TDocTextOutputBaseImpl : public IDocTextOutput, public TNonCopyable
{
private:
    IOutputStream& Os;

public:
    TDocTextOutputBaseImpl(IOutputStream& os)
        : Os(os)
    {
    }

    void WriteString(const TUtf16String& w, size_t) override {
        Os << WideToUTF8(w);
    }
};

void PrintDocText(IOutputStream& os, const TBlob& docText, bool mZone, bool wZone,
    bool xSent, bool stripTitle, const ISentReader& sentReader, bool mZoneAttrs)
{
    TDocTextOutputBaseImpl output(os);
    PrintDocText(output, docText, mZone, wZone, xSent, stripTitle, sentReader, mZoneAttrs);
}

static inline TWtringBuf GetTextSegment(const TUtf16String& text, size_t pos = 0, size_t len = (size_t)-1) {
    return TWtringBuf(text.data() + pos, Min<size_t>(text.size() - pos, len));
}

static inline void AppendTextToBuffer(IOutputStream& output, const TUtf16String& text, size_t pos = 0, size_t len = (size_t)-1) {
    output << WideToUTF8(GetTextSegment(text, pos, len)) << "\n";
}

void PrintDocText(IOutputStream& os, const TBlob& docText, EArchiveZone zoneId) {
    TArchiveMarkupZones zones;
    GetArchiveMarkupZones((const ui8*)docText.Data(), &zones);

    const TArchiveZone& segContent = zones.GetZone(zoneId);
    TVector<TArchiveSent> sentences;
    GetSentencesByNumbers((const ui8*)docText.Data(), TVector<int>(), &sentences, nullptr, false);

    for (size_t i = 0, s = 0; i < segContent.Spans.size(); ++i) {
        const TArchiveZoneSpan& span = segContent.Spans[i];
        while (s < sentences.size() && sentences[s].Number < span.SentBeg)
            ++s;

        if (s == sentences.size())
            break;

        if (span.SentBeg == span.SentEnd) {
            AppendTextToBuffer(os, sentences[s].OnlyText, span.OffsetBeg, span.OffsetEnd - span.OffsetBeg);
        } else {
            AppendTextToBuffer(os, sentences[s].OnlyText, span.OffsetBeg);
            ++s;

            while (s < sentences.size() && sentences[s].Number < span.SentEnd) {
                AppendTextToBuffer(os, sentences[s].OnlyText);
                ++s;
            }

            if (s < sentences.size())
                AppendTextToBuffer(os, sentences[s].OnlyText, 0, span.OffsetEnd);
        }
    }
}

TUtf16String GetDocZoneText(const TBlob& docText, EArchiveZone zoneId) {
    static const TUtf16String EOL_MARK(u"\n");

    TArchiveMarkupZones zones;
    GetArchiveMarkupZones((const ui8*)docText.Data(), &zones);

    const TArchiveZone& segContent = zones.GetZone(zoneId);
    TVector<TArchiveSent> sentences;
    GetSentencesByNumbers((const ui8*)docText.Data(), TVector<int>(), &sentences, nullptr, false);

    TUtf16String result;
    for (size_t i = 0, s = 0; i < segContent.Spans.size(); ++i) {
        const TArchiveZoneSpan& span = segContent.Spans[i];
        while (s < sentences.size() && sentences[s].Number < span.SentBeg)
            ++s;

        if (s == sentences.size())
            break;

        if (span.SentBeg == span.SentEnd) {
            result += GetTextSegment(sentences[s].OnlyText, span.OffsetBeg, span.OffsetEnd - span.OffsetBeg);
            result += EOL_MARK;
        } else {
            result += GetTextSegment(sentences[s].OnlyText, span.OffsetBeg);
            result += EOL_MARK;

            ++s;

            while (s < sentences.size() && sentences[s].Number < span.SentEnd) {
                result += GetTextSegment(sentences[s].OnlyText);
                result += EOL_MARK;

                ++s;
            }

            if (s < sentences.size()) {
                result += GetTextSegment(sentences[s].OnlyText, 0, span.OffsetEnd);
                result += EOL_MARK;
            }
        }
    }

    return result;
}

static size_t TCharCountToUTF8ByteSize(TStringBuf str, size_t tcharCount)
{
    // 1-byte, 2-byte, 3-byte UTF-8 corresponds to basic unicode plane encoded by one wchar16
    // 4-byte UTF-8 corresponds to other planes encoded by two wchar16-s
    size_t byteSize = 0, tcharCurrent = 0;
    while (tcharCurrent < tcharCount) {
        Y_ENSURE(byteSize < str.size());
        size_t runeLen = UTF8RuneLen(str[byteSize]);
        Y_ASSERT(runeLen); // WideToUTF8 should generate valid UTF-8
        byteSize += runeLen;
        tcharCurrent += (runeLen > 3 ? 2 : 1);
    }
    Y_ENSURE(tcharCurrent == tcharCount);
    return byteSize;
}

void SaveDocTextToProto(NTextArc::TDocArcData& arcData, const TBlob& docBlob) {
    if (docBlob.Empty())
        return;

    const TVector<int> sentNumbers;
    TVector<TArchiveSent> outSents;
    GetSentencesByNumbers(static_cast<const ui8*>(docBlob.Data()), sentNumbers, &outSents, nullptr);

    TStringStream os;

    for (size_t i = 0; i != outSents.size(); ++i) {
        Y_ASSERT(outSents[i].Number > 0);
        size_t index = outSents[i].Number - 1;
        Y_ASSERT(index >= arcData.SentencesSize());

        while (index >= arcData.SentencesSize()) {
            arcData.AddSentences();
        }

        NTextArc::TDocSentence& sent = *arcData.MutableSentences(index);
        TString sentText = WideToUTF8(outSents[i].OnlyText);
        sent.SetOffset(os.Str().size());
        sent.SetSize(sentText.size());
        os << sentText << "\n";
    }

    arcData.SetText(os.Str());

    TArchiveMarkupZones zones;
    GetArchiveMarkupZones(static_cast<const ui8*>(docBlob.Data()), &zones);

    for (size_t id = 0; id != AZ_COUNT; ++id) {
        TArchiveZone& z = zones.GetZone(id);
        for (size_t j = 0; j != z.Spans.size(); ++j) {
            NTextArc::TDocZone& zone = *arcData.AddZones();

            size_t sentBegIndex = z.Spans[j].SentBeg - 1;
            size_t sentEndIndex = z.Spans[j].SentEnd - 1;

            Y_ASSERT(sentBegIndex < arcData.SentencesSize());
            Y_ASSERT(sentEndIndex < arcData.SentencesSize());

            const NTextArc::TDocSentence& sentBeg = arcData.GetSentences(sentBegIndex);
            const NTextArc::TDocSentence& sentEnd = arcData.GetSentences(sentEndIndex);

            TStringBuf sentBegBuf(os.Str().data() + sentBeg.GetOffset(), sentBeg.GetSize());
            TStringBuf sentEndBuf(os.Str().data() + sentEnd.GetOffset(), sentEnd.GetSize());

            // z.Spans[j].OffsetBeg, z.Spans[j].OffsetEnd are offsets in wchar16-s
            size_t prefixByteSize = TCharCountToUTF8ByteSize(sentBegBuf, z.Spans[j].OffsetBeg);
            size_t suffixByteSize = TCharCountToUTF8ByteSize(sentEndBuf, z.Spans[j].OffsetEnd);

            zone.SetType(id);
            zone.SetOffset(sentBeg.GetOffset() + prefixByteSize);
            zone.SetSize(sentEnd.GetOffset() + suffixByteSize - zone.GetOffset());
            zone.SetFirstSentence(sentBegIndex);
        }
    }
}

void SaveDocExtInfoToProto(NTextArc::TDocArcData& arcData, const TBlob& infoBlob) {
    TDocDescr docDescr;
    docDescr.UseBlob(infoBlob.Data(), infoBlob.Size());

    if (!docDescr.IsAvailable()) {
        return;
    }

    ui32 hostId = docDescr.get_hostid();
    if (hostId != 0) {
        arcData.SetHostId(hostId);
    }

    ui32 urlId = docDescr.get_urlid();
    if (urlId != 0) {
        arcData.SetUrlId(urlId);
    }

    time_t time = docDescr.get_mtime();
    if (time > 0) {
        arcData.SetTime(time);
    }

    const char *hostName = docDescr.get_host();
    if (hostName && hostName[0] != '\0') {
        arcData.SetHostName(hostName);
    }

    const char *urlName = docDescr.get_url();
    if (urlName && urlName[0] != '\0') {
        arcData.SetUrlName(urlName);
    }

    const char* mimeType  = strByMime(docDescr.get_mimetype());
    if (mimeType) {
        arcData.SetMimeType(mimeType);
    }

    ECharset charset = docDescr.get_encoding();
    if (CODES_UNKNOWN < charset && charset < CODES_MAX) {
        arcData.SetCharset(charset);
    }

    TDocInfos docInfo;
    docDescr.ConfigureDocInfos(docInfo);

    for (const auto it : docInfo) {
        NTextArc::TDocInfo* info = arcData.AddDocInfos();
        info->SetDocInfoName(it.first);
        info->SetDocInfoValue(it.second);
    }

    if (docInfo.contains("documentid")) {
        arcData.SetGlobalDocId(docInfo["documentid"]);
    }

    if (docInfo.contains("lang")) {
        return arcData.SetLanguage(LanguageByName(docInfo["lang"]));
    }
}

TUtf16String ExtractDocTitleFromArc(const TBlob& docText) {
    // code extracted from tarcview
    TArchiveMarkupZones mZones;
    GetArchiveMarkupZones((const ui8*)docText.Data(), &mZones);
    TArchiveZone& zone = mZones.GetZone(AZ_TITLE);
    TUtf16String title;
    if (!zone.Spans.empty()) {
        TVector<int> sentNumbers;
        for (TVector<TArchiveZoneSpan>::const_iterator i = zone.Spans.begin(); i != zone.Spans.end(); ++i) {
            for (ui16 n = i->SentBeg; n <= i->SentEnd; ++n) {
                sentNumbers.push_back(n);
            }
        }
        TVector<TArchiveSent> outSents;
        GetSentencesByNumbers((const ui8*)docText.Data(), sentNumbers, &outSents, nullptr, false);

        TSentReader sentReader;
        for (TVector<TArchiveSent>::iterator i = outSents.begin(); i != outSents.end(); ++i) {
            if (title.length()) {
                title.append(' ');
            }
            title.append(sentReader.GetText(*i, mZones.GetSegVersion()));
        }
    }
    return title;
}

void SaveDocTitleToProto(NTextArc::TDocArcData& arcData, const TBlob& docBlob) {
    if (docBlob.Empty())
        return;
    TUtf16String title = ExtractDocTitleFromArc(docBlob);
    arcData.SetTitle(WideToUTF8(title));
}

THashSet<TUtf16String> ExtractDocAnchorTextsFromArc(const TBlob& docText, ui32 docId, const TXRefMapped2DArray& xref) {
    TVector<int> numbers;
    TVector<TArchiveSent> sentences;
    GetSentencesByNumbers((const ui8*)docText.Data(), numbers, &sentences, nullptr, false);

    THashSet<TUtf16String> anchorTexts;
    for (size_t i = 0; i < sentences.size(); ++i) {
        if (xref.GetAt(docId, i).EntryType == TXMapInfo::ET_LINK) {
            anchorTexts.insert(sentences[i].Text);
        }
    }

    return anchorTexts;
}

void SaveDocAnchorTextsToProto(NTextArc::TDocArcData& arcData, const TBlob& docBlob, ui32 docId, const TXRefMapped2DArray& xref) {
    THashSet<TUtf16String> anchorTexts = ExtractDocAnchorTextsFromArc(docBlob, docId, xref);
    for (const auto& anchorText : anchorTexts) {
        arcData.AddAnchorTexts(WideToUTF8(anchorText));
    }
}
