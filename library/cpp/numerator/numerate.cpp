#include "numerate.h"

#include <library/cpp/token/charfilter.h>
#include <library/cpp/tokenizer/tokenizer.h>

#include <library/cpp/charset/codepage.h>
#include <library/cpp/charset/recyr.hh>
#include <util/charset/wide.h>
#include <util/generic/vector.h>
#include <util/memory/tempbuf.h>
#include <util/system/defaults.h>
#include <library/cpp/containers/str_hash/str_hash.h>

static inline void NEXT_WORD(TNumerStat& stat) {
    stat.PosOK = stat.TokenPos.Inc() && stat.PosOK;
    stat.WordCount++;
}

static inline void BREAK_STEP(TNumerStat& stat) {
    stat.PosOK = stat.TokenPos.Bump() && stat.PosOK;
}

struct TZeroChecker {
};

template <>
class TCharFilter<TZeroChecker> {
public:
    explicit TCharFilter(size_t bufSize)
        : BufSize(bufSize)
        , Buffer(nullptr)
        , ZeroCount(0)
    {
    }

    ~TCharFilter() {
        if (Buffer)
            Buffer->~TCharTemp();
    }

    TWtringBuf Replace(const wchar16* token, size_t len) {
        for (const wchar16 *p = token, *e = token + len; p != e; ++p) {
            if (*p == 0) {
                Buffer = new (Array) TCharTemp(BufSize);
                wchar16* const data = Buffer->Data();
                const size_t n = p - token;
                wchar16* tok;
                if (n) {
                    std::char_traits<wchar16>::copy(data, token, n);
                    tok = data + n;
                } else
                    tok = data;
                *tok++ = 0x0020;
                ZeroCount = 1;
                ++p;
                for (; p != e; ++p) {
                    if (*p == 0) {
                        *tok++ = 0x0020;
                        ++ZeroCount;
                    } else
                        *tok++ = *p;
                }
                return TWtringBuf(data, tok - data);
            }
        }
        return TWtringBuf(token, len);
    }

    size_t GetZeroCount() const {
        return ZeroCount;
    }

private:
    const size_t BufSize;
    TCharTemp* Buffer;
    size_t Array[sizeof(TCharTemp) / sizeof(size_t) + 1];
    size_t ZeroCount;
};

class Numerator::TParsEventSeq {
public:
    TParsEventSeq(Numerator* numer, IParsedDocProperties* pd, IParsEventHandler* handler, TNumerStat* stat)
        : Numer(numer)
        , ParsProp(pd)
        , Handler(handler)
        , Stat(stat)
        , Space(SPACE_DEFAULT)
    {
        Handler->OnTextStart(ParsProp);
    }

    ~TParsEventSeq() {
        try {
            FlushEvents();
            Handler->OnTextEnd(ParsProp, *Stat);
        } catch (...) {
        }
    }

    void AddEvent(const THtmlChunk* chunk) {
        Handler->OnAddEvent(*chunk);
        Seq.push_back(chunk);
        if (IsUsefulText(chunk)) {
            Space = (SPACE_MODE)chunk->flags.space;
        }
    }

    void FlushEvents() {
        if (Seq.empty()) {
            return;
        }

        size_t k = 0;
        try {
            for (k = 0; k < Seq.size(); ++k) {
                const THtmlChunk* chunk = Seq[k];
                Handler->OnRemoveEvent(*chunk);
                Numer->RemoveEvent(*chunk);
            }
        } catch (const yexception&) {
            if (k > 0)
                Seq.erase(Seq.begin(), Seq.begin() + k);
            throw;
        }
        Seq.clear();
        Handler->OnParagraphEnd();
        //        Space = SPACE_DEFAULT;
    }

    const THtmlChunk** Begin() {
        return Seq.data();
    }

    size_t Size() const {
        return Seq.size();
    }

    SPACE_MODE GetSpaceMode() const {
        return Space;
    }

private:
    Numerator* Numer;
    IParsedDocProperties* ParsProp;
    IParsEventHandler* Handler;
    TNumerStat* Stat;
    TVector<const THtmlChunk*> Seq;
    SPACE_MODE Space;
};

#include <util/stream/file.h>

