#pragma once
#include "forums.h"
#include "forums_fsm.h"

namespace NForumsImpl {

struct TPostDescriptor;

typedef int THandlerId;

extern const int NumKnownMarkups, NumPossibleBreadcrumbs, NumPossibleTitles;
const int MaxDateChains = 3;

const int HANDLER_POST = 0;
const int HANDLER_MESSAGE = HANDLER_POST + 1;
const int HANDLER_MESSAGE_ALT = HANDLER_MESSAGE + 1;
const int HANDLER_DATE = HANDLER_MESSAGE_ALT + 1;
const int HANDLER_AUTHOR = HANDLER_DATE + MaxDateChains;
const int HANDLER_AUTHOR_ALT = HANDLER_AUTHOR + 1;
const int HANDLER_QUOTE_HEADER = HANDLER_AUTHOR_ALT + 1;
const int HANDLER_QUOTE_BODY = HANDLER_QUOTE_HEADER + 1;
const int HANDLER_NESTED_QUOTE_HEADER = HANDLER_QUOTE_BODY + 1;
const int HANDLER_NESTED_QUOTE_BODY = HANDLER_NESTED_QUOTE_HEADER + 1;
const int HANDLER_SIGNATURE = HANDLER_NESTED_QUOTE_BODY + 1;
const int HANDLER_MARKUP = HANDLER_SIGNATURE + 1;
const int HANDLER_BREADCRUMB_SPECIFIC = HANDLER_MARKUP + NumKnownMarkups;
const int HANDLER_TITLE = HANDLER_BREADCRUMB_SPECIFIC + NumPossibleBreadcrumbs;
const int HANDLER_SUBFORUM = HANDLER_TITLE + NumPossibleTitles;
const int HANDLER_TOPIC = HANDLER_SUBFORUM + 1;
const int HANDLER_SEARCH = HANDLER_TOPIC + 1;
const int HANDLER_INFO_START = HANDLER_SEARCH + 1;
const int NUM_HANDLERS = HANDLER_INFO_START + TS_COUNT;

const int HANDLER_START_POST_RELATED = HANDLER_MESSAGE;
const int HANDLER_END_POST_RELATED = HANDLER_AUTHOR_ALT + 1;

extern const TTagDescriptor HrefTag;

struct TMarkup
{
    TForumsHandler::EGenerator Generator;
    TTagChainDescriptor IdentTag;       // if this tag (possibly nested) is present, assume that this markup is used
    // description of topic pages
    TTagChainDescriptor PostEntireTag;  // tag (possibly nested) with content = entire post
    TTagChainDescriptor PostMessageTag; // tag (possibly nested) with content = message
    TTagChainDescriptor PostMessageTagAlternative;
    TTagChainDescriptor PostDateTag[MaxDateChains]; // tag (possibly nested) with content = post date
    TTagChainDescriptor PostAuthorTag;  // tag (possibly nested) with content = post author
    TTagChainDescriptor PostAuthorTagAlternative;
    TTagChainDescriptor QuoteHeaderTag;
    TTagChainDescriptor QuoteBodyTag;
    TTagChainDescriptor SignatureTag;
    bool SignatureInsideMessage;
    bool QuoteOutsideMessage;
    bool QuoteMetaInsideBody;
    bool QuoteHeaderInsideBody;
    const char* AnchorPrefix;
    const char* SidParameterName;
    // description of forum pages / forum lists
    TTagChainDescriptor SubforumTag;
    TTagChainDescriptor TopicTag;
    TTagChainDescriptor SearchTag;
    struct
    {
        TTagChainDescriptor Tag;
        ETableStrings Id;
        bool ForForum, ForTopic;
    } TableItemTags[TS_COUNT];
};

struct THandlerInfo;
typedef TList<THandlerInfo*> THandlersList;

struct THandlerInfoState
{
    TTagChainTracker Tracker;
    HT_TAG CurOpen, CurClose;
};

struct THandlerInfo
{
    typedef void (*THandler)(void* param, bool entered, const TNumerStat& pos, TPostDescriptor*& lastPost);
    THandler Handler;
    void* Param;
    TTagChainTracker Tracker;
    HT_TAG CurOpen, CurClose;
    THandlersList::iterator IndexInOpen, IndexInClose;
};

class TTagsProcessor
{
private:
    THandlersList OpenTagHandlers[HT_TagCount], CloseTagHandlers[HT_TagCount];
    TArrayHolder<THandlerInfo> HandlersInfo;
    const TMarkup* ChosenMarkup;
    THandlersList::iterator CurrentIter;
    enum { ENothing, EOpen, EClose } CurrentIterType;

