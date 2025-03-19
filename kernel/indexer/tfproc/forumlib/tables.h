#pragma once
#include "dates.h"
#include "forums.h"
#include "forums_fsm.h"
#include <library/cpp/html/entity/htmlentity.h>

namespace NForumsImpl {

/*
    There are two possible ways to get the (decoded) text from HTML in API of the parser:
    the first is OnTokenStart/OnSpaces,
    the second is OnMoveInput (or even OnAddEvent or OnRemoveEvent) and to decode text chunks manually.

    Unfortunately, OnSpaces collapses all consecutive non-token characters into one event,
    even if they are from different nodes of DOM tree:
    for "text1 <a>(text2)</a> text3" OnSpaces is called two times, with " (" and ") ",
    and there is no easy way to determine whether these parentheses are part of link or not.
    This is a problem for recognizing (is this '(' a start of pager or an end of topic name
    like "sad topic :("?) and, more important, looks weird when topic name or author name
    has paired non-tokens ("[something] lorem ipsum" or "*** lorem ipsum ***") and one
    of those gets abandoned.
    (The first version of the code has used this method and suffered from both problems.)

    In theory, we can hook (rarely used) OnTokenBegin/OnTokenEnd in addition to
    OnTokenStart+OnSpaces+OnMoveInput; they provide offsetInEvent for tokens, so
    for "text1 <a>(text2)</a> text3" we would know that "text2" starts at position 1
    in the current chunk, thus " (" should be split as " " + "(". However, there exist
    complicated cases like "text1: <a>***</a> (text2)" ("***" is a valid topic name),
    when tokenized event is ": *** (", these require an event queue like
    spaces(": *** ("), tag(<a>), raw("***"), tag(</a>), and correct processing of
    this queue at OnTokenBegin(offsetInEvent = 2) for "text2", when it becomes known
    that " (" is a part of last chunk, length of decoded text "***" is 3 (here it can
    be calculated without direct decoding: plain ASCII characters other than '&' are always
    decoded to themselves), so ": " must be the end of first chunk. And, by the way:
    the tokenizer inserts a space after each word-break tag, they should be skipped.
    Obviously, nontrivial interactions between 5 types of events with a queue of pending
    events would be quite complicated and, more important, vulnerable to changes
    in html decoder/tokenizer.

    Thus, we use the second method, manually decoding text chunks in OnMoveInput.
    We need this only when we are actually recognizing some items in a table, so
    the overhead from re-decoding is small (test program which does only parsing+running
    this code is about 1% slower with the second version of the code on test data
    consisting only of forum/topic lists; since re-decoding is not needed on other pages,
    this number is even smaller on real data).
    It means that item-recognizers have no tokenized text and no numerator data,
    just OnText(const wchar16*, size_t).

    Note: the generic table recognizer (TTablesExtractor, see below)
    also uses OnTokenStart/OnSpaces: it recognizes a text in header of a table
    (e.g. if a table column has a header with text "Forum", the column probably
    contains names of subforums), and exact bounds for non-tokens are not needed here.
*/

class ITableItemProcessor
{
public:
    virtual void OnText(const wchar16* /*text*/, size_t /*len*/)
    {}
    virtual void OnLinkTag(bool /*opened*/, const THtmlChunk* /*chunk*/)
    {}
    virtual void EndNestedWord()
    {}
    virtual void EndNestedItem()
    {}
    virtual void OnRightAligner()
    {}
    virtual ~ITableItemProcessor()
    {}
};

inline void TrimRight(TUtf16String* w)
{
    while (!w->empty() && IsSpace(w->back()))
        w->pop_back();
}

class TNumberRecognizer : public ITableItemProcessor
{
public:
    void Start(TMaybe<i64>* target)
    {
        Target = target;
        Number = 0;
        ThereWasText = false;
    }
    void End()
    {
        if (!ThereWasText)
            Number.Clear();
        if (Number.Defined())
            *Target = Number;
    }