class Numerator::TSeqTokenizer: public ITokenHandler {
public:
    TSeqTokenizer(Numerator* numer, IDecoder* decoder, const THtmlChunk** begInp, const THtmlChunk** endInp, bool backwardCompatible)
        : Numer(numer)
        , Decoder(decoder)
        , CurInput(begInp)
        , EndInput(endInp)
        , TokenStart(0)
        , InputStart(0)
        , TextLengthIndex(0)
        , BackwardCompatible(backwardCompatible)
    {
        Y_ASSERT(Numer);
        Y_ASSERT(Decoder);
        Y_ASSERT(CurInput != EndInput);

        Numer->Stat.CurWeight = (TEXT_WEIGHT)(*CurInput)->flags.weight;
        Numer->Stat.CurIrregTag = (*CurInput)->Format;
    }

    void Tokenize(bool spacePreserve, ELanguage language) {
        size_t textLen = 0;
        // Calculate length of useful text to decode.
        for (const THtmlChunk** it = CurInput; it < EndInput; ++it) {
            if (IsUsefulText(*it))
                textLen += (*it)->leng;
            else if (IsWordBreak(*it))
                textLen += 1;
        }
        // Decode useful text.
        TCharTemp buf(Decoder->GetDecodeBufferSize(textLen));
        wchar16* text = buf.Data();
        wchar16* curPos = text;
        for (const THtmlChunk** it = CurInput; it < EndInput; ++it) {
            if (IsUsefulText(*it)) {
                const size_t tLen = Decoder->DecodeEvent(**it, curPos);
                Y_ASSERT(tLen <= (*it)->leng);
                TextLengths.push_back(std::make_pair(*it, tLen));
                curPos += tLen;
            } else if (IsWordBreak(*it))
                *curPos++ = 0; // characters are added here and must be removed immediately after tokenizing
        }
        *curPos = 0;
        textLen = curPos - text;

        TTokenizerOptions opts;
        opts.LangMask = TLangMask(language);
        opts.SpacePreserve = spacePreserve;
        opts.Version = 3;
        opts.KeepAffixes = true;
        TNlpTokenizer(*this, BackwardCompatible)
            .Tokenize(text, textLen, opts);

        while (CurInput != EndInput)
            MoveInput();
    }

private:
    void OnToken(const TWideToken& wtok, size_t origleng, NLP_TYPE type) override {
        Y_ASSERT(wtok.Token);

        CheckInput();
        size_t zeroCount = 0;
        switch (type) {
            case NLP_WORD:
            case NLP_MARK:
            case NLP_INTEGER:
            case NLP_FLOAT: {
                size_t subTokenCount = wtok.SubTokens.size();
                if (subTokenCount > 1) {
                    // do not set break inside multitoken
                    if (Numer->Stat.TokenPos.Word() + subTokenCount - 1 > WORD_LEVEL_Max) {
                        BREAK_STEP(Numer->Stat);
                    }
                }

                Numer->Stat.InputPosition = Numer->InputPosition + GetCurTokenOffset();
                Numer->Handler.OnTokenBegin(**CurInput, GetCurTokenOffset());
                Numer->Handler.OnTokenStart(wtok, Numer->Stat);
                Numer->NextSpaceType = ST_NOBRK;

                const TCharSpan* begSubTok = wtok.SubTokens.begin();
                const TCharSpan* endSubTok = wtok.SubTokens.end();

                while (InputStart + GetTextLength(*CurInput) < TokenStart + origleng) {
                    while (begSubTok < endSubTok && InputStart >= TokenStart + begSubTok->EndPos()) {
                        NEXT_WORD(Numer->Stat);
                        ++begSubTok;
                    }
                    MoveInput();
                }

                Numer->Handler.OnTokenEnd(**CurInput, unsigned(TokenStart + origleng - InputStart));

                while (begSubTok < endSubTok) {
                    NEXT_WORD(Numer->Stat);
                    ++begSubTok;
                }
            } break;

            case NLP_SENTBREAK:
            case NLP_PARABREAK: {
                TCharFilter<TZeroChecker> f(wtok.Leng);
                const TWtringBuf s = f.Replace(wtok.Token, wtok.Leng);
                zeroCount = f.GetZeroCount();
                if (Numer->IgnorePunctBreaks) {
                    Numer->Handler.OnSpaces(ST_NOBRK | Numer->NextSpaceType, s.data(), (unsigned)s.size(), Numer->Stat);
                } else {
                    Numer->Handler.OnSpaces(GetSpaceType(type) | Numer->NextSpaceType, s.data(), (unsigned)s.size(), Numer->Stat);
                    BREAK_STEP(Numer->Stat);
                }
                Numer->NextSpaceType = ST_NOBRK;
                break;
            }
            default: // NLP_MISCTEXT
            {
                TCharFilter<TZeroChecker> f(wtok.Leng);
                const TWtringBuf s = f.Replace(wtok.Token, wtok.Leng);
                zeroCount = f.GetZeroCount();
                Numer->Handler.OnSpaces(ST_NOBRK | Numer->NextSpaceType, s.data(), (unsigned)s.size(), Numer->Stat);
                Numer->NextSpaceType = ST_NOBRK;
                break;
            }
        }
        // Здесь учитывается тот факт, что на каждый IsWordBreak мы добавляем
        // один символ 0 в исходную последовательность.
        TokenStart += origleng - zeroCount;
    }

