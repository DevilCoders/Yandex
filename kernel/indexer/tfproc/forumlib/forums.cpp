#include "forums.h"
#include "forums_fsm.h"
#include "tags.h"
#include "breadcrumbs.h"
#include "tables.h"
#include "dates.h"
#include <util/generic/utility.h>
#include <util/generic/algorithm.h>
#include <library/cpp/charset/wide.h>
#include <util/datetime/systime.h>
#include <util/string/cast.h>
#include <library/cpp/string_utils/scan/scan.h>
#include <library/cpp/uri/uri.h>
#include <kernel/indexer/face/inserter.h>
#include <kernel/tarc/iface/tarcface.h>

namespace NForumsImpl {

struct TQuoteDescriptor
{
    TPosting Begin, BodyBegin, End;
    bool OnlyHeader;
    TVector<TQuoteDescriptor> Nested;
    TRecognizedDate Date;
    TWtringBuf Time;
    TWtringBuf Url;
    TWtringBuf Author;
    TQuoteDescriptor(TPosting b)
        : Begin(b)
        , BodyBegin(b)
        , End(b)
        , OnlyHeader(true)
        , Date(0, 0, 0)
    {
    }
};

struct TPostDescriptor
{
    TPosting MessageBegin;
    TPosting MessageEnd;
    TPosting SignatureBegin;
    TPosting SignatureEnd;
    TRecognizedDate Date;
    TWtringBuf Author;
    TWtringBuf Anchor;
    typedef TVector<TQuoteDescriptor> TQuotesContainer;
    TQuotesContainer Quotes;
    TPostDescriptor()
        : MessageBegin(0)
        , MessageEnd(0)
        , SignatureBegin(0)
        , SignatureEnd(0)
        , Date(0, 0, 0)
    {}
};

class TDates
{
private:
    bool IsDateAmericanHint;
    bool InDate;
    TRecognizedDate FirstPostDate, LastPostDate;
    TForumDateRecognizer Recognizer;
    bool DetectAmericanDate(const TVector<TPostDescriptor>& posts) const;
public:
    void Clear()
    {
        IsDateAmericanHint = false;
        InDate = false;
        TRecognizedDate::FromInt64(0, &FirstPostDate);
        LastPostDate = FirstPostDate;
        Recognizer.Clear();
    }
    void SetDate(time_t date, bool isDateAmericanHint)
    {
        Recognizer.SetDate(date);
        IsDateAmericanHint = isDateAmericanHint;
    }
    void OnDateText(const wchar16* text, unsigned len)
    {
        Recognizer.OnDateText(text, len);
    }
    const TRecognizedDate& GetFirstPostDate() const
    {
        return FirstPostDate;
    }
    const TRecognizedDate& GetLastPostDate() const
    {
        return LastPostDate;
    }
    static void OnDateTransition(void* param, bool entered, const TNumerStat&, TPostDescriptor*& lastPost);
    bool NeedText() const
    {
        return InDate;
    }
    void PostProcessDates(TVector<TPostDescriptor>& posts);
    void DivideDates()
    {
        static const wchar16 divider = wchar16('|');
        OnDateText(&divider, 1);
    }
};

void TDates::OnDateTransition(void* param, bool entered, const TNumerStat&, TPostDescriptor*& lastPost)
{
    TDates* this_ = (TDates*)param;
    if (entered) {
        this_->InDate = true;
        this_->Recognizer.Clear();
    } else {
        this_->InDate = false;
        TRecognizedDate date;
        if (this_->Recognizer.GetDate(&date)) {
            lastPost->Date = date;
        }
    }
}

bool TDates::DetectAmericanDate(const TVector<TPostDescriptor>& posts) const
{
    if (posts.empty())
        return IsDateAmericanHint; // no data
    bool canBeAmerican = true, canBeEuropean = true;
    // test 1: month can not be greater than 12
    for (TVector<TPostDescriptor>::const_iterator it = posts.begin(); it != posts.end(); ++it) {
        if (it->Date.Priority)
            continue;
        if (it->Date.Month > 12)
            canBeEuropean = false;
        if (it->Date.Day > 12)
            canBeAmerican = false;
    }
    if (!canBeAmerican && !canBeEuropean)
        return IsDateAmericanHint; // strange... let's hope for the best
    if (!canBeAmerican)
        return false;
    if (!canBeEuropean)
        return true;
    // test 2: dates are usually increasing
    TRecognizedDate prev(0, 0, 0);
    bool prevFound = false;
    for (TVector<TPostDescriptor>::const_iterator it = posts.begin(); it != posts.end(); ++it) {
        if (it->Date.Priority)
            continue;
        if (!prevFound) {
            prev = it->Date;
            prevFound = true;
            continue;
        }
        TRecognizedDate next = it->Date;
        if (prev.ToInt64() > next.ToInt64())
            canBeEuropean = false;
        DoSwap(prev.Day, prev.Month);
        DoSwap(next.Day, next.Month);
        if (prev.ToInt64() > next.ToInt64())
            canBeAmerican = false;
        DoSwap(next.Day, next.Month);
        prev = next;
    }
    if (!canBeAmerican && canBeEuropean)
        return false;
    if (canBeAmerican && !canBeEuropean)
        return true;
    return IsDateAmericanHint;
}

void TDates::PostProcessDates(TVector<TPostDescriptor>& posts)
{
    if (DetectAmericanDate(posts)) {
        for (TVector<TPostDescriptor>::iterator it = posts.begin(); it != posts.end(); ++it)
            if (!it->Date.Priority)
                DoSwap(it->Date.Month, it->Date.Day);
    }
    if (!posts.empty()) {
        // we can't just use Posts.front().Date and Posts.back().Date,
        // since posts are sometimes edited, this usually changes the date
        for (TVector<TPostDescriptor>::const_iterator it = posts.begin(); it != posts.end(); ++it) {
            ui64 d = it->Date.ToInt64();
            if (!d)
                continue;
            ui64 firstPostDate = FirstPostDate.ToInt64();
            if (!firstPostDate || firstPostDate > d)
                FirstPostDate = it->Date;
            ui64 lastPostDate = LastPostDate.ToInt64();
            if (!lastPostDate || lastPostDate < d)
                LastPostDate = it->Date;
        }
    }
}

class TAuthors
{
private:
    THashSet<TWtringBuf> AllAuthors;
    TUtf16String& CurText;
    segmented_pool<wchar16>& StringsPool;
    int NumDifferentAuthors;
    bool InAuthor;
public:
    TAuthors(TUtf16String& curText, segmented_pool<wchar16>& stringsPool)
        : CurText(curText)
        , StringsPool(stringsPool)
    {
    }
    int GetNumDifferentAuthors() const
    {
        return AllAuthors.size();
    }
    static void OnAuthorTransition(void* param, bool entered, const TNumerStat&, TPostDescriptor*& lastPost);
    bool NeedText() const
    {
        return InAuthor;
    }
    void Clear()
    {
        NumDifferentAuthors = 0;
        AllAuthors.clear();
        InAuthor = false;
    }
};

void TAuthors::OnAuthorTransition(void* param, bool entered, const TNumerStat&, TPostDescriptor*& lastPost)
{
    TAuthors* this_ = (TAuthors*)param;
    if (entered) {
        this_->InAuthor = true;
        this_->CurText.clear();
    } else {
        this_->InAuthor = false;
        if (this_->CurText.empty() || !lastPost->Author.empty())
            return;
        TWtringBuf tmpAuthor(this_->CurText.data(), this_->CurText.size());
        TWtringBuf author;
        THashSet<TWtringBuf>::const_iterator it = this_->AllAuthors.find(tmpAuthor);
        if (it == this_->AllAuthors.end()) {
            author = TWtringBuf(this_->StringsPool.append(tmpAuthor.data(), tmpAuthor.size()), tmpAuthor.size());
            this_->AllAuthors.insert(author);
        } else
            author = TWtringBuf(it->data(), it->size());
        lastPost->Author = author;
        this_->CurText.clear();
    }
}

TMessages::TMessages(segmented_pool<wchar16>& stringsPool, TTagsProcessor* tagsProcessor)
    : ActiveQuote(nullptr)
    , StringsPool(stringsPool)
    , TagsProcessor(tagsProcessor)
{
}

TMessages::~TMessages()
{
}

void TMessages::Clear()
{
    ActiveQuote = nullptr;
    ActiveQuoteStack.clear();
    QuoteParseStack.clear();
}

void TMessages::EnterQuote(ui32 pos, TPostDescriptor* lastPost)
{
    if (ActiveQuote) {
        THandlerInfoState state;
        QuoteParseStack.emplace_back();
        TagsProcessor->SaveHandlerState(HANDLER_QUOTE_HEADER, &QuoteParseStack.back());
        QuoteParseStack.emplace_back();
        TagsProcessor->SaveHandlerState(HANDLER_QUOTE_BODY, &QuoteParseStack.back());
        TagsProcessor->SaveHandlerState(HANDLER_NESTED_QUOTE_HEADER, &state);
        TagsProcessor->RestoreHandlerState(HANDLER_QUOTE_HEADER, &state, false);
        TagsProcessor->SaveHandlerState(HANDLER_NESTED_QUOTE_BODY, &state);
        TagsProcessor->RestoreHandlerState(HANDLER_QUOTE_BODY, &state, false);
    }
    ActiveQuoteStack.push_back(ActiveQuote);
    TVector<TQuoteDescriptor>& curList = (ActiveQuote ? ActiveQuote->Nested : lastPost->Quotes);
    curList.push_back(pos);
    ActiveQuote = &curList.back();
    ParsingQuoteAuthor = false;
    HeaderOk = false;
}

void TMessages::LeaveQuote(ui32 pos)
{
    ActiveQuote->End = pos;
    ActiveQuote = ActiveQuoteStack.back();
    ActiveQuoteStack.pop_back();
    if (ActiveQuote) {
        THandlerInfoState state;
        TagsProcessor->SaveHandlerState(HANDLER_QUOTE_BODY, &state);
        TagsProcessor->RestoreHandlerState(HANDLER_NESTED_QUOTE_BODY, &state, false);
        TagsProcessor->SaveHandlerState(HANDLER_QUOTE_HEADER, &state);
        TagsProcessor->RestoreHandlerState(HANDLER_NESTED_QUOTE_HEADER, &state, false);
        TagsProcessor->RestoreHandlerState(HANDLER_QUOTE_BODY, &QuoteParseStack.back(), false);
        QuoteParseStack.pop_back();
        TagsProcessor->RestoreHandlerState(HANDLER_QUOTE_HEADER, &QuoteParseStack.back(), false);
        QuoteParseStack.pop_back();
    } else {
        TagsProcessor->Stop(HANDLER_NESTED_QUOTE_HEADER);
        TagsProcessor->Stop(HANDLER_NESTED_QUOTE_BODY);
    }
}

bool TMessages::ProcessHeader()
{
    if (CurText.empty())
        return false;
    wchar16* start = CurText.begin();
    wchar16* end = start + CurText.size();
    InitFSM();
    const wchar16* p = RunFSM(start, end);
    if (p)
        start = const_cast<wchar16*>(p);
    InitFSM();
    while (start < end && IsSpace(*start))
        ++start;
    while (start < end && IsSpace(end[-1]))
        --end;
    while (start < end && start[0] == '(' && end[-1] == ')')
        ++start, --end;
    if (start >= end)
        return false;
    std::reverse(start, end);
    TWtringBuf meta;
    bool hasMeta = HeaderFSM(start, end, meta);
    std::reverse(start, end);
    if (hasMeta) {
        const wchar16* metaEnd = meta.data() + meta.size();
        meta = TWtringBuf(start + (end - 1 - metaEnd), start + (end - 1 - meta.data()));
        RecognizeQuoteDate(meta);
        const wchar16* metaStart = meta.data();
        while (metaStart > start && IsSpace(metaStart[-1]))
            --metaStart;
        if (metaStart > start) {
            ActiveQuote->Author = TWtringBuf(StringsPool.append(start, metaStart - start), metaStart - start);
            HeaderOk = true;
        }
    } else {
        ActiveQuote->Author = TWtringBuf(StringsPool.append(start, end - start), end - start);
        return false;
    }
    return true;
}

void TMessages::RecognizeQuoteDate(TWtringBuf str)
{
    // date
    TStreamDateRecognizer recognizer(true);
    TString yStr = WideToChar(str, CODES_YANDEX);
    recognizer.Push(yStr.data(), yStr.size());
    recognizer.GetDate(&ActiveQuote->Date, true);
    // time
    const wchar16* start = str.data();
    const wchar16* end = str.data() + str.size();
    while (end > start && (IsSpace(end[-1]) || end[-1] == ')' || end[-1] == ']' || end[-1] == ':'))
        --end;
    enum { ENothing, EAM, EPM } timeModifier = ENothing;
    if (end > start + 2 && (end[-1] | 0x20) == 'm' && (end[-2] | 0x20) == 'a') {
        timeModifier = EAM;
        end -= 2;
        while (end > start && IsSpace(end[-1]))
            --end;
    } else if (end > start + 2 && (end[-1] | 0x20) == 'm' && (end[-2] | 0x20) == 'p') {
        timeModifier = EPM;
        end -= 2;
        while (end > start && IsSpace(end[-1]))
            --end;
    }
    if (end < start + 5)
        return;
    const wchar16* p;
    if (IsCommonDigit(end[-1]) && IsCommonDigit(end[-2]) && end[-3] == ':' &&
        IsCommonDigit(end[-4]) && IsCommonDigit(end[-5]))
    {
        p = end - 5;
        if (end >= start + 8 && end[-6] == ':' && IsCommonDigit(end[-7]) && IsCommonDigit(end[-8]))
            p -= 3;
        if (timeModifier == ENothing ||
            timeModifier == EAM && !(p[0] == '1' && p[1] == '2') ||
            timeModifier == EPM && (p[0] == '1' && p[1] == '2'))
        {
            ActiveQuote->Time = TWtringBuf(StringsPool.append(p, end - p), end - p);
        } else {
            int hours = (p[0] - '0') * 10 + (p[1] - '0');
            if (timeModifier == EAM)
                hours = 0;  // 12:01 AM = 00:01
            else
                hours += 12;
            hours %= 100;   // just to be sure
            size_t len = end - p;
            wchar16* q = StringsPool.append(p, len);
            q[0] = (wchar16)(hours / 10 + '0');
            q[1] = (wchar16)(hours % 10 + '0');
            ActiveQuote->Time = TWtringBuf(q, len);
        }
    }
}

void TMessages::OnQuoteHeaderTransition(void* param, bool entered, const TNumerStat& pos, TPostDescriptor*& lastPost)
{
    // note: quote header can be nested in quote "body"
    TMessages* this_ = (TMessages*)param;
    if (entered) {
        if (!this_->TagsProcessor->GetMarkup()->QuoteHeaderInsideBody || !this_->ActiveQuote) {
            this_->EnterQuote(pos.TokenPos.DocLength(), lastPost);
            this_->TagsProcessor->Stop(HANDLER_NESTED_QUOTE_HEADER);
            this_->TagsProcessor->Stop(HANDLER_NESTED_QUOTE_BODY);
            this_->ParsingQuoteMeta = false;
        }
        this_->ParsingQuoteHeader = true;
        this_->CurText.clear();
        this_->ExpectingUrl = true;
    } else {
        if (this_->ParsingQuoteHeader) {
            this_->ProcessHeader();
            this_->ActiveQuote->BodyBegin = pos.TokenPos.DocLength();
            this_->ParsingQuoteHeader = false;
            this_->ExpectingUrl = false;
        }
    }
}

void TMessages::OnQuoteBodyTransition(void* param, bool entered, const TNumerStat& pos, TPostDescriptor*& lastPost)
{
    TMessages* this_ = (TMessages*)param;
    if (entered) {
        const TMarkup* m = this_->TagsProcessor->GetMarkup();
        if (!m->QuoteHeaderTag.NumTags ||
            m->QuoteHeaderInsideBody ||
            !this_->ActiveQuote || !this_->ActiveQuote->OnlyHeader)
        {
            this_->EnterQuote(pos.TokenPos.DocLength(), lastPost);
            this_->TagsProcessor->Restart(HANDLER_NESTED_QUOTE_HEADER, false);
            this_->TagsProcessor->Restart(HANDLER_NESTED_QUOTE_BODY, false);
            this_->ParsingQuoteHeader = false;
        }
        if (this_->ParsingQuoteHeader)
            OnQuoteHeaderTransition(param, false, pos, lastPost);
        this_->ActiveQuote->OnlyHeader = false;
        this_->ActiveQuote->BodyBegin = pos.TokenPos.DocLength();
        this_->ParsingQuoteMeta = !this_->HeaderOk;
        this_->ExpectingUrl = false;
        this_->InitFSM();
        this_->CurText.clear();
    } else {
        if (this_->ParsingQuoteHeader)
            OnQuoteHeaderTransition(param, false, pos, lastPost);
        this_->LeaveQuote(pos.TokenPos.DocLength());
        this_->ParsingQuoteMeta = false;
        this_->ParsingQuoteAuthor = false;
        this_->ParsingQuoteHeader = false;
        this_->ExpectingUrl = false;
        this_->HeaderOk = true;
    }
}

void TMessages::TerminateQuote(const TNumerStat& pos)
{
    while (ActiveQuote)
        LeaveQuote(pos.TokenPos.DocLength());
    TagsProcessor->Stop(HANDLER_QUOTE_HEADER);
    TagsProcessor->Stop(HANDLER_QUOTE_BODY);
}

void TMessages::ProcessText(const wchar16* token, unsigned len, const TNumerStat& pos)
{
    if (!ActiveQuote)
        return;
    bool normal = TagsProcessor->IsInZone(HANDLER_MESSAGE) || TagsProcessor->IsInZone(HANDLER_MESSAGE_ALT);
    if (ParsingQuoteHeader || ParsingQuoteAuthor) {
        if (!normal && ParsingQuoteHeader && TWordPosition::Break(ActiveQuote->BodyBegin) != pos.TokenPos.Break())
            ParsingQuoteHeader = false;
        else
            CurText += TWtringBuf(token, len);
    } else if (ParsingQuoteMeta) {
        if (!normal)
            CurText += TWtringBuf(token, len);
        RunFSM(token, token + len);
        if (FSMDecidedStop()) {
            if (!normal) {
                ParsingQuoteHeader = true;
            }
            ParsingQuoteMeta = false;
            ActiveQuote->BodyBegin = pos.TokenPos.DocLength();
            return;
        }
        if (FSMDecidedAuthor()) {
            ParsingQuoteMeta = false;
            ParsingQuoteAuthor = true;
            CurText.clear();
        }
    }
}

void TMessages::OnOpenTag(const THtmlChunk* chunk, const TNumerStat& /*numerStat*/)
{
    if (!ActiveQuote)
        return;
    if (ParsingQuoteMeta && *chunk->Tag == HT_STRONG) {
        ParsingQuoteMeta = false;
        ParsingQuoteAuthor = true;
        CurText.clear();
    }
    if (ExpectingUrl) {
        if (HrefTag.Match(chunk)) {
            for (size_t i = 0; i < chunk->AttrCount; i++) {
                if (chunk->Attrs[i].Name.Leng == 4 && (*(ui32*)(chunk->text + chunk->Attrs[i].Name.Start) | 0x20202020) == *(ui32*)"href") {
                    const char* text = chunk->text + chunk->Attrs[i].Value.Start;
                    ui32 codedLen = chunk->Attrs[i].Value.Leng;
                    if (!codedLen)
                        continue;
                    TTempArray<wchar16> buf(codedLen * 2);
                    unsigned decodedLen = HtEntDecodeToChar(PropSet->GetCharset(), text, codedLen, buf.Data());
                    if (!decodedLen)
                        continue;
                    ActiveQuote->Url = TWtringBuf(StringsPool.append(buf.Data(), decodedLen), decodedLen);
                    break;
                }
            }
        }
        if (!ParsingQuoteHeader)
            ExpectingUrl = false;
    }
}

void TMessages::OnCloseTag(const THtmlChunk* /*chunk*/, const TNumerStat& numerStat)
{
    if (!ActiveQuote)
        return;
    if (ParsingQuoteAuthor) {
        ParsingQuoteAuthor = false;
        ExpectingUrl = true;
        ActiveQuote->Author = TWtringBuf(StringsPool.append(CurText.data(), CurText.size()), CurText.size());
        ActiveQuote->BodyBegin = numerStat.TokenPos.DocLength();
    }
    if (ParsingQuoteHeader && !TagsProcessor->IsInZone(HANDLER_MESSAGE) && !TagsProcessor->IsInZone(HANDLER_MESSAGE_ALT)) {
        if (ProcessHeader())
            ActiveQuote->BodyBegin = numerStat.TokenPos.DocLength();
        else
            ActiveQuote->Author.Clear();
        ParsingQuoteHeader = false;
        ParsingQuoteMeta = false;
    }
}

void TMessages::OnSignatureTransition(void* /*param*/, bool entered, const TNumerStat& pos, TPostDescriptor*& lastPost)
{
    if (entered)
        lastPost->SignatureBegin = pos.TokenPos.DocLength();
    else
        lastPost->SignatureEnd = pos.TokenPos.DocLength();
}

class TTitles
{
private:
    bool InAnyTitle;
    TWtringBuf Title;
    TUtf16String& CurText;
    segmented_pool<wchar16>& StringsPool;
public:
    TTitles(TUtf16String& curText, segmented_pool<wchar16>& stringsPool)
        : CurText(curText)
        , StringsPool(stringsPool)
    {
    }
    static void OnTitleTransition(void* param, bool entered, const TNumerStat&, TPostDescriptor*&);
    bool NeedText() const
    {
        return InAnyTitle;
    }
    void ProcessTextAfterBreadcrumb(const wchar16* ptr, const wchar16* end);
    void Clear()
    {
        InAnyTitle = false;
        Title.Clear();
    }
    const TWtringBuf& GetTitle() const
    {
        return Title;
    }
    void SetTitle(const TWtringBuf& title)
    {
        Title = title;
    }
};

void TTitles::OnTitleTransition(void* param, bool entered, const TNumerStat&, TPostDescriptor*&)
{
    TTitles* this_ = (TTitles*)param;
    if (entered) {
        if (!this_->Title.empty())
            return;
        this_->CurText.clear();
        this_->InAnyTitle = true;
    } else if (this_->Title.empty() && !this_->CurText.empty()) {
        this_->Title = TWtringBuf(this_->StringsPool.append(this_->CurText.data(), this_->CurText.size()), this_->CurText.size());
        this_->InAnyTitle = false;
        this_->CurText.clear();
    }
}

void TTitles::ProcessTextAfterBreadcrumb(const wchar16* ptr, const wchar16* end)
{
    if (!Title.empty())
        return;
    while (ptr != end && IsSpace(*ptr))
        ptr++;
    if (ptr != end)
        Title = TWtringBuf(StringsPool.append(ptr, end - ptr), end - ptr);
}

class TAnchors
{
private:
    TWtringBuf LastAnchor;
    int LastAnchorPriority;
    segmented_pool<wchar16>& StringsPool;
public:
    TAnchors(segmented_pool<wchar16>& stringsPool)
        : StringsPool(stringsPool)
    {
    }
    void Clear() {
        LastAnchor.Clear();
        LastAnchorPriority = 0;
    }
    const TWtringBuf& GetLast() const {
        return LastAnchor;
    }
    void CheckAnchorTag(const IParsedDocProperties*, const THtmlChunk* chunk, const TMarkup* markup);
};

void TAnchors::CheckAnchorTag(const IParsedDocProperties* propSet, const THtmlChunk* chunk, const TMarkup* markup)
{
    int tagPriority = (*chunk->Tag == HT_A) ? 3 : ((*chunk->Tag == HT_DIV) ? 2 : 1);
    for (size_t j = 0; j < chunk->AttrCount; j++)
        if ((chunk->Attrs[j].Name.Leng == 4 &&
            (*(ui32*)(chunk->text + chunk->Attrs[j].Name.Start) | 0x20202020) == *(ui32*)"name") ||
            (chunk->Attrs[j].Name.Leng == 2 &&
            (*(ui16*)(chunk->text + chunk->Attrs[j].Name.Start) | 0x2020) == *(ui16*)"id"))
        {
            const char* text = chunk->text + chunk->Attrs[j].Value.Start;
            ui32 len = chunk->Attrs[j].Value.Leng;
            if (!len)
                continue;
            if (len >= 3 && text[0] == 't' && text[1] == 'o' && text[2] == 'p')
                continue;
            int priority = tagPriority;
            if (markup) {
                size_t expectedLen = strlen(markup->AnchorPrefix);
                if (len > expectedLen && !memcmp(text, markup->AnchorPrefix, expectedLen)) {
                    bool foundNotDigit = false;
                    for (const char* p = text + expectedLen; p < text + len; p++)
                        if (*p != '_' && (*p < '0' || *p > '9'))
                            foundNotDigit = true;
                    if (!foundNotDigit)
                        priority += 100;
                }
            }
            TTempArray<wchar16> unicodeTempBuf(len * 2 + 1);
            wchar16* ptr = unicodeTempBuf.Data();
            ptr[0] = '#';
            size_t bufLen = HtEntDecodeToChar(propSet->GetCharset(), text, len, ptr + 1);
            if (!bufLen)
                continue;
            if (LastAnchorPriority >= priority)
                return;
            LastAnchor = TWtringBuf(StringsPool.append(ptr, bufLen + 1), bufLen + 1);
            LastAnchorPriority = priority;
            return;
        }
}

class TMainImpl
{
public:
    int NumPosts;
    TForumsHandler::EGenerator Generator;