    void OnText(const wchar16* text, size_t len) override;
private:
    TMaybe<i64>* Target;
    bool ThereWasText;
    TMaybe<i64> Number;
};

class TTwoNumbersRecognizer : public ITableItemProcessor
{
public:
    void Start(TMaybe<i64>* target1, TMaybe<i64>* target2)
    {
        Target1 = target1;
        Target2 = target2;
        FSM.InitFSM();
    }
    void End()
    {
        if (FSM.Recognized()) {
            *Target1 = FSM.Number1;
            *Target2 = FSM.Number2;
        }
    }
    void OnText(const wchar16* text, size_t len) override
    {
        FSM.RunFSM(text, text + len);
    }
private:
    TTwoNumbersFSM FSM;
    TMaybe<i64>* Target1;
    TMaybe<i64>* Target2;
};

class TStringRecognizer : public ITableItemProcessor
{
public:
    void Start(TUtf16String* target)
    {
        Target = target;
        Exist = false;
    }
    void Clear()
    {
        Target->clear();
    }
    void End()
    {
    }
    void OnText(const wchar16* text, size_t len) override
    {
        if (!Exist) {
            Target->clear();
            Exist = true;
        }
        Target->append(text, len);
    }
private:
    TUtf16String* Target;
    bool Exist;
};

inline bool IsMeaningless(const TWtringBuf& what)
{
    for (TWtringBuf::const_iterator it = what.begin(); it != what.end(); ++it)
        if (!IsSpace(*it) && !IsPunct(*it))
            return false;
    return true;
}

class TForumNameRecognizer : public ITableItemProcessor
{
public:
    TForumNameRecognizer()
        : PropSet(nullptr)
    {}
    void SetDocProps(const IParsedDocProperties* propSet)
    {
        PropSet = propSet;
    }
    void Start(TUtf16String* name, TUtf16String* link, TUtf16String* description, TMaybe<i64>* numTopics)
    {
        ForumName = name;
        ForumLink = link;
        ForumDescription = description;
        InsideLink = false;
        InsideDescription = false;
        MaybeSubforumStart = 0;
        MaybeSubforumEnd = 0;
        MaybeSubforumLink = false;
        NumTopics = numTopics;
        RightAligned = false;
    }
    void End()
    {
        const wchar16* d = ForumDescription->data();
        const wchar16* de = d + ForumDescription->size();
        while (d < de && IsSpace(*d))
            ++d;
        const wchar16* p = FindEndOfDescription(d, de);
        if (p == d && (*p == '(' || *p == '[')) {
            wchar16 close = (*p == '(' ? ')' : ']');
            for (; d < de && *d != close; d++)
                ;
            if (d < de) {
                ++d;
                while (d < de && IsSpace(*d))
                    ++d;
                unsigned toDel = d - ForumDescription->data();
                ForumDescription->remove(0, toDel);
                MaybeSubforumStart = Max(MaybeSubforumStart, toDel) - toDel;
                MaybeSubforumEnd = Max(MaybeSubforumEnd, toDel) - toDel;
                d = ForumDescription->data();
                p = FindEndOfDescription(d, d + ForumDescription->size());
            }
        }
        if (p)
            ForumDescription->remove(p - d);
        TrimRight(ForumDescription);
        if (MaybeSubforumEnd >= ForumDescription->size())
            ForumDescription->remove(MaybeSubforumStart);
        int numTopics;
        d = ForumDescription->data();
        p = IsNumTopicsPrefix(d, d + ForumDescription->size(), &numTopics);
        if (p) {
            ForumDescription->remove(0, p - d);
            *NumTopics = numTopics;
        }
    }
    void OnText(const wchar16* text, size_t len) override;
    void OnLinkTag(bool opened, const THtmlChunk* chunk) override;
    void OnRightAligner() override
    {
        RightAligned = true;
    }
    void EndNestedItem() override
    {
        if (InsideDescription) {
            const wchar16* d = ForumDescription->data();
            if (IsMeaningless(*ForumDescription) || FindEndOfDescription(d, d + ForumDescription->size()) == d)
                ForumDescription->clear();
            else
                InsideDescription = false;
        }
        RightAligned = false;
    }
private:
    const IParsedDocProperties* PropSet;
    TUtf16String* ForumName;
    TUtf16String* ForumLink;
    TUtf16String* ForumDescription;
    TMaybe<i64>* NumTopics;
    unsigned MaybeSubforumStart, MaybeSubforumEnd;
    bool InsideLink, InsideDescription;
    bool MaybeSubforumLink;
    bool RightAligned;
};

struct TMarkup;

struct TLastMessageBlock
{
    bool InsideLink;
    bool IsAuthorship;
    TUtf16String Text;
};

struct TSubforumInfo
{
    ui32 BeginPos, EndPos;
    TUtf16String ForumName, ForumLink;
    TUtf16String ForumDescription;
    TUtf16String LastMessageTitle, LastMessageAuthor;
    TRecognizedDate LastMessageDate;
    TVector<TLastMessageBlock> LastMessageBlocks;
    TMaybe<i64> NumTopics, NumMessages;
    TSubforumInfo()
        : LastMessageDate(0, 0, 0)
    {}
};

struct TTopicInfo
{
    ui32 BeginPos, EndPos;
    TUtf16String Title, Link, Description;
    TUtf16String Author, LastMessageAuthor;
    TRecognizedDate CreationDate, LastMessageDate;
    TVector<TLastMessageBlock> LastMessageBlocks;
    TMaybe<i64> NumMessages, NumViews;
    TTopicInfo()
        : CreationDate(0, 0, 0)
        , LastMessageDate(0, 0, 0)
    {}
};

/*
    The last message column contains last topic name (for subforums), author of last message, date/time of last message.
    Last topic name is always a link to the topic, author and date/time may be (one or both) links or plain text
    depending on markup and whether author was deleted.
    Sometimes author is prefixed with some string like "Author: " or "by". In this case, TLastMessageRecognizer
    fills the author directly. Sometimes author is not marked with text. Then TLastMessageRecognizer just collects
    everything in blocks. Postprocessing stage processes collected blocks in bulk and tries to recognize blocks
    with dates, topic names, authors.
*/
class TLastMessageRecognizer : public ITableItemProcessor
{
public:
    void OnText(const wchar16* text, size_t len) override;
    void OnLinkTag(bool opened, const THtmlChunk* /*chunk*/) override;
    void EndNestedWord() override;
    void Start(TUtf16String* author, TVector<TLastMessageBlock>* blocks)
    {
        Author = author;
        RecognizingAuthor = false;
        BlockStarted = false;
        InsideLink = false;
        Blocks = blocks;
        blocks->clear();
    }
    void End()
    {
        for (TVector<TLastMessageBlock>::iterator it = Blocks->begin(); it != Blocks->end(); )
            if (it->IsAuthorship)
                it = Blocks->erase(it);
            else
                ++it;
    }
    static void PostprocessSubforums(TVector<TSubforumInfo>& subforums, time_t date);
    static void PostprocessTopics(TVector<TTopicInfo>& topics, time_t date);
private:
    TUtf16String* Author;
    bool RecognizingAuthor;
    bool BlockStarted;
    bool InsideLink;
    TVector<TLastMessageBlock>* Blocks;
};

class TSubforumRecognizer
{
public:
    void SetDocProps(const IParsedDocProperties* propSet)
    {
        ForumNameRec.SetDocProps(propSet);
    }
    void SetDate(time_t /*date*/)
    {
    }
    void Start(ui32 pos, TSubforumInfo* target)
    {
        target->BeginPos = pos;
        target->EndPos = 0;
        ForumNameRec.Start(&target->ForumName, &target->ForumLink, &target->ForumDescription, &target->NumTopics);
        LastMsgRec.Start(&target->LastMessageAuthor, &target->LastMessageBlocks);
        NumTopicsRec.Start(&target->NumTopics);
        NumMessagesRec.Start(&target->NumMessages);
        NumTopicsAndMessagesRec.Start(&target->NumTopics, &target->NumMessages);
    }
    bool End(ui32 pos, TSubforumInfo* target)
    {
        target->EndPos = pos;
        ForumNameRec.End();
        LastMsgRec.End();
        NumTopicsRec.End();
        NumMessagesRec.End();
        NumTopicsAndMessagesRec.End();
        /*
        If the entire row is inside <noindex>, then BeginPos == EndPos,
        this causes ASSERT in text archive writer. Exclude such rows.
        */
        return (!target->ForumName.empty() && pos != target->BeginPos);
    }
    ITableItemProcessor* GetProcessor(ETableStrings from)
    {
        switch (from) {
        case TS_FORUM_NAME:
            return &ForumNameRec;
        case TS_LAST_MESSAGE:
        case TS_LAST_MESSAGE2:
            return &LastMsgRec;
        case TS_NUM_TOPICS:
            return &NumTopicsRec;
        case TS_NUM_MESSAGES:
        case TS_NUM_MESSAGES2:
            return &NumMessagesRec;
        case TS_TWO_NUMBERS:
        case TS_TWO_NUMBERS2:
            return &NumTopicsAndMessagesRec;
        default:
            return nullptr; // title from topic info?
        }
    }
private:
    TForumNameRecognizer ForumNameRec;
    TLastMessageRecognizer LastMsgRec;
    TNumberRecognizer NumTopicsRec, NumMessagesRec;
    TTwoNumbersRecognizer NumTopicsAndMessagesRec;
};

class TTopicNameRecognizer : public ITableItemProcessor
{
public:
    TTopicNameRecognizer()
        : PropSet(nullptr)
    {}
    void SetDocProps(const IParsedDocProperties* propSet)
    {
        PropSet = propSet;
    }
    void SetDate(time_t date)
    {
        DateRec.SetDate(date);
    }
    void Start(TUtf16String* name, TUtf16String* link, TUtf16String* author, TRecognizedDate* date)
    {
        TopicName = name;
        TopicLink = link;
        TopicAuthor = author;
        Date = date;
        DateRec.Clear();
        Active = nullptr;
        CurPage = 0;
        PagerClose = 0;
        InsideLink = false;
        PagerState = NoPager;
        AdState = MaybeAd;
    }
    void End()
    {
        TRecognizedDate date;
        if (DateRec.GetDate(&date))
            *Date = date;
    }
    void OnText(const wchar16* text, size_t len) override;
    void OnLinkTag(bool opened, const THtmlChunk* chunk) override;
private:
    const IParsedDocProperties* PropSet;
    TUtf16String* TopicName;
    TUtf16String* TopicLink;
    TUtf16String* TopicAuthor;
    TUtf16String* Active;
    TRecognizedDate* Date;
    TForumDateRecognizer DateRec;
    unsigned CurPage;
    wchar16 PagerClose;
    bool InsideLink;
    enum { NoPager, MaybePager, InPager, InUnclosedPager } PagerState;
    enum { NoAd, MaybeAd, IsAd } AdState;
};

class TTopicRecognizer
{
public:
    void SetDocProps(const IParsedDocProperties* propSet)
    {
        TopicNameRec.SetDocProps(propSet);
    }
    void SetDate(time_t date)
    {
        TopicNameRec.SetDate(date);
    }
    void Start(ui32 pos, TTopicInfo* target)
    {
        target->BeginPos = pos;
        target->EndPos = 0;
        Topic = target;
        HadAuthor = false;
        TopicNameRec.Start(&target->Title, &target->Link, &target->Author, &target->CreationDate);
        LastMsgRec.Start(&target->LastMessageAuthor, &target->LastMessageBlocks);
        NumMessagesRec.Start(&target->NumMessages);
        NumViewsRec.Start(&target->NumViews);
        NumMessagesAndViewsRec.Start(&target->NumMessages, &target->NumViews);
        AuthorRec.Start(&target->Author);
    }
    bool End(ui32 pos, TTopicInfo* target)
    {
        target->EndPos = pos;
        TopicNameRec.End();
        LastMsgRec.End();
        NumMessagesRec.End();
        NumViewsRec.End();
        NumMessagesAndViewsRec.End();
        AuthorRec.End();
        // author column in InvisionPB can contain only avatar with actual author in topic description
        if (HadAuthor && target->Author.empty())
            target->Description.swap(target->Author);
        // descriptions in ExBB start with >>
        if (!target->Description.empty() && target->Description[0] == 0xBB)
            target->Description.erase(0, 1);
        return (!target->Title.empty() && pos != target->BeginPos);
    }
    ITableItemProcessor* GetProcessor(ETableStrings from)
    {
        switch (from) {
        case TS_TOPIC_NAME:
            return &TopicNameRec;
        case TS_LAST_MESSAGE:
        case TS_LAST_MESSAGE2:
            return &LastMsgRec;
        case TS_NUM_MESSAGES:
        case TS_NUM_MESSAGES2:
            return &NumMessagesRec;
        case TS_NUM_VIEWS:
            return &NumViewsRec;
        case TS_TWO_NUMBERS:
        case TS_TWO_NUMBERS2:
            return &NumMessagesAndViewsRec;
        case TS_AUTHOR:
            if (!HadAuthor)
                Topic->Description.swap(Topic->Author);
            HadAuthor = true;
            return &AuthorRec;
        default:
            return nullptr; // title from subforum info?
        }
    }
private:
    TTopicInfo* Topic;
    bool HadAuthor;
    TTopicNameRecognizer TopicNameRec;
    TLastMessageRecognizer LastMsgRec;
    TNumberRecognizer NumMessagesRec, NumViewsRec;
    TTwoNumbersRecognizer NumMessagesAndViewsRec;
    TStringRecognizer AuthorRec;
};

/*
    There are two independent mechanisms for detection of subforum/topic tables.
    The first mechanism (TTablesExtractor) tries to be generic for all <table>-based markups;
    it analyzes text of first row of every <table>.
    The second mechanism (TSpecificTablesExtractor) handles non-<table>-based markups
    (<div>/<ul>/<li> which are properly placed using CSS) and uses concrete tags from tags.cpp.
    Both use classes above for recognizing content in individual items.
*/

class TCommonTablesExtractor
{
public:
    TCommonTablesExtractor(TTagsProcessor* tagsProcessor)
        : TagsProcessor(tagsProcessor)
    {}
    void SetDocProps(const IParsedDocProperties* propSet)
    {
        PropSet = propSet;
        SubforumRec.SetDocProps(propSet);
        TopicRec.SetDocProps(propSet);
    }
    void SetDate(time_t date, bool isDateAmericanHint)
    {
        Date = date;
        SubforumRec.SetDate(date);
        TopicRec.SetDate(date);
        IsDateAmericanHint = isDateAmericanHint;
    }
    void PostProcess();
    void OnTextChunk(const THtmlChunk* chunk)
    {
        if (IgnoreBlock || !CurProcessor)
            return;
        unsigned leng = chunk->leng;
        if (!leng)
            return;
        TTempArray<wchar16> unicodeTempBuf(leng * 2);
        wchar16* buf = unicodeTempBuf.Data();
        size_t bufLen = HtEntDecodeToChar(PropSet->GetCharset(), chunk->text, leng, buf);
        Y_ASSERT(bufLen <= leng * 2);
        if (bufLen && !IsSpace(buf, bufLen))
            CurProcessor->OnText(buf, bufLen);
    }