    size_t GetTextLength(const THtmlChunk* e) const {
        return (TextLengthIndex < TextLengths.size() && e == TextLengths[TextLengthIndex].first ? TextLengths[TextLengthIndex].second : 0);
    }

    void MoveInput() {
        Y_ASSERT(CurInput != EndInput);
        Numer->DiscardInput(**CurInput, Decoder);
        InputStart += GetTextLength(*CurInput);
        // Go to next chunk
        if (TextLengthIndex < TextLengths.size() && *CurInput == TextLengths[TextLengthIndex].first)
            ++TextLengthIndex;
        ++CurInput;
        if (CurInput != EndInput) {
            Numer->Stat.CurWeight = (TEXT_WEIGHT)(*CurInput)->flags.weight;
            Numer->Stat.CurIrregTag = (*CurInput)->Format;
        }
    }

    void CheckInput() {
        while (CurInput != EndInput && InputStart + GetTextLength(*CurInput) <= TokenStart)
            MoveInput();
    }

    unsigned GetCurTokenOffset() const {
        return unsigned(TokenStart - InputStart);
    }

private:
    Numerator* Numer;
    IDecoder* Decoder;
    const THtmlChunk** CurInput;
    const THtmlChunk** const EndInput;
    size_t TokenStart;
    size_t InputStart;
    size_t TextLengthIndex;
    TVector<std::pair<const THtmlChunk*, size_t>> TextLengths; //!< lengths of decoded text chunks
    const bool BackwardCompatible;
};

Numerator::Numerator(INumeratorHandler& handler, const TConfig& config)
    : Stat(config.StartDoc, config.StartBreak, config.StartWord)
    , Handler(handler)
    , FlushedLeng(0)
    , InputPosition(0)
    , NextSpaceType(ST_NOBRK)
    , Language(LANG_UNK)
    , BackwardCompatible(config.BackwardCompatible)
    , IgnorePunctBreaks(false)
    , workOK(true)
{
}

Numerator::~Numerator() {
}

void Numerator::Numerate(NHtml::TSegmentedQueueIterator first, NHtml::TSegmentedQueueIterator last,
                         IParsedDocProperties* docProps, IZoneAttrConf* config) {
    TNlpInputDecoder decoder(docProps->GetCharset());
    this->NumerateEvents(first, last, docProps, config, &decoder);
}

void Numerator::RemoveEvent(const THtmlChunk& e) {
    if (e.flags.markup != MARKUP_IMPLIED)
        FlushedLeng += e.leng;
}

void Numerator::AnalyzeParserEvents(const THtmlChunk** beg, size_t size, SPACE_MODE spaceMode, IDecoder* decoder) {
    if (size != 0) {
        TSeqTokenizer(this, decoder, beg, beg + size, BackwardCompatible)
            .Tokenize(spaceMode == SPACE_PRESERVE, Language);
    }
}

static const wchar16 nullToken[] = {0};

bool Numerator::ProcessEvent(TParsEventSeq* eventSeq, const THtmlChunk* chunk, IDecoder* decoder) {
    PARSED_TYPE pt = (PARSED_TYPE)chunk->flags.type;

    if (pt <= PARSED_EOF) {
        eventSeq->AddEvent(chunk); // for the last event to be processed!
        AnalyzeParserEvents(eventSeq->Begin(), eventSeq->Size(), eventSeq->GetSpaceMode(), decoder);
        eventSeq->FlushEvents();
        if (pt == PARSED_ERROR) {
            workOK = false;
            ParseError = chunk->text && chunk->leng ? TString(chunk->text, chunk->leng) : TString("unknown error");
        }
        return false;
    }

    if (chunk->flags.brk >= (int)BREAK_PARAGRAPH) {
        AnalyzeParserEvents(eventSeq->Begin(), eventSeq->Size(), eventSeq->GetSpaceMode(), decoder);
        Stat.CurWeight = (TEXT_WEIGHT)chunk->flags.weight;
        Stat.CurIrregTag = chunk->Format;
        Handler.OnSpaces((chunk->flags.brk == BREAK_PARAGRAPH ? ST_PARABRK : ST_SENTBRK), nullToken, 0, Stat);
        BREAK_STEP(Stat);
        eventSeq->FlushEvents();
    }

    eventSeq->AddEvent(chunk); // all events in order
    return true;
}

