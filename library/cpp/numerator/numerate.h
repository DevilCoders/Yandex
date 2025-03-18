#pragma once

#include "decoder.h"

#include <library/cpp/html/face/parsface.h>
#include <library/cpp/html/face/zoneconf.h>
#include <library/cpp/html/face/zonecount.h>
#include <library/cpp/html/storage/queue.h>
#include <library/cpp/wordpos/wordpos.h>
#include <library/cpp/langs/langs.h>
#include <library/cpp/token/nlptypes.h>
#include <library/cpp/token/token_structure.h>

#include <util/generic/string.h>

struct TNumerStat {
    TWordPosition TokenPos;
    ui32 WordCount;
    TEXT_WEIGHT CurWeight;
    TIrregTag CurIrregTag;
    ui32 InputPosition;
    bool PosOK;

    TNumerStat(ui32 startDoc = 0, ui32 startBreak = 1, ui32 startWord = 1)
        : TokenPos(TWordPosition(startDoc, startBreak, startWord))
        , WordCount(0)
        , CurWeight(WEIGHT_ZERO)
        , CurIrregTag(IRREG_none)
        , InputPosition(0)
        , PosOK(true)
    {
    }

    inline bool IsOverflow() const {
        return !PosOK;
    }
};

class IParsEventHandler // bad
{
protected:
    virtual ~IParsEventHandler() {
    }

public:
    virtual void OnTextStart(const IParsedDocProperties*) {
    }
    virtual void OnTextEnd(const IParsedDocProperties*, const TNumerStat&) {
    }
    virtual void OnParagraphEnd() {
    }
    virtual void OnAddEvent(const THtmlChunk&) {
    }
    virtual void OnRemoveEvent(const THtmlChunk&) {
    }
    virtual void OnTokenBegin(const THtmlChunk&, unsigned /*offsetInEv*/) {
    }
    virtual void OnTokenEnd(const THtmlChunk&, unsigned /*offsetInEv*/) {
    }
    virtual void OnMoveInput(const THtmlChunk&, const TZoneEntry*, const TNumerStat&) {
    }
};

class ITokenizerHandler {
protected:
    virtual ~ITokenizerHandler() {
    }

public:
    virtual void OnTokenStart(const TWideToken&, const TNumerStat&) {
    }
    virtual void OnSpaces(TBreakType, const wchar16*, unsigned /*length*/, const TNumerStat&) {
    }
};

class INumeratorHandler: public IParsEventHandler, public ITokenizerHandler {
};

class Numerator: private TNonCopyable {
    class TSeqTokenizer;
    class TParsEventSeq;

public:
    struct TConfig {
        ui32 StartDoc = 0;
        ui32 StartBreak = 1;
        ui32 StartWord = 1;
        bool BackwardCompatible = false;

        TConfig() {
        }

        explicit TConfig(bool backwardCompatible)
            : BackwardCompatible(backwardCompatible)
        {
        }
    };

    explicit Numerator(INumeratorHandler& handler, const TConfig& config = TConfig());
    virtual ~Numerator();

    bool DocFormatOK() const {
        return workOK;
    }

    const TString& GetParseError() const {
        return ParseError;
    }

    size_t GetFlushedLeng() const {
        return FlushedLeng;
    }

    const TNumerStat& GetStat() const {
        return Stat;
    }

    void Numerate(NHtml::TSegmentedQueueIterator first, NHtml::TSegmentedQueueIterator last,
                  IParsedDocProperties* docProps, IZoneAttrConf* config);

protected:
    void NumerateEvents(NHtml::TSegmentedQueueIterator first, NHtml::TSegmentedQueueIterator last,
                        IParsedDocProperties* docProps, IZoneAttrConf* config, IDecoder* decoder);

private:
    void AnalyzeParserEvents(const THtmlChunk** beg, size_t size, SPACE_MODE spaceMode, IDecoder* decoder);
    void DiscardInput(const THtmlChunk& chunk, IDecoder* decoder);
    // returns 'true' if we should continue
    bool ProcessEvent(TParsEventSeq* eventSeq, const THtmlChunk* chunk, IDecoder* decoder);
    void RemoveEvent(const THtmlChunk& e);

private:
    TNumerStat Stat;
    INumeratorHandler& Handler;
    size_t FlushedLeng;
    ui32 InputPosition;
    int NextSpaceType;
    ELanguage Language;
    bool BackwardCompatible;
    bool IgnorePunctBreaks;
    bool workOK;
    THolder<TZoneCounter> ZoneCounter;
    TString ParseError;
};

class TNumeratorHandlersNoEvents: public INumeratorHandler, private TNonCopyable {
protected:
    typedef TVector<INumeratorHandler*> THandlers;
    THandlers Handlers;

public:
    void AddHandler(INumeratorHandler* handler) {
        Handlers.push_back(handler);
    }
    void Clear() {
        Handlers.clear();
    }
    void OnTextStart(const IParsedDocProperties* p) override {
        for (auto handler : Handlers)
            handler->OnTextStart(p);
    }
    void OnTextEnd(const IParsedDocProperties* p, const TNumerStat& stat) override {
        for (auto handler : Handlers)
            handler->OnTextEnd(p, stat);
    }
    void OnParagraphEnd() override {
        for (auto handler : Handlers)
            handler->OnParagraphEnd();
    }
    void OnMoveInput(const THtmlChunk& chunk, const TZoneEntry* ze, const TNumerStat& stat) override {
        for (auto handler : Handlers)
            handler->OnMoveInput(chunk, ze, stat);
    }

    void OnTokenStart(const TWideToken& tok, const TNumerStat& stat) override {
        for (auto handler : Handlers)
            handler->OnTokenStart(tok, stat);
    }

};

class TNumeratorHandlers: public TNumeratorHandlersNoEvents {
public:
    void OnAddEvent(const THtmlChunk& e) override {
        for (auto handler : Handlers)
            handler->OnAddEvent(e);
    }
    void OnRemoveEvent(const THtmlChunk& e) override {
        for (auto handler : Handlers)
            handler->OnRemoveEvent(e);
    }
    void OnSpaces(TBreakType type, const wchar16* val, unsigned len, const TNumerStat& stat) override {
        for (auto handler : Handlers)
            handler->OnSpaces(type, val, len, stat);
    }

    void OnTokenBegin(const THtmlChunk& chunk, unsigned offsetInEv) override {
        for (auto handler : Handlers)
            handler->OnTokenBegin(chunk, offsetInEv);
    }

    void OnTokenEnd(const THtmlChunk& chunk, unsigned offsetInEv) override {
        for (auto handler : Handlers)
            handler->OnTokenEnd(chunk, offsetInEv);
    }

};

bool Numerate(INumeratorHandler& handler, NHtml::TSegmentedQueueIterator first, NHtml::TSegmentedQueueIterator last,
              IParsedDocProperties* docProps, IZoneAttrConf* attrConf, const Numerator::TConfig& config = Numerator::TConfig(false));
