#pragma once

#include <library/cpp/token/token_structure.h>
#include <util/memory/segmented_string_pool.h>
#include <util/generic/vector.h>
#include <util/generic/maybe.h>

struct TNumerStat;
struct THtmlChunk;
class IParsedDocProperties;

namespace NForumsImpl {

class TTagsProcessor;
struct TPostDescriptor;
struct TQuoteDescriptor;
struct THandlerInfoState;


class TPageNumbers
{
private:
    int cs; // Ragel variable
    int TempNumber; // temporary number which can become e.g. NumPages if the following word is 'pages'
    int ThisPage, NumPages, NumMessages;
    int ThisStartMsg, ThisEndMsg;
    bool Active;
    bool ThisPageIsReliable, NumPagesIsReliable;
public:
    TPageNumbers()
    {
        Clear();
    }
    void Clear();
    void BreakParagraph();
    void ProcessToken(const wchar16* p, const wchar16* pe);
    void Stop()
    {
        Active = false;
    }
    int GetPage() const
    {
        return ThisPage;
    }
    int GetNumPages() const
    {
        return NumPages;
    }
    int GetNumMessages() const
    {
        return NumMessages;
    }
};

class TMessages
{
private:
    int cs; // Ragel variable; not valid if ActiveQuote == 0 or ParsingQuoteMeta == false
    bool ParsingQuoteHeader;    // not valid if ActiveQuote == 0
    bool ParsingQuoteAuthor;    // not valid if ActiveQuote == 0
    bool ParsingQuoteMeta;      // metainformation inside body; not valid if ActiveQuote == 0
    bool ExpectingUrl;      // not valid if ActiveQuote == 0
    bool HeaderOk;          // not valid if ActiveQuote == 0
    TUtf16String CurText;
    TQuoteDescriptor* ActiveQuote;
    TVector<TQuoteDescriptor*> ActiveQuoteStack;
    segmented_pool<wchar16>& StringsPool;
    TTagsProcessor* TagsProcessor;
    TVector<THandlerInfoState> QuoteParseStack;

    void EnterQuote(ui32 pos, TPostDescriptor* lastPost);
    void LeaveQuote(ui32 pos);
    void InitFSM();
    const wchar16* RunFSM(const wchar16* p, const wchar16* pe);
    bool FSMDecidedStop() const;
    bool FSMDecidedAuthor() const;
    bool ProcessHeader();
    void RecognizeQuoteDate(TWtringBuf str);
    bool HeaderFSM(const wchar16* p, const wchar16* pe, TWtringBuf& meta);
    void ProcessText(const wchar16* token, unsigned len, const TNumerStat& pos);

public:
    const IParsedDocProperties* PropSet;

    static void OnQuoteHeaderTransition(void* param, bool entered, const TNumerStat& pos, TPostDescriptor*& lastPost);
    static void OnQuoteBodyTransition(void* param, bool entered, const TNumerStat& pos, TPostDescriptor*& lastPost);
    static void OnSignatureTransition(void* param, bool entered, const TNumerStat& pos, TPostDescriptor*& lastPost);
    void TerminateQuote(const TNumerStat& pos);

    TMessages(segmented_pool<wchar16>& stringsPool, TTagsProcessor* tagsProcessor);
    ~TMessages();
    void Clear();

    void OnTokenStart(const TWideToken& token, const TNumerStat& pos)
    {
        ProcessText(token.Token, token.Leng, pos);
    }
    void OnSpaces(const wchar16* token, unsigned len, const TNumerStat& pos)
    {
        ProcessText(token, len, pos);
    }
    void OnOpenTag(const THtmlChunk* chunk, const TNumerStat& numerStat);
    void OnCloseTag(const THtmlChunk* chunk, const TNumerStat& numerStat);
};

enum ETableStrings {
    // subforums
    TS_FORUM_NAME,
    TS_LAST_MESSAGE,
    TS_NUM_TOPICS,
    TS_NUM_MESSAGES,
    TS_TWO_NUMBERS,
    // topics
    TS_TOPIC_NAME,
    TS_NUM_MESSAGES2,   // same as TS_NUM_MESSAGES
    TS_NUM_VIEWS,
    TS_LAST_MESSAGE2,   // same as TS_LAST_MESSAGE; some markups use different style for subforums and topics
    TS_TWO_NUMBERS2,    // same as TS_TWO_NUMBERS
    TS_AUTHOR,
    // total
    TS_COUNT,
    // special
    TS_SIMILAR_TOPICS,
};

class TTableHeaderRecognizer
{
public:
    void InitFSM();
    enum EContext
    {
        Unknown,
        InSubforum,
        InTopic
    };
    void RunFSM(const wchar16* p, const wchar16* pe, EContext context);

    bool Recognized() const;
    ETableStrings GetResult() const
    {
        return Result;
    }

private:
    int cs; // Ragel variable
    ETableStrings Result;

};

class TTwoNumbersFSM
{
public:
    void InitFSM();
    void RunFSM(const wchar16* p, const wchar16* pe);
    bool Recognized() const;
    i64 Number1, Number2;

private:
    int cs;
    i64 Number1sp, Number2sp;
};

const wchar16* FindEndOfDescription(const wchar16* p, const wchar16* pe);
const wchar16* IsNumTopicsPrefix(const wchar16* p, const wchar16* pe, int* numTopics);
const wchar16* IsAuthorshipPrefixLastMsg(const wchar16* p, const wchar16* pe);
const wchar16* IsAuthorshipPrefixTopic(const wchar16* p, const wchar16* pe);
bool IsAdPrefixTopic(const wchar16* p, const wchar16* pe);

inline bool IsArrowCharacter(wchar16 c)
{
    return (c == '>' || c == 0xAB || c == 0xBB || c == 0x203A || c == 0x2039 || c == 0x2192);
}

class TSpecialDateRecognizer
{
public:
    void InitFSM();
    void RunFSM(const wchar16* p, const wchar16* pe);
    bool Recognized() const;
    int GetDeltaSeconds() const
    { return DeltaSeconds; }
private:
    int cs; // Ragel variable
    int DeltaSeconds;
    int CurNumber, TempNumber;
};

} // namespace NForumsImpl