    void ChangeOpenTag(THandlerInfo* h, HT_TAG tag, bool addToTail = true)
    {
        Y_ASSERT((unsigned)tag <= HT_TagCount);
        if (h->CurOpen == tag)
            return;
        if (h->CurOpen != HT_TagCount) {
            if (CurrentIterType == EOpen && CurrentIter == h->IndexInOpen)
                ++CurrentIter;
            OpenTagHandlers[h->CurOpen].erase(h->IndexInOpen);
        }
        h->CurOpen = tag;
        if (tag != HT_TagCount) {
            h->IndexInOpen = OpenTagHandlers[tag].insert(addToTail ? OpenTagHandlers[tag].end() : OpenTagHandlers[tag].begin(), h);
            if (addToTail && CurrentIterType == EOpen && CurrentIter == OpenTagHandlers[tag].end())
                CurrentIter = h->IndexInOpen;
        }
    }
    void ChangeCloseTag(THandlerInfo* h, HT_TAG tag, bool addToTail = true)
    {
        Y_ASSERT((unsigned)tag <= HT_TagCount);
        if (h->CurClose == tag)
            return;
        if (h->CurClose != HT_TagCount) {
            if (CurrentIterType == EClose && CurrentIter == h->IndexInClose)
                ++CurrentIter;
            CloseTagHandlers[h->CurClose].erase(h->IndexInClose);
        }
        h->CurClose = tag;
        if (tag != HT_TagCount) {
            h->IndexInClose = CloseTagHandlers[tag].insert(addToTail ? CloseTagHandlers[tag].end() : CloseTagHandlers[tag].begin(), h);
            if (addToTail && CurrentIterType == EClose && CurrentIter == CloseTagHandlers[tag].end())
                CurrentIter = h->IndexInClose;
        }
    }

public:
    TTagsProcessor()
        : HandlersInfo(new THandlerInfo[NUM_HANDLERS])
        , ChosenMarkup(nullptr)
        , CurrentIterType(ENothing)
    {
    }
    void ProcessOpenTag(HT_TAG tag, const THtmlChunk* chunk, int depth, const TNumerStat& pos, TPostDescriptor* lastPost);
    void ProcessCloseTag(HT_TAG tag, const THtmlChunk* chunk, int depth, const TNumerStat& pos, TPostDescriptor* lastPost);
    void Assign(THandlerId handler, THandlerInfo::THandler func, void* param)
    {
        THandlerInfo& h = HandlersInfo[handler];
        h.Handler = func;
        h.Param = param;
    }
    void Start(THandlerId handler, bool addToTail = true)
    {
        ChangeOpenTag(handler, HandlersInfo[handler].Tracker.GetFirstTag(), addToTail);
    }
    void Restart(THandlerId handler, bool addToTail = true)
    {
        HandlersInfo[handler].Tracker.Restart();
        Start(handler, addToTail);
    }
    void Stop(THandlerId handler)
    {
        ChangeOpenTag(handler, HT_TagCount);
        ChangeCloseTag(handler, HT_TagCount);
        HandlersInfo[handler].Tracker.Restart();
    }
    void SetAssociatedChain(THandlerId handler, const TTagChainDescriptor* associated)
    {
        HandlersInfo[handler].Tracker.SetAssociatedChain(associated);
    }
    void Init();
    void SaveHandlerState(THandlerId handler, THandlerInfoState* state) const
    {
        state->Tracker = HandlersInfo[handler].Tracker;
        state->CurOpen = HandlersInfo[handler].CurOpen;
        state->CurClose = HandlersInfo[handler].CurClose;
    }
    void RestoreHandlerState(THandlerId handler, const THandlerInfoState* state, bool addToTail = true)
    {
        Stop(handler);
        HandlersInfo[handler].Tracker = state->Tracker;
        ChangeOpenTag(handler, state->CurOpen, addToTail);
        ChangeCloseTag(handler, state->CurClose, addToTail);
    }
    void ChangeOpenTag(THandlerId handler, HT_TAG tag, bool addToTail = true)
    {
        ChangeOpenTag(&HandlersInfo[handler], tag, addToTail);
    }
    void ChangeCloseTag(THandlerId handler, HT_TAG tag, bool addToTail = true)
    {
        ChangeCloseTag(&HandlersInfo[handler], tag, addToTail);
    }
    bool IsInZone(THandlerId handler) const
    {
        return (bool)HandlersInfo[handler].Tracker;
    }
    const TMarkup* GetMarkup() const
    {
        return ChosenMarkup;
    }
};

} // namespace NForumsImpl