    TVector<TSubforumInfo> SubForums;
    TVector<TTopicInfo> Topics;

protected:
    TTagsProcessor* TagsProcessor;
    const IParsedDocProperties* PropSet;
    ITableItemProcessor* CurProcessor;
    TSubforumRecognizer SubforumRec;
    TTopicRecognizer TopicRec;
    time_t Date;
    bool IsDateAmericanHint;
    bool IgnoreBlock;
};

class TTablesExtractor : public TCommonTablesExtractor
{
public:
    TTablesExtractor(TTagsProcessor* tagsProcessor)
        : TCommonTablesExtractor(tagsProcessor)
    {}
    void Clear();
    void OnOpenTag(const THtmlChunk* chunk, const TNumerStat& numerStat);
    void OnCloseTag(const THtmlChunk* chunk, const TNumerStat& numerStat);
    void OnTokenStart(const TWideToken& token, const TNumerStat& /*numerStat*/)
    {
        if (!FSMActive)
            return;
        TTableHeaderRecognizer::EContext context = IsSubforum() ? TTableHeaderRecognizer::InSubforum :
            (IsTopic() ? TTableHeaderRecognizer::InTopic : TTableHeaderRecognizer::Unknown);
        HeaderRec.RunFSM(token.Token, token.Token + token.Leng, context);
    }
    void OnSpaces(const wchar16* token, unsigned len, const TNumerStat& /*numerStat*/)
    {
        if (!FSMActive)
            return;
        TTableHeaderRecognizer::EContext context = IsSubforum() ? TTableHeaderRecognizer::InSubforum :
            (IsTopic() ? TTableHeaderRecognizer::InTopic : TTableHeaderRecognizer::Unknown);
        HeaderRec.RunFSM(token, token + len, context);
    }

private:
    TTableHeaderRecognizer HeaderRec;
    bool FSMActive;
    bool InsideHead, InsideBody;
    bool SkipThisRow;