    TDates Dates;
    TMessages Messages;
    TAuthors Authors;
    TTitles Titles;
    TBreadcrumbs Breadcrumbs;
    TAnchors Anchors;
    TPageNumbers PageNumbers;
    TTablesExtractor Tables;
    TSpecificTablesExtractor Tables2;

    TMainImpl();
    ~TMainImpl();
    void Clear();
    void OnCommitDoc(IDocumentDataInserter* inserter);
    void OnTextStart(const IParsedDocProperties* ps) {
        PropSet = ps;
        Messages.PropSet = ps;
        Tables.SetDocProps(ps);
        Tables2.SetDocProps(ps);
    }
    void OnTextEnd();
    void OnMoveInput(const THtmlChunk& chunk, const TZoneEntry*, const TNumerStat& numerStat);
    void OnTokenStart(const TWideToken& token, const TNumerStat& numerStat);
    void OnSpaces(const wchar16* token, unsigned len, const TNumerStat& numerStat);

private:
    const IParsedDocProperties* PropSet;
    int GlobalDepth, DepthOfStrong, DepthOfLink;

    TUtf16String CurText;
    segmented_pool<wchar16> StringsPool;
    TVector<TPostDescriptor> Posts;

    static void OnPostTransition(void* param, bool entered, const TNumerStat&, TPostDescriptor*& lastPost);
    static void OnMessageTransition(void* param, bool entered, const TNumerStat& pos, TPostDescriptor*& lastPost);
    static void OnMarkupTransition(void* param, bool entered, const TNumerStat&, TPostDescriptor*&);
    void ProcessText(const char* text, unsigned len);

