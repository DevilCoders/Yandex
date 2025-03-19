#include "forums_handler.h"

#include <kernel/snippets/archive/unpacker/unpacker.h>
#include <kernel/snippets/archive/zone_checker/zone_checker.h>

#include <library/cpp/tokenizer/tokenizer.h>

#include <util/charset/wide.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/string/cast.h>

namespace NSnippets
{

static bool SafeGetWideString(const TDocInfos& infos, const char* attrName, TUtf16String& result)
{
    TDocInfos::const_iterator ii = infos.find(attrName);
    if (ii == infos.end()) {
        return false;
    }
    size_t len = strlen(ii->second);
    result.resize(len, 0);
    size_t written;
    size_t pos = UTF8ToWideImpl<false>(ii->second, len, result.begin(), written);
    if (pos != len) {
        result.clear();
        return false;
    }
    result.remove(written);
    return true;
}

static bool SafeGetInt(const TDocInfos& infos, const char* attrName, int& result)
{
    TDocInfos::const_iterator ii = infos.find(attrName);
    if (ii == infos.end()) {
        return false;
    }

    return TryFromString<int>(ii->second, result);
}

TForumMarkupViewer::TForumMarkupViewer(bool enabled, const TDocInfos& infos, bool filterByFirstPosition)
    : PageKind(NONE)
    , HasValidMarkup(false)
    , HasValidAttrs(false)
    , MixedPageKind(false)
    , PageNum(0)
    , TotalNumMessages(0)
    , NumItems(0)
    , NumItemsPrecise(false)
    , NumPages(0)
    , NumItemsOnPage(0)
    , FilterByPosition(filterByFirstPosition)
    , Unpacker(nullptr)
{
    if (!enabled) {
        return;
    }

    int forumParserVer;
    if (!SafeGetInt(infos, "forum_parser_ver", forumParserVer)) {
        return;
    }
    if (forumParserVer < 1 || forumParserVer > 3) {
        return;
    }

    SafeGetInt(infos, "forum_page", PageNum);
    SafeGetInt(infos, "forum_total_messages", TotalNumMessages);
    SafeGetInt(infos, "forum_total_pages", NumPages);
    SafeGetWideString(infos, "forum_title", ForumTitle);
    HasValidAttrs = true;
}


TUtf16String ExtractForumAttr(const THashMap<TString, TUtf16String>* attributes, const char* attrName)
{
    TUtf16String result;
    if (!attrName) {
        return result;
    }
    THashMap<TString, TUtf16String>::const_iterator attr;
    attr = attributes->find(attrName);
    if (attr != attributes->end()) {
        result = attr->second;
    }
    size_t len = result.size();
    TCharTemp tmpBuf(len);
    wchar16* ptr = tmpBuf.Data();
    for (size_t i = 0; i < len; ++i) {
        wchar16 ch = result[i];
        *ptr = IsWhitespace(ch) ? ' ' : ch;
        ++ptr;
    }
    result.assign(tmpBuf.Data(), len);
    Collapse(result);
    Strip(result);

    return result;
}

void TForumMarkupViewer::ZonesToForumItems(const TArchiveMarkupZones& zones)
{
    const char* dateAttr = nullptr;
    const char* popularityAttr = nullptr;
    const char* uriAttr = nullptr;
    const char* authorAttr = nullptr;
    const char* contentAttr = "forum_name";
    const char* longContentAttr = "forum_descr";
    EArchiveZone zone;
    bool hasMessages = !zones.GetZone(AZ_FORUM_MESSAGE).Spans.empty();
    bool hasThreads = !zones.GetZone(AZ_FORUM_TOPIC_INFO).Spans.empty();
    bool hasForums = !zones.GetZone(AZ_FORUM_INFO).Spans.empty();
    bool needDate = false;

    if (hasMessages) {
        zone = AZ_FORUM_MESSAGE;
        PageKind = MESSAGES;
        dateAttr = "forum_date";
        uriAttr = "forum_anchor";
        authorAttr = "forum_author";
        needDate = true;
    }
    else if (hasThreads) {
        zone = AZ_FORUM_TOPIC_INFO;
        PageKind = THREADS;
        dateAttr = "forum_last_date";
        uriAttr = "forum_link";
        popularityAttr = "forum_messages";
    }
    else if (hasForums) {
        PageKind = FORUMS;
        zone = AZ_FORUM_INFO;
        dateAttr = "forum_last_date";
        uriAttr = "forum_link";
        popularityAttr = "forum_topics";
    }
    else {
        return;
    }

    MixedPageKind = ((int)hasMessages + (int)hasThreads + (int)hasForums > 1);

    if (MixedPageKind) {
        return;
    }

    MessageSpans = zones.GetZone(zone).Spans;
    SortZones(MessageSpans);
    NumItemsOnPage = MessageSpans.ysize();

    const TArchiveZoneAttrs& forumAttrs = zones.GetZoneAttrs(zone);
    for (TVector<TArchiveZoneSpan>::const_iterator ii = MessageSpans.begin(), end = MessageSpans.end(); ii != end; ++ii)
    {
        const TArchiveZoneSpan& span = *ii;
        if (span.Empty()) {
            continue;
        }

        const THashMap<TString, TUtf16String>* attributes = forumAttrs.GetSpanAttrs(span).AttrsHash;
        if (!attributes) {
            continue;
        }

        TForumMessageZone message(*ii);
        message.Anchor = ExtractForumAttr(attributes, uriAttr);
        message.Date = ExtractForumAttr(attributes, dateAttr);
        message.Content = ExtractForumAttr(attributes, contentAttr);
        message.LongContent = ExtractForumAttr(attributes, longContentAttr);
        message.Author = ExtractForumAttr(attributes, authorAttr);
        message.Position = ForumZones.size();
        TUtf16String popularityStr = ExtractForumAttr(attributes, popularityAttr);
        size_t popularity = 0;
        if (TryFromString<size_t>(popularityStr, popularity)) {
            message.Popularity = popularity;
            message.HasPopularity = true;
        }

        if (needDate && !message.Date) {
            continue;
        }

        ForumZones.push_back(message);
   }
}

int GuessLastSent(const TArchiveMarkupZones& zones)
{
    int result = 0;
    for (int i = 0; i < (int)AZ_COUNT; ++i) {
        const TVector<TArchiveZoneSpan>& spans = zones.GetZone((EArchiveZone)i).Spans;
        if (spans.empty())
            continue;
        const TArchiveZoneSpan& span = spans.back();
        if (span.SentEnd > result)
            result = span.SentEnd;
    }
    return result;
}

void TForumMarkupViewer::OnMarkup(const TArchiveMarkupZones& zones)
{
    if (!HasValidAttrs) {
        return;
    }

    int maxSent = FilterByPosition ? GuessLastSent(zones) : 0;

    ZonesToForumItems(zones);

    if (ForumZones.empty()) {
        return;
    }

    if (PageKind != MESSAGES && maxSent > 0) {
        TForumMessageZone& zone = ForumZones.front();
        if (zone.Span.SentBeg > maxSent * 0.4) {
            // too much content before the first zone
            return;
        }
    }

    if (PageKind == MESSAGES && TotalNumMessages > 0) {
        NumItemsPrecise = true;
        NumItems = TotalNumMessages;
    }
    else if (!MixedPageKind) {
        if (PageNum <= 1 && NumPages <= 1) {
            NumItemsPrecise = true;
            NumItems = NumItemsOnPage;
        }
        else if (NumPages > PageNum && PageNum > 0) {
            NumItems = NumItemsOnPage * (NumPages - 1);
        }
    }

    TForwardInZoneChecker msgCheck(MessageSpans, false);
    ForumMessageSents.resize(MessageSpans.back().SentEnd + 1, false);
    for (int i = 0; i < ForumMessageSents.ysize(); ++i) {
        ForumMessageSents[i] = msgCheck.SeekToSent(i);
    }

    for (TZones::iterator ii = ForumZones.begin(), end = ForumZones.end(); ii != end; ++ii) {
        All.PushBack((*ii).Span.SentBeg, (*ii).Span.SentEnd);
    }

    Unpacker->AddRequest(All);
    HasValidMarkup = true;
}

struct TForumTokenCallback: public ITokenHandler
{
    static const size_t MAX_WORDS = 5;
    TVector<size_t> Offsets;
    size_t CurrOffset;

