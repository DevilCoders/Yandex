#include "tables.h"
#include "forums.h"
#include "forums_fsm.h"
#include "tags.h"

namespace NForumsImpl {

void TTablesExtractor::Clear()
{
    InsideHead = false;
    InsideBody = false;
    FSMActive = false;
    IgnoreBlock = false;
    Depth = 0;
    CurProcessor = nullptr;
    SubForums.clear();
    Topics.clear();
}

static int GetColSpan(const THtmlChunk* chunk)
{
    static const char strColSpan[] = "colspan";
    for (size_t i = 0; i < chunk->AttrCount; i++)
        if (chunk->Attrs[i].Name.Leng == sizeof(strColSpan) - 1) {
            const char* attr = chunk->text + chunk->Attrs[i].Name.Start;
            size_t j;
            for (j = 0; j < sizeof(strColSpan) - 1; j++)
                if ((attr[j] | 0x20) != strColSpan[j])
                    break;
            if (j >= sizeof(strColSpan) - 1)
                return atoi(chunk->text + chunk->Attrs[i].Value.Start);
        }
    return 1;   // default value
}

static inline bool HasIntersection(int start1, int end1, int start2, int end2)
{
    return (start1 < end2 && start2 < end1);
}

bool TTablesExtractor::FindColumn(ETableStrings& coltype)
{
    int found = -1;
    for (int i = 0; i < TS_COUNT; i++) {
        if (HasIntersection(CurColumn, CurColumn + CurColSpan, Columns[i].first, Columns[i].second)) {
            if (found < 0)
                found = i;
            else
                return false;   // table item spans several important columns
        }
    }
    if (found < 0)
        return false;
    coltype = (ETableStrings)found;
    return true;
}

/*
    The HTML syntax model used by parser (library/cpp/html/syntax/syntax.cpp) enforces several rules:
    every <tr> is inside <table>;
    every <tr> terminates previous <tr> except when it is fully nested with its parent <table>;
    every <th>/<td> is inside <tr>;
    every <th>/<td> terminates previous <th>/<td> except when it is fully nested with its parent <table>.
    If input does not satisfy to these rules, the parser inserts fake tags (MARKUP_IMPLIED) or
    marks tags as MARKUP_IGNORED.
    Thus, we can be sure that StartTable()/StartRow()/StartItem()/EndItem()/EndRow()/EndTable()
    are always called in this sequence for one table.
    However, there can be nested tables. For a nested table, we either consider it as a part of contents
    or abandon the current <table>, restarting with new <table>.
    In the first case, we don't call Start/End functions while nested.
    In the second case, the call sequence can be
    StartTable()/StartHeaderRow()/StartHeaderItem()/StartTable()/(normal processing)/EndTable()/EndTable().
    Rules for distinguishing between two cases:
    - a table inside body (i.e. in rows after a row with words recognized as column header) is always a part of contents
    - a table inside a column with colspan>=2 is a part of contents
    - otherwise, a table restarts processing
*/

void TTablesExtractor::StartTable()
{
    Y_ASSERT(!InsideBody);
    InsideHead = true;
    FSMActive = false;
    InsideBody = false;
    IgnoreBlock = false;
    SkipThisRow = true;
    CurColumn = 0;
    for (int i = 0; i < TS_COUNT; i++)
        Columns[i].second = Columns[i].first = -1;
}

void TTablesExtractor::EndTable()
{
    InsideHead = false;
    InsideBody = false;
}

void TTablesExtractor::StartHeaderRow()
{
    SkipThisRow = false;
}

void TTablesExtractor::EndHeaderRow()
{
    /*  There are some non-forums which happen to have set of links organized as table with several rows.
        If one of links points to a forum and is named "Forum", we will detect it as possible column with forum names.
        To avoid considering random links which happened to be located below forum link, we require that
        at least two columns must be recognized as forum-related. */
    int numRecognized = 0;
    for (int i = 0; i < TS_COUNT; i++)
        if (Columns[i].first >= 0)
            numRecognized++;
    if (numRecognized || !SkipThisRow) {
        InsideHead = false;
        InsideBody = (CurColumn > CurColSpan) && (numRecognized >= 2) && (Columns[TS_FORUM_NAME].first >= 0 || Columns[TS_TOPIC_NAME].first >= 0);
    }
}

void TTablesExtractor::StartHeaderItem()
{
    if (CurColSpan != 1)
        SkipThisRow = true;
    FSMActive = true;
    HeaderRec.InitFSM();
}

void TTablesExtractor::EndHeaderItem()
{
    if (FSMActive && HeaderRec.Recognized()) {
        if (HeaderRec.GetResult() == TS_SIMILAR_TOPICS) {
            SkipThisRow = false;
        } else {
            Columns[HeaderRec.GetResult()].first = CurColumn;
            Columns[HeaderRec.GetResult()].second = CurColumn + CurColSpan;
        }
    }
    FSMActive = false;
}

void TTablesExtractor::StartSubforumRow(ui32 pos)
{
    SkipThisRow = false;
    SubForums.emplace_back();
    SubforumRec.Start(pos, &SubForums.back());
}

void TTablesExtractor::EndSubforumRow(ui32 pos)
{
    Y_ASSERT(!SubForums.empty() && SubForums.back().EndPos == 0);
    if (!SubforumRec.End(pos, &SubForums.back()))
        SubForums.pop_back();
}

void TTablesExtractor::StartSubforumItem()
{
    Y_ASSERT(!SubForums.empty() && SubForums.back().EndPos == 0);
    ETableStrings colType;
    if (FindColumn(colType))
        CurProcessor = SubforumRec.GetProcessor(colType);
}

void TTablesExtractor::EndSubforumItem()
{
    Y_ASSERT(!SubForums.empty() && SubForums.back().EndPos == 0);
    CurProcessor = nullptr;
}

void TTablesExtractor::StartTopicRow(ui32 pos)
{
    SkipThisRow = false;
    Topics.emplace_back();
    TopicRec.Start(pos, &Topics.back());
}

void TTablesExtractor::EndTopicRow(ui32 pos)
{
    Y_ASSERT(!Topics.empty() && Topics.back().EndPos == 0);
    if (!TopicRec.End(pos, &Topics.back()))
        Topics.pop_back();
}

void TTablesExtractor::StartTopicItem()
{
    Y_ASSERT(!Topics.empty() && Topics.back().EndPos == 0);
    ETableStrings colType;
    if (FindColumn(colType))
        CurProcessor = TopicRec.GetProcessor(colType);
}

void TTablesExtractor::EndTopicItem()
{
    Y_ASSERT(!Topics.empty() && Topics.back().EndPos == 0);
    CurProcessor = nullptr;
}

static bool HasOnclick(const THtmlChunk* chunk)
{
    static const char onclick[] = "onclick";
    for (size_t i = 0; i < chunk->AttrCount; i++) {
        if (chunk->Attrs[i].Name.Leng == sizeof(onclick) - 1 &&
            !strnicmp(chunk->text + chunk->Attrs[i].Name.Start, onclick, sizeof(onclick) - 1))
        {
            return true;
        }
    }
    return false;
}

static bool IsRightAligner(const IParsedDocProperties* propSet, const THtmlChunk* chunk)
{
    static const char style[] = "style";
    for (size_t i = 0; i < chunk->AttrCount; i++) {
        if (chunk->Attrs[i].Name.Leng == sizeof(style) - 1 &&
            !strnicmp(chunk->text + chunk->Attrs[i].Name.Start, style, sizeof(style) - 1))
        {
            const char* attrValue = chunk->text + chunk->Attrs[i].Value.Start;
            unsigned attrValueLen = chunk->Attrs[i].Value.Leng;
            if (!attrValueLen)
                continue;
            TTempArray<wchar16> tempBuf(attrValueLen * 2);
            wchar16* ptr = tempBuf.Data();
            size_t bufLen = HtEntDecodeToChar(propSet->GetCharset(), attrValue, attrValueLen, ptr);
            if (!bufLen)
                continue;
            ToLower(ptr, bufLen);
            TWtringBuf input = TWtringBuf(ptr, bufLen);
            static const wchar16 align[] = {'f', 'l', 'o', 'a', 't', ':'};
            static const wchar16 right[] = {'r', 'i', 'g', 'h', 't'};
            size_t pos = input.find(TWtringBuf(align, Y_ARRAY_SIZE(align)));
            if (pos == TWtringBuf::npos)
                continue;
            pos += Y_ARRAY_SIZE(align);
            while (pos < bufLen && IsSpace(input[pos]))
                ++pos;
            if (input.SubStr(pos).StartsWith(TWtringBuf(right, Y_ARRAY_SIZE(right))))
                return true;
        }
    }
    return false;
}

void TTablesExtractor::OnOpenTag(const THtmlChunk* chunk, const TNumerStat& numerStat)
{
    if (CurProcessor && (chunk->Tag->is(HT_br) || chunk->Tag->is(HT_SPAN))) {
        CurProcessor->EndNestedWord();
        if (IsRightAligner(PropSet, chunk))
            CurProcessor->OnRightAligner();
    }
    Y_ASSERT(chunk->flags.markup != MARKUP_IGNORED); // rejected by a caller
    switch (chunk->Tag->id()) {
    case HT_TABLE:
        if (InsideBody || InsideHead && CurColSpan > 1) {
            ++Depth;
            return;
        }
        StartTable();
        break;
    case HT_TR:
        if (Depth)
            return;
        CurColumn = 0;
        if (InsideHead)
            StartHeaderRow();
        else if (InsideBody && IsSubforum())
            StartSubforumRow(numerStat.TokenPos.DocLength());
        else if (InsideBody && IsTopic())
            StartTopicRow(numerStat.TokenPos.DocLength());
        break;
    case HT_TD:
    case HT_TH:
        if (Depth)
            return;
        CurColSpan = GetColSpan(chunk);
        if (InsideHead)
            StartHeaderItem();
        else if (InsideBody && IsSubforum())
            StartSubforumItem();
        else if (InsideBody && IsTopic())
            StartTopicItem();
        break;
    case HT_BR:
        // treat <br> as '\n' character
        if (CurProcessor) {
            static const wchar16 newline[1] = {'\n'};
            CurProcessor->OnText(newline, 1);
        }
        break;
    case HT_SPAN:
        // assume <span onclick="..."> to be a pseudo-link
        if (!HasOnclick(chunk))
            break;
        [[fallthrough]];
    case HT_A:
        if (CurProcessor)
            CurProcessor->OnLinkTag(true, chunk);
        break;
    case HT_FIELDSET:
        IgnoreBlock = true;
        break;
    default:
        // shut up gcc, "enumeration value xxx not handled in switch"
        break;
    }
    Y_ASSERT(!(InsideHead && InsideBody));
    Y_ASSERT(InsideBody || !CurProcessor);
}

void TTablesExtractor::OnCloseTag(const THtmlChunk* chunk, const TNumerStat& numerStat)
{
    Y_ASSERT(chunk->flags.markup != MARKUP_IGNORED); // rejected by a caller
    switch (chunk->Tag->id()) {
    case HT_TABLE:
        if (Depth)
            --Depth;
        else
            EndTable();
        break;
    case HT_TR:
        if (Depth)
            return;
        if (InsideHead)
            EndHeaderRow();
        else if (InsideBody && IsSubforum())
            EndSubforumRow(numerStat.TokenPos.DocLength());
        else if (InsideBody && IsTopic())
            EndTopicRow(numerStat.TokenPos.DocLength());
        break;
    case HT_TD:
    case HT_TH:
        if (Depth) {
            if (CurProcessor)
                CurProcessor->EndNestedItem();
            return;
        }
        if (InsideHead)
            EndHeaderItem();
        else if (InsideBody && IsSubforum())
            EndSubforumItem();
        else if (InsideBody && IsTopic())
            EndTopicItem();
        CurColumn += CurColSpan;
        break;
    case HT_SPAN:
    case HT_A:
        if (CurProcessor)
            CurProcessor->OnLinkTag(false, chunk);
        break;
    case HT_FIELDSET:
        IgnoreBlock = false;
        break;
    default:
        if (CurProcessor && chunk->flags.brk >= BREAK_PARAGRAPH)
            CurProcessor->EndNestedItem();
        break;
    }
    if (CurProcessor && (chunk->Tag->is(HT_br) || chunk->Tag->is(HT_SPAN)))
        CurProcessor->EndNestedWord();
}

void TCommonTablesExtractor::PostProcess()
{
    TLastMessageRecognizer::PostprocessSubforums(SubForums, Date);
    TLastMessageRecognizer::PostprocessTopics(Topics, Date);
    // decide whether dates with zero priority are DD/MM/YY or MM/DD/YY
    bool canBeAmerican = true, canBeEuropean = true;
    for (TVector<TSubforumInfo>::const_iterator it = SubForums.begin(); it != SubForums.end(); ++it) {
        const TRecognizedDate& d = it->LastMessageDate;
        if (!d.Priority) {
            if (d.Month > 12)
                canBeEuropean = false;
            if (d.Day > 12)
                canBeAmerican = false;
        }
    }
    for (TVector<TTopicInfo>::const_iterator it = Topics.begin(); it != Topics.end(); ++it) {
        const TRecognizedDate& d1 = it->LastMessageDate;
        if (!d1.Priority) {
            if (d1.Month > 12)
                canBeEuropean = false;
            if (d1.Day > 12)
                canBeAmerican = false;
        }
        const TRecognizedDate& d2 = it->CreationDate;
        if (!d2.Priority) {
            if (d2.Month > 12)
                canBeEuropean = false;
            if (d2.Day > 12)
                canBeAmerican = false;
        }
    }
    bool isAmerican;
    if (canBeEuropean && !canBeAmerican)
        isAmerican = false;
    else if (!canBeEuropean && canBeAmerican)
        isAmerican = true;
    else
        isAmerican = IsDateAmericanHint;
    if (isAmerican) {
        for (TVector<TSubforumInfo>::iterator it = SubForums.begin(); it != SubForums.end(); ++it) {
            TRecognizedDate& d = it->LastMessageDate;
            if (!d.Priority)
                DoSwap(d.Day, d.Month);
        }
        for (TVector<TTopicInfo>::iterator it = Topics.begin(); it != Topics.end(); ++it) {
            TRecognizedDate& d1 = it->LastMessageDate;
            if (!d1.Priority)
                DoSwap(d1.Day, d1.Month);
            TRecognizedDate& d2 = it->CreationDate;
            if (!d2.Priority)
                DoSwap(d2.Day, d2.Month);
        }
    }
}

void TSpecificTablesExtractor::Clear()
{
    SubForums.clear();
    Topics.clear();
    CurProcessor = nullptr;
    IgnoreBlock = false;
}

void TSpecificTablesExtractor::RegisterCallbacks()
{
    TagsProcessor->Assign(HANDLER_SUBFORUM, &OnSubforumTransition, this);
    TagsProcessor->Assign(HANDLER_TOPIC, &OnTopicTransition, this);
    TagsProcessor->Assign(HANDLER_SEARCH, &OnSearchTransition, this);
    for (int i = 0; i < TS_COUNT; i++) {
        HandlersData[i].Parent = this;
        HandlersData[i].Id = (ETableStrings)i;
        TagsProcessor->Assign(HANDLER_INFO_START + i, &OnTableItemTransition, &HandlersData[i]);
    }
}

void TSpecificTablesExtractor::InitForMarkup()
{
    const TMarkup* markup = TagsProcessor->GetMarkup();
    Y_ASSERT(markup);
    if (markup->SubforumTag.NumTags) {
        TagsProcessor->SetAssociatedChain(HANDLER_SUBFORUM, &markup->SubforumTag);
        TagsProcessor->Start(HANDLER_SUBFORUM);
    }
    if (markup->TopicTag.NumTags) {
        TagsProcessor->SetAssociatedChain(HANDLER_TOPIC, &markup->TopicTag);
        TagsProcessor->Start(HANDLER_TOPIC);
    }
    if (markup->SearchTag.NumTags) {
        TagsProcessor->SetAssociatedChain(HANDLER_SEARCH, &markup->SearchTag);
        TagsProcessor->Start(HANDLER_SEARCH);
    }
    for (int i = 0; i < TS_COUNT; i++) {
        if (!markup->TableItemTags[i].Tag.NumTags)
            break;
        TagsProcessor->SetAssociatedChain(HANDLER_INFO_START + markup->TableItemTags[i].Id, &markup->TableItemTags[i].Tag);
    }
}

void TSpecificTablesExtractor::OnTableTransition(bool entered, const TNumerStat& pos, bool forTopic)
{
    const TMarkup* markup = TagsProcessor->GetMarkup();
    Y_ASSERT(markup);
    if (entered) {
        InTopic = forTopic;
        if (forTopic) {
            Topics.emplace_back();
            TopicRec.Start(pos.TokenPos.DocLength(), &Topics.back());
        } else {
            SubForums.emplace_back();
            SubforumRec.Start(pos.TokenPos.DocLength(), &SubForums.back());
        }
        for (int i = 0; i < TS_COUNT; i++) {
            if (!markup->TableItemTags[i].Tag.NumTags)
                break;
            if (forTopic ? markup->TableItemTags[i].ForTopic : markup->TableItemTags[i].ForForum)
                TagsProcessor->Start(HANDLER_INFO_START + markup->TableItemTags[i].Id);
        }
    } else {
        for (int i = 0; i < TS_COUNT; i++) {
            if (!markup->TableItemTags[i].Tag.NumTags)
                break;
            if (forTopic ? markup->TableItemTags[i].ForTopic : markup->TableItemTags[i].ForForum)
                TagsProcessor->Stop(HANDLER_INFO_START + markup->TableItemTags[i].Id);
        }
        if (forTopic) {
            Y_ASSERT(!Topics.empty() && Topics.back().EndPos == 0);
            if (!TopicRec.End(pos.TokenPos.DocLength(), &Topics.back()))
                Topics.pop_back();
        } else {
            Y_ASSERT(!SubForums.empty() && SubForums.back().EndPos == 0);
            if (!SubforumRec.End(pos.TokenPos.DocLength(), &SubForums.back()))
                SubForums.pop_back();
        }
    }
}

void TSpecificTablesExtractor::OnTableItemTransition(void* param, bool entered,
    const TNumerStat&, TPostDescriptor*&)
{
    THandlerData* data = (THandlerData*)param;
    if (entered) {
        if (data->Parent->InTopic)
            data->Parent->CurProcessor = data->Parent->TopicRec.GetProcessor(data->Id);
        else
            data->Parent->CurProcessor = data->Parent->SubforumRec.GetProcessor(data->Id);
    } else
        data->Parent->CurProcessor = nullptr;
}

static void AssignLink(const IParsedDocProperties* propSet, const THtmlChunk* chunk, TUtf16String* target)
{
    for (size_t i = 0; i < chunk->AttrCount; i++) {
        if (chunk->Attrs[i].Name.Leng == 4) {
            const char* attrName = chunk->text + chunk->Attrs[i].Name.Start;
            if ((*(ui32*)attrName | 0x20202020) == *(ui32*)"href") {
                const char* attrValue = chunk->text + chunk->Attrs[i].Value.Start;
                unsigned attrValueLen = chunk->Attrs[i].Value.Leng;
                if (!attrValueLen)
                    continue;
                TTempArray<wchar16> tempBuf(attrValueLen * 2);
                wchar16* ptr = tempBuf.Data();
                size_t bufLen = HtEntDecodeToChar(propSet->GetCharset(), attrValue, attrValueLen, ptr);
                if (!bufLen)
                    continue;
                target->assign(ptr, bufLen);
                break;
            }
        }
    }
}

static bool IsLoginLink(const TWtringBuf& link)
{
    size_t pos = link.find('?', 0);
    if (pos == TWtringBuf::npos)
        return false;
    static const wchar16 login1[] = {'l', 'o', 'g', 'i', 'n'};
    static const wchar16 login2[] = {'L', 'o', 'g', 'i', 'n'};
    static const wchar16 login3[] = {'L', 'O', 'G', 'I', 'N'};
    if (link.find(TWtringBuf(login1, 5), pos) ||
        link.find(TWtringBuf(login2, 5), pos) ||
        link.find(TWtringBuf(login3, 5), pos))
        return true;
    return false;
}

void TForumNameRecognizer::OnLinkTag(bool opened, const THtmlChunk* chunk)
{
    if (RightAligned)
        return;
    if (opened) {
        MaybeSubforumLink = false;
        if (!ForumName->empty()) {
            if (InsideDescription && IsMeaningless(*ForumDescription)) {
                ForumDescription->clear();
                InsideDescription = false;
                if (ForumName->size() <= 1 && IsLoginLink(*ForumLink))
                    ForumName->clear();
                else
                    return;
            } else {
                const wchar16* p = ForumDescription->data();
                const wchar16* pe = p + ForumDescription->size();
                while (pe > p && IsSpace(pe[-1]))
                    pe--;
                if (pe > p && pe[-1] == 0x2022) { // bullet
                    pe--;
                    while (pe > p && IsSpace(pe[-1]))
                        pe--;
                    if (MaybeSubforumEnd < pe - p)
                        MaybeSubforumStart = pe - p;
                    MaybeSubforumLink = true;
                }
                return;
            }
        }
        AssignLink(PropSet, chunk, ForumLink);
        if (!ForumLink->empty() && !(ForumLink->size() == 1 && (*ForumLink)[0] == '#'))
            InsideLink = true;
    } else {
        if (MaybeSubforumLink) {
            MaybeSubforumEnd = ForumDescription->size();
            MaybeSubforumLink = false;
        }
        if (!ForumName->empty() && ForumDescription->empty())
            InsideDescription = true;
        InsideLink = false;
    }
}

void TForumNameRecognizer::OnText(const wchar16* text, size_t len)
{
    if (RightAligned)
        return;
    if (InsideLink)
        ForumName->append(text, len);
    else if (InsideDescription) {
        if (ForumDescription->empty()) {
            while (len && IsSpace(text[0]))
                --len, ++text;
        }
        if (len)
            ForumDescription->append(text, len);
    }
}

static inline bool IsEllipsis(const TWtringBuf& str)
{
    const wchar16* p = str.data();
    const wchar16* pe = p + str.size();
    while (p < pe && IsSpace(*p))
        ++p;
    while (p < pe && IsSpace(pe[-1]))
        --pe;
    if (pe == p + 1 && (*p == 0x2026 || *p == 0x22EF))
        return true;
    if (pe == p + 3 && p[0] == '.' && p[1] == '.' && p[2] == '.')
        return true;
    return false;
}

void TTopicNameRecognizer::OnLinkTag(bool opened, const THtmlChunk* chunk)
{
    if (AdState == IsAd)
        return;

    if (opened) {
        InsideLink = true;
        if (TopicName->empty()) {
            static const TTagDescriptor ipsBadge = { HT_any, 0, "class", 5, "ipsBadge", 8, TTagDescriptor::Prefix };
            AssignLink(PropSet, chunk, TopicLink);
            if (!TopicLink->empty() && !(TopicLink->size() == 1 && (*TopicLink)[0] == '#') && !ipsBadge.Match(chunk)) {
                Active = TopicName;
                TopicAuthor->clear();
            }
        } else if (PagerState == InUnclosedPager && IsEllipsis(*TopicAuthor)) {
            TopicAuthor->clear();
            CurPage = (unsigned)-1;
        }
    } else {
        InsideLink = false;
        if (Active == TopicName && !Active->empty()) {
            PagerState = MaybePager;
            TopicAuthor->clear();
        }
        Active = nullptr;
        if ((PagerState == MaybePager || PagerState == InUnclosedPager) && !TopicAuthor->empty()) {
            bool onlyDigits = true;
            unsigned number = 0;
            bool isLast = false;
            for (TWtringBuf::const_iterator it = TopicAuthor->begin(); it != TopicAuthor->end(); ++it)
                if (IsCommonDigit(*it))
                    number = number * 10 + (*it - '0');
                else if (*it == 0xBB || *it == 0x2192)  // unicode for ">> N" and "N ->"
                    isLast = true;
                else if (!IsSpace(*it)) {
                    onlyDigits = false;
                    break;
                }
            if (onlyDigits && (CurPage == (unsigned)-1 || number == CurPage + 1 || isLast && number > CurPage + 1)) {
                CurPage = number;
                TopicAuthor->clear();
                PagerState = InUnclosedPager;
            } else
                PagerState = NoPager;
        }
    }
}

void TTopicNameRecognizer::OnText(const wchar16* text, size_t len)
{
    if (AdState == MaybeAd)
        AdState = IsAdPrefixTopic(text, text + len) ? IsAd : NoAd;
    if (AdState == IsAd)
        return;

    if (PagerState == InPager) {
        for (size_t i = 0; i < len; i++)
            if (text[i] == PagerClose) {
                PagerState = NoPager;
                text += i + 1;
                len -= i + 1;
                break;
            }
    }
    if (PagerState == MaybePager || Active == TopicName && !Active->empty()) {
        size_t i = 0;
        while (i < len && IsSpace(text[i]))
            i++;
        if (i < len) {
            if (text[i] == '(')
                PagerState = InPager, PagerClose = ')';
            else if (text[i] == '[')
                PagerState = InPager, PagerClose = ']';
            else if (text[i] == 0xAB)
                PagerState = InPager, PagerClose = 0xBB;
            for (; i < len; i++)
                if (text[i] == PagerClose)
                    break;
            if (i < len) // assume author like "[here be nick]"
                PagerState = NoPager;
            else
                Active = nullptr;
        }
    }
    if (Active) {
        while (len && (*text == '\r' || *text == '\n' || IsSpace(*text)))
            ++text, --len;
        while (len && (text[len - 1] == '\r' || text[len - 1] == '\n' || IsSpace(text[len - 1])))
            --len;

        Active->append(text, len);
    } else if (PagerState != InPager) {
        if (TopicAuthor->empty()) {
            if (!InsideLink) {
                const wchar16* author = IsAuthorshipPrefixTopic(text, text + len);
                if (author)
                    len = text + len - author, text = author;
                while (len && IsSpace(text[0]))
                    --len, ++text;
                while (len && IsSpace(text[len - 1]))
                    --len;
                if (len && (text[len - 1] == ',' || text[len - 1] == '[')) {
                    --len;
                    while (len && IsSpace(text[len - 1]))
                        --len;
                }
            }
            if (len)
                TopicAuthor->append(text, len);
        } else {
            DateRec.OnDateText(text, len);
        }
    }
}

// numbers: "nnnnnnnnn", "nnn nnn nnn", "nnn,nnn,nnn"
void TNumberRecognizer::OnText(const wchar16* text, size_t len)
{
    i64* p = Number.Get();
    if (!p)
        return;
    for (size_t i = 0; i < len; i++) {
        wchar16 a = text[i];
        if (IsSpace(a) || a == ',')
            continue;
        ThereWasText = true;
        if (!IsCommonDigit(a)) {
            Target->Clear();
            break;
        }
        *p = *p * 10 + (a - '0');
    }
}

void TLastMessageRecognizer::EndNestedWord()
{
    BlockStarted = false;
    if (!Author->empty())
        RecognizingAuthor = false;
}

void TLastMessageRecognizer::OnText(const wchar16* text, size_t len)
{
    if (len == 1 && text[0] == '\n')
        return;
    if (!InsideLink) {
        while (len && IsSpace(*text))
            ++text, --len;
    }
    if (!len)
        return;
    if (!BlockStarted) {
        Blocks->push_back(TLastMessageBlock());
        Blocks->back().InsideLink = false;
        Blocks->back().IsAuthorship = false;
        BlockStarted = true;
    }
    if (InsideLink) {
        Blocks->back().Text.clear();
        Blocks->back().InsideLink = true;
    }
    Blocks->back().Text.append(text, len);
    if (!RecognizingAuthor) {
        const wchar16* author = IsAuthorshipPrefixLastMsg(text, text + len);
        if (author) {
            const wchar16* end = text + len;
            while (author < end && IsSpace(*author))
                ++author;
            text = author;
            len = end - author;
            RecognizingAuthor = true;
            Author->clear();
            for (TVector<TLastMessageBlock>::iterator it = Blocks->begin(); it != Blocks->end(); ++it)
                if (it->IsAuthorship)
                    it->IsAuthorship = false;
        }
    }
    if (RecognizingAuthor) {
        if (Author->empty()) {
            while (len && (*text == '\r' || *text == '\n'))
                ++text, --len;
            while (len && (text[len - 1] == '\r' || text[len - 1] == '\n'))
                --len;
            if (!InsideLink) {
                while (len && IsSpace(*text))
                    ++text, --len;
                while (len && IsSpace(text[len - 1]))
                    --len;
            }
        }
        Author->append(text, len);
        Blocks->back().IsAuthorship = true;
    }
}

void TLastMessageRecognizer::OnLinkTag(bool opened, const THtmlChunk*)
{
    if (opened) {
        InsideLink = true;
    } else {
        InsideLink = false;
        if (!Blocks->empty() && Blocks->back().Text.size() == 1 && IsArrowCharacter(Blocks->back().Text[0])) {
            Blocks->pop_back();
            BlockStarted = false;
        }
    }
}

template<typename T>
void ParseDatesColumn(TVector<T>& scan, time_t date, THashMap<int, int>& dateColumn)
{
    // find the block with most recognized dates
    THashMap< int, TVector<int> > numRecognizedDates;
    for (typename TVector<T>::const_iterator it = scan.begin(); it != scan.end(); ++it) {
        size_t numBlocks = it->LastMessageBlocks.size();
        if (!numBlocks)
            continue;
        TVector<int>& r = numRecognizedDates[numBlocks];
        for (TVector<TLastMessageBlock>::const_iterator jt = it->LastMessageBlocks.begin(); jt != it->LastMessageBlocks.end(); ++jt) {
            TForumDateRecognizer recognizer;
            recognizer.Clear();
            recognizer.SetDate(date);
            recognizer.OnDateText(jt->Text.begin(), jt->Text.size());
            TRecognizedDate date;
            if (recognizer.GetDate(&date)) {
                size_t block = jt - it->LastMessageBlocks.begin();
                if (r.size() <= block)
                    r.resize(block + 1, 0);
                r[block]++;
            }
        }
    }
    for (THashMap< int, TVector<int> >::const_iterator it = numRecognizedDates.begin(); it != numRecognizedDates.end(); ++it) {
        const TVector<int>& r = it->second;
        int dateBlock = -1;
        int bestResult = 0;
        for (TVector<int>::const_iterator jt = r.begin(); jt != r.end(); ++jt)
            if (bestResult < *jt) {
                bestResult = *jt;
                dateBlock = jt - r.begin();
            }
        if (dateBlock >= 0)
            dateColumn[it->first] = dateBlock;
    }
    if (dateColumn.empty())
        return;
    // use it
    for (typename TVector<T>::iterator it = scan.begin(); it != scan.end(); ++it) {
        size_t numBlocks = it->LastMessageBlocks.size();
        if (const int* ptr = dateColumn.FindPtr(numBlocks)) {
            TForumDateRecognizer recognizer;
            recognizer.Clear();
            recognizer.SetDate(date);
            recognizer.OnDateText(it->LastMessageBlocks[*ptr].Text.begin(), it->LastMessageBlocks[*ptr].Text.size());
            recognizer.GetDate(&it->LastMessageDate);
        }
    }
}

static bool IsTime(const TWtringBuf& text)
{
    const wchar16* start = text.data();
    const wchar16* end = start + text.size();
    while (start < end && IsSpace(*start))
        ++start;
    while (start < end && IsSpace(end[-1]))
        --end;
    if (end - start >= 5 && IsDigit(start[0]) && IsDigit(start[1]) && start[2] == ':' && IsDigit(start[3]) && IsDigit(start[4]))
        return true;
    if (end - start == 7 && IsDigit(start[0]) && start[1] == ':' &&
        IsDigit(start[2]) && IsDigit(start[3]) && start[4] == ':' &&
        IsDigit(start[5]) && IsDigit(start[6]))
    {
        return true;
    }
    return false;
}

void TLastMessageRecognizer::PostprocessSubforums(TVector<TSubforumInfo>& subforums, time_t date)
{
    /*for (TVector<TSubforumInfo>::iterator it = subforums.begin(); it != subforums.end(); ++it) {
        for (TVector<TLastMessageBlock>::iterator jt = it->LastMessageBlocks.begin(); jt != it->LastMessageBlocks.end(); ++jt)
            Cout << "last_message_block" << (int)(jt - it->LastMessageBlocks.begin()) << " = " << jt->Text << "\n";
    }*/
    THashMap<int, int> dateColumns;
    ParseDatesColumn(subforums, date, dateColumns);
    // no dates? Something is wrong, better to give up
    if (dateColumns.empty())
        return;
    for (TVector<TSubforumInfo>::iterator it = subforums.begin(); it != subforums.end(); ++it) {
        size_t numBlocks = it->LastMessageBlocks.size();
        THashMap<int, int>::iterator dateIter = dateColumns.find(numBlocks);
        if (dateIter == dateColumns.end())
            continue;
        int dateColumn = dateIter->second;
        bool topicFound = false;
        for (int topicColumn = 0; (size_t)topicColumn < numBlocks; topicColumn++) {
            if (topicColumn == dateColumn)
                continue;
            if (topicColumn == dateColumn + 1 && IsTime(it->LastMessageBlocks[topicColumn].Text))
                // special for DonanimHaber: date -> time -> author
                if ((size_t)topicColumn + 1 == numBlocks - 1 && it->LastMessageAuthor.empty())
                    break;
                else
                    continue;
            if (!it->LastMessageBlocks[topicColumn].InsideLink)
                continue;
            topicFound = true;
            it->LastMessageTitle = it->LastMessageBlocks[topicColumn].Text;
            if (it->LastMessageAuthor.empty()) {
                for (int authorColumn = topicColumn + 1; (size_t)authorColumn < numBlocks; authorColumn++) {
                    if (authorColumn == dateColumn)
                        continue;
                    it->LastMessageAuthor = it->LastMessageBlocks[authorColumn].Text;
                    if (!it->LastMessageBlocks[authorColumn].InsideLink)
                        TrimRight(&it->LastMessageAuthor);
                    break;
                }
            }
            break;
        }
        if (!topicFound && it->LastMessageAuthor.empty()) {
            for (int authorColumn = 0; (size_t)authorColumn < numBlocks; authorColumn++) {
                if (authorColumn == dateColumn)
                    continue;
                if (authorColumn == dateColumn + 1 && IsTime(it->LastMessageBlocks[authorColumn].Text))
                    continue;
                it->LastMessageAuthor = it->LastMessageBlocks[authorColumn].Text;
                if (!it->LastMessageBlocks[authorColumn].InsideLink)
                    TrimRight(&it->LastMessageAuthor);
            }
        }
    }
    bool removeTrailingDash = true;
    for (TVector<TSubforumInfo>::iterator it = subforums.begin(); it != subforums.end(); ++it) {
        size_t s = it->LastMessageAuthor.size();
        if (s && !(s >= 2 && it->LastMessageAuthor[s - 1] == '-' && it->LastMessageAuthor[s - 2] == ' '))
            removeTrailingDash = false;
    }
    if (removeTrailingDash)
        for (TVector<TSubforumInfo>::iterator it = subforums.begin(); it != subforums.end(); ++it)
            if (!it->LastMessageAuthor.empty())
                it->LastMessageAuthor.remove(it->LastMessageAuthor.size() - 2);
    bool removeParentheses = true;
    for (TVector<TSubforumInfo>::iterator it = subforums.begin(); it != subforums.end(); ++it) {
        size_t s = it->LastMessageAuthor.size();
        if (s && !(s >= 2 && it->LastMessageAuthor[0] == '(' && it->LastMessageAuthor[s - 1] == ')'))
            removeParentheses = false;
    }
    if (removeParentheses)
        for (TVector<TSubforumInfo>::iterator it = subforums.begin(); it != subforums.end(); ++it) {
            if (it->LastMessageAuthor.empty())
                continue;
            it->LastMessageAuthor.remove(it->LastMessageAuthor.size() - 1);
            it->LastMessageAuthor.remove(0, 1);
        }
}

void TLastMessageRecognizer::PostprocessTopics(TVector<TTopicInfo>& topics, time_t date)
{
    /*for (TVector<TTopicInfo>::iterator it = topics.begin(); it != topics.end(); ++it) {
        for (TVector<TLastMessageBlock>::iterator jt = it->LastMessageBlocks.begin(); jt != it->LastMessageBlocks.end(); ++jt)
            Cout << "last_message_block" << (int)(jt - it->LastMessageBlocks.begin()) << " = " << jt->Text << "\n";
    }*/
    THashMap<int, int> dateColumns;
    ParseDatesColumn(topics, date, dateColumns);
    if (dateColumns.empty())
        return;
    for (TVector<TTopicInfo>::iterator it = topics.begin(); it != topics.end(); ++it) {
        if (!it->LastMessageAuthor.empty())
            continue;
        size_t numBlocks = it->LastMessageBlocks.size();
        THashMap<int, int>::iterator dateIter = dateColumns.find(numBlocks);
        if (dateIter == dateColumns.end())
            continue;
        int dateColumn = dateIter->second;
        int authorColumn = 0;
        if (dateColumn == 0) {
            authorColumn++;
            if (numBlocks >= 2 && IsTime(it->LastMessageBlocks[1].Text))
                authorColumn++;
        }
        if ((size_t)authorColumn < numBlocks) {
            it->LastMessageAuthor = it->LastMessageBlocks[authorColumn].Text;
            if (!it->LastMessageBlocks[authorColumn].InsideLink)
                TrimRight(&it->LastMessageAuthor);
        }
    }
    bool removeLeadingDash = true;
    for (TVector<TTopicInfo>::iterator it = topics.begin(); it != topics.end(); ++it)
        if (!(it->LastMessageAuthor.size() >= 2 && it->LastMessageAuthor[0] == '-' && it->LastMessageAuthor[1] == 160))
            removeLeadingDash = false;
    if (removeLeadingDash)
        for (TVector<TTopicInfo>::iterator it = topics.begin(); it != topics.end(); ++it)
            it->LastMessageAuthor.remove(0, 2);
}

} // namespace NForumsImpl