void Numerator::NumerateEvents(NHtml::TSegmentedQueueIterator first, NHtml::TSegmentedQueueIterator last,
                               IParsedDocProperties* docProps, IZoneAttrConf* config, IDecoder* decoder) {
    Y_ASSERT(docProps);
    if (config != nullptr) {
        ZoneCounter.Reset(new TZoneCounter(config, docProps));
    }

    const char* prop = nullptr;
    if (!docProps->GetProperty(PP_LANGUAGE, &prop))
        Language = LanguageByName(prop);
    else
        Language = LANG_UNK;

    if (!docProps->GetProperty(PP_IGNORE_PUNCT_BREAKS, &prop))
        IgnorePunctBreaks = strcmp(prop, "yes") == 0;
    else
        IgnorePunctBreaks = false;

    bool shouldContinue = true;
    try {
        TParsEventSeq eventSeq(this, docProps, &Handler, &Stat);
        while (shouldContinue && first != last) {
            const THtmlChunk* const chunk = GetHtmlChunk(first);
            ++first;
            shouldContinue = ProcessEvent(&eventSeq, chunk, decoder);
        }
    } catch (const yexception& e) { // don't catch std::bad_alloc
        Cerr << "Numerator::NumerateEvents: " << e.what() << Endl;
        workOK = false;
        ParseError = e.what();
    }
}

void Numerator::DiscardInput(const THtmlChunk& chunk, IDecoder* decoder) {
    if (MARKUP_IMPLIED != chunk.flags.markup)
        InputPosition += chunk.leng;

    // TODO: replace all the code below with call to OnMoveInput(chunk, NULL, stat)
    //       if TNumerSerializer2 works everywhere

    if (!ZoneCounter.Get()) {
        Handler.OnMoveInput(chunk, nullptr, Stat);
        return;
    }

    const int type = chunk.flags.type;

    if (type == PARSED_EOF) {
        // TODO: all zones must be actually closed so all code for PARSED_EOF should be removed
        const HashSet& openZones = ZoneCounter->GetOpenZones();
        if (openZones.empty()) {
            // the call required for several htparser.ini, but later it will be removed
            Handler.OnMoveInput(chunk, nullptr, Stat);
        } else {
            TZoneEntry zone;
            zone.IsClose = true;
            for (const auto& openZone : openZones) {
                zone.Name = openZone.first;
                Handler.OnMoveInput(chunk, &zone, Stat); // does it OK to pass EOF several times?
            }
        }
    } else {
        TZoneEntry zone;
        ZoneCounter->CheckEvent(chunk, &zone);

        if (zone.IsValid()) {
            if (zone.IsOpen && !zone.OnlyAttrs)
                NextSpaceType = ST_ZONEOPN;

            if (zone.IsClose && !zone.NoOpeningTag)
                NextSpaceType = ST_ZONECLS;

            if (zone.Attrs.size()) {
                TCharTemp decodeBuf(GetDecodeBufferSize(zone.Attrs.data(), zone.Attrs.size(), decoder));
                DecodeAttrValues(zone.Attrs.data(), zone.Attrs.size(), decodeBuf.Data(), decoder);
                Handler.OnMoveInput(chunk, &zone, Stat);
            } else {
                Handler.OnMoveInput(chunk, &zone, Stat);
            }
        } else {
            Handler.OnMoveInput(chunk, nullptr, Stat);
        }
    }
}

bool Numerate(INumeratorHandler& handler, NHtml::TSegmentedQueueIterator first, NHtml::TSegmentedQueueIterator last,
              IParsedDocProperties* docProps, IZoneAttrConf* attrConf, const Numerator::TConfig& config) {
    Numerator numerator(handler, config);
    numerator.Numerate(first, last, docProps, attrConf);
    if (!numerator.DocFormatOK()) {
        docProps->SetProperty(PP_INDEXRES, numerator.GetParseError().data());
        return false;
    } else {
        docProps->SetProperty(PP_INDEXRES, "OK");
        return true;
    }
}