    TForumTokenCallback()
        : CurrOffset(0)
    {
        Offsets.reserve(MAX_WORDS);
    }

    void OnToken(const TWideToken& /*token*/, size_t origleng, NLP_TYPE type) override
    {
        if (type == NLP_WORD && Offsets.size() < MAX_WORDS) {
            Offsets.push_back(CurrOffset);
        }
        CurrOffset += origleng;
    }
};

bool HasSuffixHash(const TWtringBuf& text, const THashSet<size_t>& sentHashes)
{
    TForumTokenCallback callback;
    TNlpTokenizer tokenizer(callback, false);
    tokenizer.Tokenize(text.data(), text.length());
    for (int i = 0; i < callback.Offsets.ysize(); ++i) {
        TWtringBuf wb = text;
        wb.Skip(callback.Offsets[i]);
        if (sentHashes.contains(ComputeHash(wb))) {
            return true;
        }
    }
    return false;
}

void TForumMarkupViewer::FillSentExcludeFilter(TSentFilter& filter, const TArchiveView& sents) const {
    static const size_t MIN_DUPE_LEN = 20;

    if (!IsValid() || sents.Empty())
        return;

    THashSet<size_t> sentHashes;

    for (size_t i = 0; i < sents.Size(); ++i) {
        const TArchiveSent* s = sents.Get(i);
        int id = s->SentId;
        const TWtringBuf& text = s->Sent;
        if (id < 0 || id >= ForumMessageSents.ysize() || !ForumMessageSents[id]) {
            filter.Mark(id);
            continue;
        }
        if (text.length() < MIN_DUPE_LEN)
            continue;

        size_t hash = ComputeHash(text);
        if (sentHashes.contains(hash) || HasSuffixHash(text, sentHashes)) {
            filter.Mark(id);
        }
        else {
            sentHashes.insert(hash);
        }
    }
}

}