    std::pair<int, int> Columns[TS_COUNT];

    int CurColumn, CurColSpan;
    int Depth;  // sometimes table items with forum names contain another tables :(

    bool FindColumn(ETableStrings& coltype);
    // If both TS_FORUM_NAME and TS_TOPIC_NAME are present, assume topic, not subforum.
    bool IsSubforum() const
    {
        return Columns[TS_FORUM_NAME].first >= 0 && Columns[TS_TOPIC_NAME].first < 0;
    }
    bool IsTopic() const
    {
        return Columns[TS_TOPIC_NAME].first >= 0;
    }

    void StartTable();
    void EndTable();
    void StartHeaderRow();
    void EndHeaderRow();
    void StartSubforumRow(ui32 pos);
    void EndSubforumRow(ui32 pos);
    void StartTopicRow(ui32 pos);
    void EndTopicRow(ui32 pos);
    void StartHeaderItem();
    void EndHeaderItem();
    void StartSubforumItem();
    void EndSubforumItem();
    void StartTopicItem();
    void EndTopicItem();
};

class TSpecificTablesExtractor : public TCommonTablesExtractor
{
public:
    TSpecificTablesExtractor(TTagsProcessor* tagsProcessor)
        : TCommonTablesExtractor(tagsProcessor)
    { }
    void RegisterCallbacks();
    void InitForMarkup();
    void Clear();