    NForumsImpl::TTagsProcessor TagsProcessor;
};

void TMainImpl::ProcessText(const char* text, unsigned len)
{
    if (Authors.NeedText() && !CurText.empty())
        return;
    TTempArray<wchar16> unicodeTempBuf(len * 2);
    size_t bufLen = HtEntDecodeToChar(PropSet->GetCharset(), text, len, unicodeTempBuf.Data());
    Y_ASSERT(bufLen <= len);
    if (!bufLen)
        return;
    wchar16* ptr = unicodeTempBuf.Data();
    wchar16* end = ptr + bufLen;
    while (ptr != end && IsSpace(*ptr))
        ptr++;
    while (ptr != end && IsSpace(end[-1]))
        end--;
    if (ptr == end)
        return;
    if (Dates.NeedText())
        Dates.OnDateText(ptr, end - ptr);
    if (Breadcrumbs.NeedCheckText()) {
        if (!Breadcrumbs.CheckText(ptr, end - ptr)) {
            if (end > ptr + 1 && IsArrowCharacter(*ptr) && !Breadcrumbs.Empty()) {
                Titles.ProcessTextAfterBreadcrumb(ptr + 1, end);
            }
        }
    }
    if (Authors.NeedText() || Titles.NeedText() || Breadcrumbs.NeedText()) {
        if (!CurText.empty())
            CurText.append(unicodeTempBuf.Data(), ptr - unicodeTempBuf.Data());
        CurText.append(ptr, end - ptr);
    }
}

TMainImpl::TMainImpl()
    : Messages(StringsPool, &TagsProcessor)
    , Authors(CurText, StringsPool)
    , Titles(CurText, StringsPool)
    , Breadcrumbs(CurText, StringsPool)
    , Anchors(StringsPool)
    , Tables(&TagsProcessor)
    , Tables2(&TagsProcessor)
    , StringsPool(65536)
{
    TagsProcessor.Assign(HANDLER_POST, &OnPostTransition, this);
    TagsProcessor.Assign(HANDLER_MESSAGE, &OnMessageTransition, this);
    TagsProcessor.Assign(HANDLER_MESSAGE_ALT, &OnMessageTransition, this);
    for (int i = 0; i < MaxDateChains; i++)
        TagsProcessor.Assign(HANDLER_DATE + i, &TDates::OnDateTransition, &Dates);
    TagsProcessor.Assign(HANDLER_AUTHOR, &TAuthors::OnAuthorTransition, &Authors);
    TagsProcessor.Assign(HANDLER_AUTHOR_ALT, &TAuthors::OnAuthorTransition, &Authors);
    TagsProcessor.Assign(HANDLER_QUOTE_HEADER, &NForumsImpl::TMessages::OnQuoteHeaderTransition, &Messages);
    TagsProcessor.Assign(HANDLER_QUOTE_BODY, &NForumsImpl::TMessages::OnQuoteBodyTransition, &Messages);
    TagsProcessor.Assign(HANDLER_NESTED_QUOTE_HEADER, &NForumsImpl::TMessages::OnQuoteHeaderTransition, &Messages);
    TagsProcessor.Assign(HANDLER_NESTED_QUOTE_BODY, &NForumsImpl::TMessages::OnQuoteBodyTransition, &Messages);
    TagsProcessor.Assign(HANDLER_SIGNATURE, &NForumsImpl::TMessages::OnSignatureTransition, &Messages);
    for (int i = 0; i < NumKnownMarkups; i++)
        TagsProcessor.Assign(HANDLER_MARKUP + i, &OnMarkupTransition, this);
    for (int i = 0; i < NumPossibleBreadcrumbs; i++)
        TagsProcessor.Assign(HANDLER_BREADCRUMB_SPECIFIC + i, &TBreadcrumbs::OnSpecificBreadcrumbTransition, &Breadcrumbs);
    for (int i = 0; i < NumPossibleTitles; i++)
        TagsProcessor.Assign(HANDLER_TITLE + i, &TTitles::OnTitleTransition, &Titles);
    Clear();
    Tables2.RegisterCallbacks();
}

TMainImpl::~TMainImpl()
{
}

void TMainImpl::Clear(void)
{
    TagsProcessor.Init();
    Generator = TForumsHandler::Unknown;
    NumPosts = 0;
    Dates.Clear();
    Messages.Clear();
    Authors.Clear();
    Titles.Clear();
    Breadcrumbs.Clear();
    Anchors.Clear();
    PageNumbers.Clear();
    StringsPool.clear();
    Posts.clear();
    GlobalDepth = 0;
    DepthOfStrong = 0;
    DepthOfLink = 0;
    Tables.Clear();
    Tables2.Clear();
}

void TMainImpl::OnMarkupTransition(void* param, bool entered, const TNumerStat&, TPostDescriptor*&)
{
    if (!entered) {
        Y_ASSERT(0);
        return;
    }
    TMainImpl* this_ = (TMainImpl*)param;
    for (int i = 0; i < NumKnownMarkups; i++)
        this_->TagsProcessor.Stop(HANDLER_MARKUP + i);
    const TMarkup& m = *this_->TagsProcessor.GetMarkup();
    this_->Generator = m.Generator;
    this_->TagsProcessor.SetAssociatedChain(HANDLER_POST, &m.PostEntireTag);
    this_->TagsProcessor.Start(HANDLER_POST);
    this_->TagsProcessor.SetAssociatedChain(HANDLER_MESSAGE, &m.PostMessageTag);
    this_->TagsProcessor.SetAssociatedChain(HANDLER_MESSAGE_ALT, &m.PostMessageTagAlternative);
    for (int i = 0; i < MaxDateChains; i++)
        this_->TagsProcessor.SetAssociatedChain(HANDLER_DATE + i, &m.PostDateTag[i]);
    this_->TagsProcessor.SetAssociatedChain(HANDLER_AUTHOR, &m.PostAuthorTag);
    this_->TagsProcessor.SetAssociatedChain(HANDLER_AUTHOR_ALT, &m.PostAuthorTagAlternative);
    this_->TagsProcessor.SetAssociatedChain(HANDLER_QUOTE_HEADER, &m.QuoteHeaderTag);
    this_->TagsProcessor.SetAssociatedChain(HANDLER_NESTED_QUOTE_HEADER, &m.QuoteHeaderTag);
    this_->TagsProcessor.SetAssociatedChain(HANDLER_QUOTE_BODY, &m.QuoteBodyTag);
    this_->TagsProcessor.SetAssociatedChain(HANDLER_NESTED_QUOTE_BODY, &m.QuoteBodyTag);
    this_->TagsProcessor.SetAssociatedChain(HANDLER_SIGNATURE, &m.SignatureTag);
    this_->Tables2.InitForMarkup();
}

void TMainImpl::OnPostTransition(void* param, bool entered, const TNumerStat&, TPostDescriptor*& lastPost)
{
    TMainImpl* this_ = (TMainImpl*)param;
    if (entered) {
        if (!lastPost) {
            // the first post; stop looking for breadcrumbs and title
            for (int i = 0; i < NumPossibleBreadcrumbs; i++)
                this_->TagsProcessor.Stop(HANDLER_BREADCRUMB_SPECIFIC + i);
            this_->Breadcrumbs.Stop();
            for (int i = 0; i < NumPossibleTitles; i++)
                this_->TagsProcessor.Stop(HANDLER_TITLE + i);
            this_->PageNumbers.Stop();
        }
        this_->Posts.push_back(TPostDescriptor());
        lastPost = &this_->Posts.back();
        for (int i = HANDLER_START_POST_RELATED; i < HANDLER_END_POST_RELATED; i++)
            this_->TagsProcessor.Start(i);
    } else {
        lastPost->Anchor = this_->Anchors.GetLast();
        this_->Anchors.Clear();
        this_->Messages.Clear();
        for (int i = HANDLER_START_POST_RELATED; i < HANDLER_END_POST_RELATED; i++)
            this_->TagsProcessor.Stop(i);
        this_->TagsProcessor.Stop(HANDLER_SIGNATURE);
        this_->TagsProcessor.Stop(HANDLER_QUOTE_HEADER);
        this_->TagsProcessor.Stop(HANDLER_QUOTE_BODY);
        this_->TagsProcessor.Stop(HANDLER_NESTED_QUOTE_HEADER);
        this_->TagsProcessor.Stop(HANDLER_NESTED_QUOTE_BODY);
        if (!lastPost->Quotes.empty() && lastPost->Quotes.back().Begin == lastPost->SignatureBegin &&
            lastPost->Quotes.back().End == lastPost->SignatureEnd)
        {
            lastPost->Quotes.pop_back();
        }
    }
}

void TMainImpl::OnMessageTransition(void* param, bool entered, const TNumerStat& pos, TPostDescriptor*& lastPost)
{
    TMainImpl* this_ = (TMainImpl*)param;
    if (this_->TagsProcessor.IsInZone(HANDLER_QUOTE_HEADER) || this_->TagsProcessor.IsInZone(HANDLER_QUOTE_BODY))
        return;
    if (entered) {
        if (lastPost->MessageBegin) {
            if (this_->TagsProcessor.GetMarkup()->QuoteOutsideMessage)
                this_->Messages.TerminateQuote(pos);
            if (!lastPost->Quotes.empty() && lastPost->Quotes.back().End > lastPost->MessageEnd) {
                lastPost->MessageEnd = 0;
            } else {
                if (lastPost->SignatureEnd) {
                    lastPost->MessageEnd = lastPost->SignatureEnd;
                    lastPost->SignatureBegin = 0;
                    lastPost->SignatureEnd = 0;
                }
                this_->Messages.OnSignatureTransition(&this_->Messages, entered, pos, lastPost);
                return;
            }
        } else {
            this_->NumPosts++;
            lastPost->MessageBegin = pos.TokenPos.DocLength();
        }
        for (int i = 0; i < MaxDateChains; i++)
            this_->TagsProcessor.Stop(HANDLER_DATE + i);
        this_->TagsProcessor.Stop(HANDLER_AUTHOR);
        this_->TagsProcessor.Stop(HANDLER_AUTHOR_ALT);
        this_->TagsProcessor.Start(HANDLER_QUOTE_HEADER);
        this_->TagsProcessor.Start(HANDLER_QUOTE_BODY);
        if (this_->TagsProcessor.GetMarkup()->SignatureInsideMessage)
            this_->TagsProcessor.Start(HANDLER_SIGNATURE);
        else
            this_->TagsProcessor.Stop(HANDLER_SIGNATURE);
    } else {
        if (this_->TagsProcessor.GetMarkup()->QuoteOutsideMessage)
            NForumsImpl::TMessages::OnQuoteBodyTransition(&this_->Messages, true, pos, lastPost);
        if (lastPost->MessageEnd /*&& !KnownMarkups[this_->MarkupIndex].SignatureTag.NumTags*/) {
            this_->Messages.OnSignatureTransition(&this_->Messages, entered, pos, lastPost);
            return;
        }
        lastPost->MessageEnd = pos.TokenPos.DocLength();
        for (int i = 0; i < MaxDateChains; i++)
            this_->TagsProcessor.Start(HANDLER_DATE + i);
        this_->TagsProcessor.Start(HANDLER_AUTHOR);
        this_->TagsProcessor.Start(HANDLER_AUTHOR_ALT);
        if (this_->TagsProcessor.GetMarkup()->SignatureInsideMessage)
            this_->TagsProcessor.Stop(HANDLER_SIGNATURE);
        else
            this_->TagsProcessor.Start(HANDLER_SIGNATURE);
    }
}

void TMainImpl::OnMoveInput(const THtmlChunk& chunk, const TZoneEntry*, const TNumerStat& numerStat)
{
    if (chunk.IsCDATA) // ignore scripts, embedded css and so on
        return;
    if (chunk.flags.markup == MARKUP_IGNORED) {
        if (!chunk.Tag || *chunk.Tag != HT_STRONG && *chunk.Tag != HT_B)
            return;
        if (chunk.GetLexType() == HTLEX_START_TAG) {
            if (DepthOfStrong++)
                return;
        } else if (chunk.GetLexType() == HTLEX_END_TAG && DepthOfStrong > 0) {
            if (--DepthOfStrong)
                return;
        }
    }

    HTLEX_TYPE chunkLexType = chunk.GetLexType();
    if (chunkLexType == HTLEX_START_TAG || chunkLexType == HTLEX_END_TAG)
        // avoid date sticking with previous token which ends with digit
        // example: "Inna_17</a></strong> » December 27, 2011" ->
        // correct: 27.12.2011, wrong: 17.12.2027
        Dates.DivideDates();

    switch (chunkLexType) {
    case HTLEX_START_TAG:
        Anchors.CheckAnchorTag(PropSet, &chunk, TagsProcessor.GetMarkup());
        Messages.OnOpenTag(&chunk, numerStat);
        if (Posts.empty())
            Breadcrumbs.CheckBreadcrumbOpenTag(&chunk, GlobalDepth, PropSet);
        TagsProcessor.ProcessOpenTag(HT_any, &chunk, GlobalDepth, numerStat, Posts.empty() ? nullptr : &Posts.back());
        TagsProcessor.ProcessOpenTag(chunk.Tag->id(), &chunk, GlobalDepth, numerStat, Posts.empty() ? nullptr : &Posts.back());
        if (chunk.flags.markup != MARKUP_IGNORED) {
            Tables.OnOpenTag(&chunk, numerStat);
            Tables2.OnOpenTag(&chunk, numerStat);
            if (!Tables.Topics.empty() || !Tables2.Topics.empty())
                PageNumbers.Stop();
            GlobalDepth++;
            DepthOfStrong = 0;
        }
        if (!DepthOfLink && HrefTag.Match(&chunk))
            DepthOfLink = GlobalDepth;
        break;
    case HTLEX_EMPTY_TAG:
        if (*chunk.Tag != HT_IMG)
            Breadcrumbs.Stop();
        if (chunk.flags.markup != MARKUP_IGNORED) {
            Tables.OnOpenTag(&chunk, numerStat);
            Tables.OnCloseTag(&chunk, numerStat);
            Tables2.OnOpenTag(&chunk, numerStat);
            Tables2.OnCloseTag(&chunk, numerStat);
        }
        TagsProcessor.ProcessOpenTag(HT_any, &chunk, GlobalDepth, numerStat, Posts.empty() ? nullptr : &Posts.back());
        TagsProcessor.ProcessOpenTag(chunk.Tag->id(), &chunk, GlobalDepth, numerStat, Posts.empty() ? nullptr : &Posts.back());
        TagsProcessor.ProcessCloseTag(HT_any, &chunk, GlobalDepth, numerStat, Posts.empty() ? nullptr : &Posts.back());
        TagsProcessor.ProcessCloseTag(chunk.Tag->id(), &chunk, GlobalDepth, numerStat, Posts.empty() ? nullptr : &Posts.back());
        break;
    case HTLEX_END_TAG:
        if (chunk.flags.markup != MARKUP_IGNORED) {
            Tables.OnCloseTag(&chunk, numerStat);
            Tables2.OnCloseTag(&chunk, numerStat);
            GlobalDepth--;
            DepthOfStrong = 0;
        }
        if (DepthOfLink > GlobalDepth)
            DepthOfLink = 0;
        Messages.OnCloseTag(&chunk, numerStat);
        if (Posts.empty())
            Breadcrumbs.CheckBreadcrumbCloseTag(&chunk, GlobalDepth);
        TagsProcessor.ProcessCloseTag(HT_any, &chunk, GlobalDepth, numerStat, Posts.empty() ? nullptr : &Posts.back());
        TagsProcessor.ProcessCloseTag(chunk.Tag->id(), &chunk, GlobalDepth, numerStat, Posts.empty() ? nullptr : &Posts.back());
        if (*chunk.Tag == HT_TR)
            PageNumbers.BreakParagraph();
        break;
    case HTLEX_TEXT:
        // we can't process date in OnTokenStart/OnSpaces:
        // e.g. second markup for punBB uses "...<a href="...">N1</a></span><a href="...">18-12-2011 15:15:12</a></span>",
        // and this is lexed to ... 118-12-2011 ...
        // authorship is also more convenient to handle here, use the first tag-free text chunk
        // the same for breadcrumbs, the delimiters are sometimes included in the text
        if (Posts.empty())
            Breadcrumbs.CheckBreadcrumbText(chunk.text, chunk.leng);
        if (chunk.leng && (Dates.NeedText() || Authors.NeedText() || Breadcrumbs.NeedText() || Titles.NeedText() || Breadcrumbs.NeedCheckText()))
            ProcessText(chunk.text, chunk.leng);
        Tables.OnTextChunk(&chunk);
        Tables2.OnTextChunk(&chunk);
        break;
    default:
        break;
    }
}

void TMainImpl::OnTokenStart(const TWideToken& token, const TNumerStat& numerStat)
{
    // normal tokens are passed with leading ' ', tokens inside links are passed with leading pair " \x01"
    const wchar16 tokenStart[2] = { ' ', 1 };
    PageNumbers.ProcessToken(tokenStart, tokenStart + (DepthOfLink ? 2 : 1));
    PageNumbers.ProcessToken(token.Token, token.Token + token.Leng);
    Messages.OnTokenStart(token, numerStat);
    Tables.OnTokenStart(token, numerStat);
}

void TMainImpl::OnSpaces(const wchar16* token, unsigned len, const TNumerStat& numerStat)
{
    Messages.OnSpaces(token, len, numerStat);
    Tables.OnSpaces(token, len, numerStat);
}

void TMainImpl::OnTextEnd()
{
    Dates.PostProcessDates(Posts);
    Tables.PostProcess();
    Tables2.PostProcess();
    if (!Breadcrumbs.Empty() && Breadcrumbs.GetLast() == Titles.GetTitle())
        Breadcrumbs.DeleteLast();
    if (Generator == TForumsHandler::SimpleMachinesForum && !Breadcrumbs.Empty() && Titles.GetTitle().empty()) {
        Titles.SetTitle(Breadcrumbs.GetLast());
        Breadcrumbs.DeleteLast();
    }
}

static void DateToWtring(const TRecognizedDate& date, wchar16* str, unsigned* len)
{
    str[0] = (wchar16)(date.Day / 10 + '0');
    str[1] = (wchar16)(date.Day % 10 + '0');
    str[2] = '.';
    str[3] = (wchar16)(date.Month / 10 + '0');
    str[4] = (wchar16)(date.Month % 10 + '0');
    str[5] = '.';
    str[6] = (wchar16)(date.Year / 1000 + '0');
    str[7] = (wchar16)((date.Year / 100) % 10 + '0');
    str[8] = (wchar16)((date.Year / 10) % 10 + '0');
    str[9] = (wchar16)(date.Year % 10 + '0');
    *len = 10;
}

static void StoreDate(IDocumentDataInserter* inserter, const char* attrName, const TRecognizedDate& date, ui32 pos)
{
    wchar16 buf[16];
    unsigned len;
    DateToWtring(date, buf, &len);
    Y_ASSERT(len <= Y_ARRAY_SIZE(buf));
    inserter->StoreArchiveZoneAttr(attrName, buf, len, pos);
}

static void NumberToWtring(i64 number, wchar16* str, unsigned* len)
{
    unsigned s = 0;
    if (number < 0) {
        str[s++] = '-';
        number = -number;
    }
    unsigned l = s;
    do {
        str[l++] = '0' + number % 10;
        number /= 10;
    } while (number);
    Reverse(str + s, str + l);
    *len = l;
}

static void StoreNumber(IDocumentDataInserter* inserter, const char* attrName, i64 number, ui32 pos)
{
    wchar16 buf[32];
    unsigned len;
    NumberToWtring(number, buf, &len);
    Y_ASSERT(len <= Y_ARRAY_SIZE(buf));
    inserter->StoreArchiveZoneAttr(attrName, buf, len, pos);
}

struct TBaseUrl
{
    const char* BaseUrl;
    TStringBuf SidParameterName;
};

struct TSidScanner
{
    TStringBuf SidParameterName;
    TStringBuf Total;
    static bool IsValidSid(const TStringBuf& val)
    {
        if (val.size() < 24u)
            return false;
        return true;
    }
    void operator()(const TStringBuf& key, const TStringBuf& val)
    {
        if (key == SidParameterName && IsValidSid(val))
            Total = TStringBuf(key.data(), val.data() + val.size());
    }
};

static void StoreUrl(IDocumentDataInserter* inserter, const char* attrName, TPosting pos, const TBaseUrl& baseUrl, const TWtringBuf& url)
{
    NUri::TUri uri;
    NUri::TState::EParsed ret = uri.Parse(WideToUTF8(url), NUri::TUri::FeaturesDefault | NUri::TUri::FeatureSchemeKnown, baseUrl.BaseUrl);
    if (ret == NUri::TState::ParsedOK && (uri.GetScheme() == NUri::TUri::SchemeHTTP || uri.GetScheme() == NUri::TUri::SchemeHTTPS)) {
        TString str;
        if (!baseUrl.SidParameterName.empty()) {
            TSidScanner scanner = {baseUrl.SidParameterName, nullptr};
            TStringBuf query = uri.GetField(NUri::TUri::FieldQuery);
            ScanKeyValue<false, '&', '='>(query, scanner);
            if (!scanner.Total.empty()) {
                if (scanner.Total.size() == query.size()) {
                    uri.FldClr(NUri::TUri::FieldQuery);
                } else {
                    if (scanner.Total.data() != query.data())
                        scanner.Total = TStringBuf(scanner.Total.data() - 1, scanner.Total.size() + 1);
                    else
                        scanner.Total = TStringBuf(scanner.Total.data(), scanner.Total.size() + 1);
                    str = query;
                    str.remove(scanner.Total.data() - query.data(), scanner.Total.size());
                    uri.FldMemUse(NUri::TUri::FieldQuery, str);
                }
            }
        }
        TUtf16String wtr = UTF8ToWide<true>(uri.PrintS());
        inserter->StoreArchiveZoneAttr(attrName, wtr.data(), wtr.size(), pos);
    }
}

static void StoreQBodyAttributes(IDocumentDataInserter* inserter, TPosting pos, const TQuoteDescriptor* quote, const TBaseUrl& baseUrl)
{
    if (!quote->Author.empty())
        inserter->StoreArchiveZoneAttr(NArchiveZoneAttr::NForum::QUOTE_AUTHOR, quote->Author.data(), quote->Author.size(), pos);
    if (quote->Date.ToInt64())
        StoreDate(inserter, NArchiveZoneAttr::NForum::QUOTE_DATE, quote->Date, pos);
    if (!quote->Time.empty())
        inserter->StoreArchiveZoneAttr(NArchiveZoneAttr::NForum::QUOTE_TIME, quote->Time.data(), quote->Time.size(), pos);
    if (!quote->Url.empty())
        StoreUrl(inserter, NArchiveZoneAttr::NForum::QUOTE_URL, pos, baseUrl, quote->Url);
}

static void StoreQuoteBody(IDocumentDataInserter* inserter, const TQuoteDescriptor* quote, const TBaseUrl& baseUrl)
{
    TPosting cur = quote->BodyBegin;
    for (TVector<TQuoteDescriptor>::const_iterator it = quote->Nested.begin(); it != quote->Nested.end(); ++it) {
        if (cur < it->Begin) {
            inserter->StoreZone("forum_qbody", cur, it->Begin, true);
            StoreQBodyAttributes(inserter, cur, quote, baseUrl);
        }
        StoreQuoteBody(inserter, it, baseUrl);
        cur = it->End;
    }
    if (cur < quote->End) {
        inserter->StoreZone("forum_qbody", cur, quote->End, true);
        StoreQBodyAttributes(inserter, cur, quote, baseUrl);
    }
}

static void StoreSubforumInfo(IDocumentDataInserter* inserter, const TSubforumInfo* info, const TBaseUrl& baseUrl)
{
    inserter->StoreZone("forum_info", info->BeginPos, info->EndPos, true);
    if (info->ForumName.size())
        inserter->StoreArchiveZoneAttr(NArchiveZoneAttr::NForum::NAME, info->ForumName.data(), info->ForumName.size(), info->BeginPos);
    if (info->ForumLink.size())
        StoreUrl(inserter, NArchiveZoneAttr::NForum::LINK, info->BeginPos, baseUrl, info->ForumLink);
    if (info->ForumDescription.size())
        inserter->StoreArchiveZoneAttr(NArchiveZoneAttr::NForum::DESCRIPTION, info->ForumDescription.data(), info->ForumDescription.size(), info->BeginPos);
    if (info->LastMessageTitle.size())
        inserter->StoreArchiveZoneAttr(NArchiveZoneAttr::NForum::LAST_TITLE, info->LastMessageTitle.data(), info->LastMessageTitle.size(), info->BeginPos);
    if (info->LastMessageAuthor.size())
        inserter->StoreArchiveZoneAttr(NArchiveZoneAttr::NForum::LAST_AUTHOR, info->LastMessageAuthor.data(), info->LastMessageAuthor.size(), info->BeginPos);
    if (info->LastMessageDate.ToInt64())
        StoreDate(inserter, NArchiveZoneAttr::NForum::LAST_DATE, info->LastMessageDate, info->BeginPos);
    if (info->NumTopics.Defined())
        StoreNumber(inserter, NArchiveZoneAttr::NForum::TOPICS, *info->NumTopics.Get(), info->BeginPos);
    if (info->NumMessages.Defined())
        StoreNumber(inserter, NArchiveZoneAttr::NForum::MESSAGES, *info->NumMessages.Get(), info->BeginPos);
}

static void StoreTopicInfo(IDocumentDataInserter* inserter, const TTopicInfo* info, const TBaseUrl& baseUrl)
{
    inserter->StoreZone("forum_topic_info", info->BeginPos, info->EndPos, true);
    if (info->Title.size())
        inserter->StoreArchiveZoneAttr(NArchiveZoneAttr::NForum::NAME, info->Title.data(), info->Title.size(), info->BeginPos);
    if (info->Link.size())
        StoreUrl(inserter, NArchiveZoneAttr::NForum::LINK, info->BeginPos, baseUrl, info->Link);
    if (info->Author.size())
        inserter->StoreArchiveZoneAttr(NArchiveZoneAttr::NForum::AUTHOR, info->Author.data(), info->Author.size(), info->BeginPos);
    if (info->CreationDate.ToInt64())
        StoreDate(inserter, NArchiveZoneAttr::NForum::DATE, info->CreationDate, info->BeginPos);
    if (info->LastMessageDate.ToInt64())
        StoreDate(inserter, NArchiveZoneAttr::NForum::LAST_DATE, info->LastMessageDate, info->BeginPos);
    if (info->LastMessageAuthor.size())
        inserter->StoreArchiveZoneAttr(NArchiveZoneAttr::NForum::LAST_AUTHOR, info->LastMessageAuthor.data(), info->LastMessageAuthor.size(), info->BeginPos);
    if (info->Description.size())
        inserter->StoreArchiveZoneAttr(NArchiveZoneAttr::NForum::DESCRIPTION, info->Description.data(), info->Description.size(), info->BeginPos);
    if (info->NumMessages.Defined())
        StoreNumber(inserter, NArchiveZoneAttr::NForum::MESSAGES, *info->NumMessages.Get(), info->BeginPos);
    if (info->NumViews.Defined())
        StoreNumber(inserter, NArchiveZoneAttr::NForum::VIEWS, *info->NumViews.Get(), info->BeginPos);
}

void TMainImpl::OnCommitDoc(IDocumentDataInserter* inserter)
{
    const TVector<TSubforumInfo>& subForums = (Tables2.SubForums.empty() ? Tables.SubForums : Tables2.SubForums);
    const TVector<TTopicInfo>& topics = (Tables2.Topics.empty() ? Tables.Topics : Tables2.Topics);
    if (Posts.empty() && subForums.empty() && topics.empty())
        return;
    inserter->StoreTextArchiveDocAttr("forum_parser_ver", "3"); // parser version
    if (Dates.GetFirstPostDate().ToInt64())
        inserter->StoreErfDocAttr("FirstPostDate", Dates.GetFirstPostDate().ToString());
    if (Dates.GetLastPostDate().ToInt64())
        inserter->StoreErfDocAttr("LastPostDate", Dates.GetLastPostDate().ToString());
    if (NumPosts)
        inserter->StoreErfDocAttr("NumForumPosts", ToString(NumPosts));
    if (Authors.GetNumDifferentAuthors())
        inserter->StoreErfDocAttr("NumForumAuthors", ToString(Authors.GetNumDifferentAuthors()));
    if (!Titles.GetTitle().empty())
        inserter->StoreTextArchiveDocAttr("forum_title", WideToUTF8(Titles.GetTitle()));
    TString breadcrumb;
    Breadcrumbs.PackToString(breadcrumb);
    if (!breadcrumb.empty())
        inserter->StoreTextArchiveDocAttr("breadcrumb", breadcrumb);
    if (PageNumbers.GetPage())
        inserter->StoreTextArchiveDocAttr("forum_page", ToString(PageNumbers.GetPage()));
    if (PageNumbers.GetNumPages())
        inserter->StoreTextArchiveDocAttr("forum_total_pages", ToString(PageNumbers.GetNumPages()));
    if (!Posts.empty()) {
        if (PageNumbers.GetNumMessages())
            inserter->StoreTextArchiveDocAttr("forum_total_messages", ToString(PageNumbers.GetNumMessages()));
    } else {
        if (!subForums.empty())
            inserter->StoreTextArchiveDocAttr("forum_num_forums", ToString(subForums.size()));
        if (!topics.empty())
            inserter->StoreTextArchiveDocAttr("forum_num_topics", ToString(topics.size()));
    }
    TBaseUrl baseUrl = {nullptr, nullptr};
    PropSet->GetProperty(PP_BASE, &baseUrl.BaseUrl);
    if (TagsProcessor.GetMarkup())
        baseUrl.SidParameterName = TagsProcessor.GetMarkup()->SidParameterName;
    TPosting prevMessageBegin = 0;
    TPosting prevMessageEnd = 0;
    for (TVector<TPostDescriptor>::const_iterator it = Posts.begin(); it != Posts.end(); ++it) {
        if (!it->MessageBegin || !it->MessageEnd || it->MessageBegin == prevMessageBegin && it->MessageEnd == prevMessageEnd)
            continue;
        inserter->StoreZone("forum_message", it->MessageBegin, it->MessageEnd, true);
        prevMessageBegin = it->MessageBegin;
        prevMessageEnd = it->MessageEnd;
        if (!it->Author.empty())
            inserter->StoreArchiveZoneAttr(NArchiveZoneAttr::NForum::AUTHOR, it->Author.data(), it->Author.size(), it->MessageBegin);
        if (it->Date.ToInt64())
            StoreDate(inserter, NArchiveZoneAttr::NForum::DATE, it->Date, it->MessageBegin);
        if (!it->Anchor.empty())
            inserter->StoreArchiveZoneAttr(NArchiveZoneAttr::NForum::ANCHOR, it->Anchor.data(), it->Anchor.size(), it->MessageBegin);
        for (TPostDescriptor::TQuotesContainer::const_iterator jt = it->Quotes.begin(); jt != it->Quotes.end(); ++jt)
            if (jt->Begin != jt->End &&
                (jt->Begin >= it->MessageBegin && jt->End <= it->MessageEnd ||
                jt->Begin >= it->SignatureBegin && jt->End <= it->SignatureEnd))
            {
                inserter->StoreZone("forum_quote", jt->Begin, jt->End, true);
                StoreQuoteBody(inserter, jt, baseUrl);
            }
        if (it->SignatureBegin < it->SignatureEnd)
            inserter->StoreZone("forum_signature", it->SignatureBegin, it->SignatureEnd, true);
    }
    if (Posts.empty()) {
        for (TVector<TSubforumInfo>::const_iterator it = subForums.begin(); it != subForums.end(); ++it)
            StoreSubforumInfo(inserter, &*it, baseUrl);
        for (TVector<TTopicInfo>::const_iterator it = topics.begin(); it != topics.end(); ++it)
            StoreTopicInfo(inserter, &*it, baseUrl);
    }
}

}   // namespace NForumsImpl

TForumsHandler::TForumsHandler()
    : Impl(new NForumsImpl::TMainImpl)
{
}

TForumsHandler::~TForumsHandler()
{
}

TForumsHandler::EGenerator TForumsHandler::GetGenerator() const
{
    return Impl->Generator;
}

int TForumsHandler::GetNumPosts() const
{
    return Impl->NumPosts;
}

const TRecognizedDate& TForumsHandler::GetFirstPostDate() const
{
    return Impl->Dates.GetFirstPostDate();
}

const TRecognizedDate& TForumsHandler::GetLastPostDate() const
{
    return Impl->Dates.GetLastPostDate();
}

int TForumsHandler::GetNumDifferentAuthors() const
{
    return Impl->Authors.GetNumDifferentAuthors();
}

void TForumsHandler::Clear(void)
{
    Impl->Clear();
}

void TForumsHandler::OnAddDoc(const char* /*url*/, time_t date, bool isDateAmericanHint)
{
    Impl->Clear();
    Impl->Dates.SetDate(date, isDateAmericanHint);
    Impl->Tables.SetDate(date, isDateAmericanHint);
    Impl->Tables2.SetDate(date, isDateAmericanHint);
}

void TForumsHandler::OnCommitDoc(IDocumentDataInserter* inserter)
{
    Impl->OnCommitDoc(inserter);
}

void TForumsHandler::OnTextStart(const IParsedDocProperties* ps)
{
    Impl->OnTextStart(ps);
}

void TForumsHandler::OnTextEnd(const IParsedDocProperties*, const TNumerStat&)
{
    Impl->OnTextEnd();
}

void TForumsHandler::OnMoveInput(const THtmlChunk& chunk, const TZoneEntry* ze, const TNumerStat& numerStat)
{
    Impl->OnMoveInput(chunk, ze, numerStat);
}

void TForumsHandler::OnTokenStart(const TWideToken& token, const TNumerStat& numerStat)
{
    Impl->OnTokenStart(token, numerStat);
}

void TForumsHandler::OnSpaces(TBreakType, const wchar16* token, unsigned len, const TNumerStat& numerStat)
{
    Impl->OnSpaces(token, len, numerStat);
}