    void OnOpenTag(const THtmlChunk* chunk, const TNumerStat& /*numerStat*/)
    {
        if (*chunk->Tag == HT_A && CurProcessor)
            CurProcessor->OnLinkTag(true, chunk);
        else if (CurProcessor) {
            if (*chunk->Tag == HT_BR) {
                // treat <br> as '\n' character
                static const wchar16 newline[1] = {'\n'};
                CurProcessor->OnText(newline, 1);
            }
            if (chunk->Tag->is(HT_br) || *chunk->Tag == HT_CITE || *chunk->Tag == HT_SPAN)
                CurProcessor->EndNestedWord();
        }
    }
    void OnCloseTag(const THtmlChunk* chunk, const TNumerStat& /*numerStat*/)
    {
        if (*chunk->Tag == HT_A && CurProcessor)
            CurProcessor->OnLinkTag(false, chunk);
        else if (CurProcessor) {
            if (chunk->flags.brk >= BREAK_PARAGRAPH)
                CurProcessor->EndNestedItem();
            if ((chunk->Tag->is(HT_br) || *chunk->Tag == HT_CITE || *chunk->Tag == HT_SPAN) && CurProcessor)
                CurProcessor->EndNestedWord();
        }
    }

private:
    void OnTableTransition(bool entered, const TNumerStat& pos, bool forTopic);
    static void OnSubforumTransition(void* param, bool entered, const TNumerStat& pos, TPostDescriptor*&)
    {
        ((TSpecificTablesExtractor*)param)->OnTableTransition(entered, pos, false);
    }
    static void OnTopicTransition(void* param, bool entered, const TNumerStat& pos, TPostDescriptor*&)
    {
        ((TSpecificTablesExtractor*)param)->OnTableTransition(entered, pos, true);
    }
    static void OnSearchTransition(void* /*param*/, bool /*entered*/, const TNumerStat& /*pos*/, TPostDescriptor*&)
    {
    }

    static void OnTableItemTransition(void* param, bool entered, const TNumerStat&, TPostDescriptor*&);

    bool InTopic;

    struct THandlerData
    {
        TSpecificTablesExtractor* Parent;
        ETableStrings Id;
    };
    THandlerData HandlersData[TS_COUNT];
};

} // namespace NForumsImpl
