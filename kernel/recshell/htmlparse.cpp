#include "htmlparse.h"

#include <library/cpp/charset/codepage.h>
#include <library/cpp/html/entity/htmlentity.h>
#include <library/cpp/html/face/blob/chunkslist.h>
#include <library/cpp/html/face/onchunk.h>
#include <library/cpp/html/face/parsface.h>
#include <library/cpp/html/html5/parse.h>

#include <util/charset/wide.h>
#include <util/generic/ptr.h>
#include <util/generic/yexception.h>
#include <util/stream/buffer.h>
#include <util/stream/output.h>
#include <util/string/strip.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/string/util.h>

namespace NRecognizerShell {
namespace {

/// Class for finding a specific tag with one specific attribute value.
 /* If the default constructor is called the attributes values are not
  * checked.
  */
template <HT_TAG TagId>
class TTagInteriorDetector {
public:
    TTagInteriorDetector()
        : IsInsideTag(false)
        , TagDepthAfterMain(0)
    {}

    TTagInteriorDetector(const TString& attribute, const TString& value)
        : Attribute(attribute)
        , Value(value)
        , IsInsideTag(false)
        , TagDepthAfterMain(0)
    {}

    void Update(const THtmlChunk& chunk);
    bool IsInside() const;

private:
    TString Attribute;
    TString Value;
    bool IsInsideTag;
    int TagDepthAfterMain;
};

/// Generic handler that extracts only pure text from the HTML.
class THtmlParserHandler : public IParserResult {
public:
    class TDoneException : public yexception {
    public:
        TDoneException() {}
    };

public:
    THtmlParserHandler(TRecognizeInputBuffer* outputBuffer,
                       TMetaTagsHints* metaTagsHints,
                       bool unknownUtf = false);
    THtmlChunk* OnHtmlChunk(const THtmlChunk& chunk) override;

    /// Determine whether the parser for the document was chosen correctly.
     /* The ParseHtml function uses this function to check if it is necessary
      * to run another parser on the document.
      */
    virtual bool IsHtmlHandlerCorrect() const;

protected:
    TRecognizeInputBuffer* OutputBuffer;
    TMetaTagsHints* Hints;
    const bool UnknownUtf;
    wchar16* AttrGuard;
    TTagInteriorDetector<HT_OPTION> OptionDetector;

protected:
    void CheckLangAttr(const THtmlChunk* chunk);
    void CheckMeta(const THtmlChunk* chunk);
    void CheckUsefulAttrs(const THtmlChunk& chunk);
    void PutEventTextToBuffer(const THtmlChunk* chunk);
};

/// Parser for twitter timeline documents. Removes all auxilary text.
class TTwitterHtmlParserHandler : public THtmlParserHandler {
public:
    TTwitterHtmlParserHandler(TRecognizeInputBuffer* outputBuffer,
                              TMetaTagsHints* metaTagsHints,
                              bool unknownUtf = false);
    THtmlChunk* OnHtmlChunk(const THtmlChunk& chunk) override;
    bool IsHtmlHandlerCorrect() const override;

private:
    TTagInteriorDetector<HT_DIV> TweetTextDetector;
    bool IsCorrect;
};

/// Parser for youtube watch-video documents. Removes all auxilary text.
class TYoutubeHtmlParserHandler : public THtmlParserHandler {
public:
    TYoutubeHtmlParserHandler(TRecognizeInputBuffer* outputBuffer,
                              TMetaTagsHints* metaTagsHints,
                              bool unknownUtf = false);
    THtmlChunk* OnHtmlChunk(const THtmlChunk& chunk) override;
    bool IsHtmlHandlerCorrect() const override;

private:
    TTagInteriorDetector<HT_DIV> HeadlineDetector;
    TTagInteriorDetector<HT_DIV> DescriptionDetector;
    bool IsCorrect;
};

THtmlParserHandler::THtmlParserHandler(TRecognizeInputBuffer* outputBuffer,
                                       TMetaTagsHints* metaTagsHints,
                                       bool unknownUtf)
    : OutputBuffer(outputBuffer)
    , Hints(metaTagsHints)
    , UnknownUtf(unknownUtf)
{
    size_t bufsize = OutputBuffer->End - OutputBuffer->Buffer;

    memset(OutputBuffer->Buffer, 0, bufsize * sizeof(wchar16));

    // Reserve at least half of the buffer for the text
    AttrGuard = OutputBuffer->Buffer + (bufsize / 2);
}

bool THtmlParserHandler::IsHtmlHandlerCorrect() const {
    return true;
}

static inline const char* GetCharsetCanonicalName(const char* name) {
    ECharset code = CharsetByName(name);
    if (code == CODES_UNKNOWN)
        return name;

    return NameByCharset(code);
}

static inline void AppendHint(TString& hints, const TString& hint) {
    if (hints.empty())
        hints = hint;
    else
        hints += "\t" + hint;
}

void THtmlParserHandler::CheckLangAttr(const THtmlChunk* chunk) {
    if (!chunk->Tag)
        return;
    switch (chunk->Tag->id()) {
        case HT_HTML:
        case HT_HEAD:
        case HT_BODY:
            break;
        default:
            return;
    }
    for (size_t i = 0; i != chunk->AttrCount; ++i) {
        const NHtml::TAttribute& attr = chunk->Attrs[i];
        TString attrName(chunk->text + attr.Name.Start, attr.Name.Leng);
        if (!stricmp(attrName.data(), "lang")) {
            TString attrValue(chunk->text + attr.Value.Start, attr.Value.Leng);
            AppendHint(Hints->Language, attrValue);
            break;
        }
    }
}

void THtmlParserHandler::CheckMeta(const THtmlChunk* chunk) {
    CheckLangAttr(chunk);
    if (!chunk->Tag || chunk->Tag->id() != HT_META ||
        chunk->flags.markup == MARKUP_IGNORED)
    {
        return;
    }
    int curProp = 0;
    for (size_t i = 0; i < chunk->AttrCount; ++i) {
        const NHtml::TAttribute& attr = chunk->Attrs[i];
        TString attrName(chunk->text + attr.Name.Start, attr.Name.Leng);
        TString attrValue(chunk->text + attr.Value.Start, attr.Value.Leng);
        if (!stricmp(attrName.data(), "charset"))
            AppendHint(Hints->Encoding, GetCharsetCanonicalName(attrValue.data()));
        if (!stricmp(attrValue.data(), "content-type"))
            curProp = 1;
        else if (!stricmp(attrValue.data(), "content-language"))
            curProp = 2;
    }
    if (curProp) {
        for (size_t i = 0; i < chunk->AttrCount; ++i) {
            const NHtml::TAttribute& attr = chunk->Attrs[i];
            TString attrName(chunk->text + attr.Name.Start, attr.Name.Leng);
            TString attrValue(chunk->text + attr.Value.Start, attr.Value.Leng);
            if (curProp && !stricmp(attrName.data(), "content")) {
                str_spn sspn_spaces("\x20\t\n\r");
                if (curProp == 1) {
                    const char *p = attrValue.data();
                    p += sspn_spaces.spn(p);
                    if (strnicmp(p, "text/html", 9) != 0)
                        break;
                    p += 9;
                    p += sspn_spaces.spn(p);
                    if (*p != ';')
                        break;
                    p += 1;
                    p += sspn_spaces.spn(p);
                    if (strnicmp(p, "charset", 7) != 0)
                        break;
                    p += 7;
                    p += sspn_spaces.spn(p);
                    if (*p != '=')
                        break;
                    p += 1;
                    p += sspn_spaces.spn(p);
                    const TString& charset = Strip(p);
                    AppendHint(Hints->Encoding, GetCharsetCanonicalName(charset.data()));
                } else {
                    AppendHint(Hints->Language, attrValue);
                }
            }
        }
    }
}

static inline bool OneMoreIsUsefulText(const THtmlChunk* ev) {
    TEXT_WEIGHT w = (TEXT_WEIGHT)ev->flags.weight;
    PARSED_TYPE t = (PARSED_TYPE)ev->flags.type;
    // ev->flags.atype is hack for noindex text
    return t == PARSED_TEXT && (w != WEIGHT_ZERO || ev->flags.atype != 0);
}

static inline size_t DecodeToUnknownPlane(
        bool unkUtf, const char* txt, unsigned len, wchar16* result)
{
    Y_ASSERT(txt && len);
    if (unkUtf) {
        return HtEntDecodeToChar(CODES_UTF8, txt, len, result);
    } else {
        return HtEntDecodeToChar(CODES_UNKNOWNPLANE, txt, len, result);
    }
}

void THtmlParserHandler::CheckUsefulAttrs(const THtmlChunk& chunk) {
    if (!chunk.AttrCount)
        return;

    for (size_t i = 0; i != chunk.AttrCount; ++i) {
        if (OutputBuffer->Attrs < AttrGuard)
            break;

        const NHtml::TAttribute& a = chunk.Attrs[i];

        const char* atext = chunk.text + a.Value.Start;
        size_t aleng = a.Value.Leng;

        if (aleng) {
            TCharTemp tempBuf(aleng);
            size_t textLen = DecodeToUnknownPlane(
                    UnknownUtf, atext, aleng, tempBuf.Data());
            size_t copyLen = Min(
                    size_t(OutputBuffer->Attrs - OutputBuffer->TextEnd),
                    textLen);
            if (AttrGuard + copyLen > OutputBuffer->Attrs)
                copyLen = OutputBuffer->Attrs - AttrGuard;
            OutputBuffer->Attrs -= copyLen;
            memcpy(OutputBuffer->Attrs,
                   tempBuf.Data(),
                   copyLen * sizeof(wchar16));
            if (OutputBuffer->Attrs > OutputBuffer->TextEnd)
                *--OutputBuffer->Attrs = ' ';
        }
    }
}

void THtmlParserHandler::PutEventTextToBuffer(const THtmlChunk* chunk) {
    // only put space if not present
    if (IsWordBreak(chunk) && *OutputBuffer->TextEnd != ' ')
        *OutputBuffer->TextEnd++ = ' ';
    if (OutputBuffer->TextEnd == OutputBuffer->Attrs)
        return;

    if (!chunk->leng)
        return;

    Y_ASSERT(OutputBuffer->Attrs > OutputBuffer->TextEnd);

    if (OneMoreIsUsefulText(chunk)) {
        TCharTemp tempBuf(chunk->leng);
        size_t textLen = DecodeToUnknownPlane(
                UnknownUtf, chunk->text, chunk->leng, tempBuf.Data());
        size_t copyLen = Min(
                size_t(OutputBuffer->Attrs - OutputBuffer->TextEnd),
                textLen);
        memcpy(OutputBuffer->TextEnd, tempBuf.Data(), copyLen * sizeof(wchar16));
        OutputBuffer->TextEnd += copyLen;
    } else if (OutputBuffer->Attrs > AttrGuard) {
        CheckUsefulAttrs(*chunk);
    }
}

THtmlChunk* THtmlParserHandler::OnHtmlChunk(const THtmlChunk& chunk) {
    OptionDetector.Update(chunk);
    if (OptionDetector.IsInside()) {
        return nullptr;
    }
    if (OutputBuffer->TextEnd < OutputBuffer->Attrs) {
        CheckMeta(&chunk);
        PutEventTextToBuffer(&chunk);
    } else {
        throw TDoneException();
    }
    return nullptr;
}

template <HT_TAG TagId>
void TTagInteriorDetector<TagId>::Update(const THtmlChunk& chunk) {
    if (!chunk.Tag || chunk.Tag->id() != TagId) {
        return;
    }
    if (chunk.GetLexType() == HTLEX_EMPTY_TAG) {
        return;
    }

    if (chunk.GetLexType() == HTLEX_END_TAG) {
        if (IsInsideTag) {
            --TagDepthAfterMain;
            if (TagDepthAfterMain == 0) {
                IsInsideTag = false;
            }
        }
    } else {
        Y_ASSERT(chunk.GetLexType() == HTLEX_START_TAG);

        if (Attribute.empty()) {
            IsInsideTag = true;
        } else {
            for (size_t i = 0; i < chunk.AttrCount; ++i) {
                TStringBuf attrName(chunk.text + chunk.Attrs[i].Name.Start,
                                chunk.Attrs[i].Name.Leng);
                TStringBuf attrValue(chunk.text + chunk.Attrs[i].Value.Start,
                                 chunk.Attrs[i].Value.Leng);

                if (attrName == Attribute && attrValue == Value) {
                    IsInsideTag = true;
                    break;
                }
            }
        }

        if (IsInsideTag) {
            ++TagDepthAfterMain;
        }
    }
}

template <HT_TAG TagId>
bool TTagInteriorDetector<TagId>::IsInside() const {
    return IsInsideTag;
}

TTwitterHtmlParserHandler::TTwitterHtmlParserHandler(
        TRecognizeInputBuffer* outputBuffer,
        TMetaTagsHints* metaTagsHints,
        bool unknownUtf)
    : THtmlParserHandler(outputBuffer, metaTagsHints, unknownUtf)
    , TweetTextDetector("class", "js-tweet-text-container")
    , IsCorrect(false)
{ }

THtmlChunk* TTwitterHtmlParserHandler::OnHtmlChunk(const THtmlChunk& chunk) {
    TweetTextDetector.Update(chunk);

    if (OutputBuffer->TextEnd < OutputBuffer->Attrs) {
        CheckMeta(&chunk);
        if (TweetTextDetector.IsInside()) {
            IsCorrect = true;
            PutEventTextToBuffer(&chunk);
        }
    } else {
        throw TDoneException();
    }
    return nullptr;
}

bool TTwitterHtmlParserHandler::IsHtmlHandlerCorrect() const {
    return IsCorrect;
}

TYoutubeHtmlParserHandler::TYoutubeHtmlParserHandler(
        TRecognizeInputBuffer* outputBuffer,
        TMetaTagsHints* metaTagsHints,
        bool unknownUtf)
    : THtmlParserHandler(outputBuffer, metaTagsHints, unknownUtf)
    , HeadlineDetector("id", "watch7-headline")
    , DescriptionDetector("id", "watch-description-text")
    , IsCorrect(false)
{ }

THtmlChunk* TYoutubeHtmlParserHandler::OnHtmlChunk(const THtmlChunk& chunk) {
    HeadlineDetector.Update(chunk);
    DescriptionDetector.Update(chunk);

    if (OutputBuffer->TextEnd < OutputBuffer->Attrs) {
        CheckMeta(&chunk);
        if (HeadlineDetector.IsInside() || DescriptionDetector.IsInside()) {
            IsCorrect = true;
            PutEventTextToBuffer(&chunk);
        }
    } else {
        throw TDoneException();
    }
    return nullptr;
}

bool TYoutubeHtmlParserHandler::IsHtmlHandlerCorrect() const {
    return IsCorrect;
}

static void RunParseHtml(const TStringBuf& data, THtmlParserHandler& parserHandler) {
    TMemoryInput input(data.begin(), data.size());
    try {
        TBuffer normalizedDoc;
        {
            TBufferOutput out(normalizedDoc);
            TransferData(&input, &out);
        }
        NHtml5::ParseHtml(normalizedDoc, &parserHandler);
    } catch(const THtmlParserHandler::TDoneException&) {
        // do nothing
    }
}

static void RunParseHtml(NHtml::TSegmentedQueueIterator first,
                         NHtml::TSegmentedQueueIterator last,
                         THtmlParserHandler& parserHandler)
{
    try {
        while (first != last) {
            const THtmlChunk* const ev = GetHtmlChunk(first);
            if (ev->flags.type <= (int)PARSED_EOF)
                break;
            ++first;
            parserHandler.OnHtmlChunk(*ev);
        }
    } catch (const THtmlParserHandler::TDoneException&) {
        // do nothing
    }
}

static void RunParseHtml(const NHtml::TChunksRef& chunks, THtmlParserHandler& parserHandler) {
    try {
        if (!NHtml::NumerateHtmlChunks(chunks, &parserHandler)) {
            ythrow yexception() << "can't deserialize html chunks";
        }
    } catch (const THtmlParserHandler::TDoneException&) {
        // do nothing
    }
}

TAutoPtr<THtmlParserHandler> ChooseHandler(const TRecognizer::THints& hints,
                                           TRecognizeInputBuffer* outputBuffer,
                                           TMetaTagsHints* metaTagsHints,
                                           bool unknownUtf,
                                           bool generalParserOnly)
{
    if (!generalParserOnly) {
        TStringBuf host = GetHost(CutHttpPrefix(hints.Url));

        if (host.find("youtube") != TStringBuf::npos) {
            return TAutoPtr<THtmlParserHandler>(new TYoutubeHtmlParserHandler(
                outputBuffer, metaTagsHints, unknownUtf));
        } else if (host.find("twitter") != TStringBuf::npos) {
            return TAutoPtr<THtmlParserHandler>(new TTwitterHtmlParserHandler(
                outputBuffer, metaTagsHints, unknownUtf));
        }
    }

    return TAutoPtr<THtmlParserHandler>(new THtmlParserHandler(
        outputBuffer, metaTagsHints, unknownUtf));
}

} // namespace


void ParseHtml(const TStringBuf& data,
               const TRecognizer::THints& hints,
               TVector<wchar16>* buffer,
               TRecognizeInputBuffer* recognizerBuffer,
               TMetaTagsHints* metaTagsHints,
               bool unknownUtf,
               bool generalParserOnly)
{
    *recognizerBuffer = TRecognizeInputBuffer(buffer->begin(), buffer->end());

    TAutoPtr<THtmlParserHandler> handler = ChooseHandler(hints,
                                                         recognizerBuffer,
                                                         metaTagsHints,
                                                         unknownUtf,
                                                         generalParserOnly);
    RunParseHtml(data, *handler.Get());

    if (!handler->IsHtmlHandlerCorrect()) {
        // Retry with default parser
        *recognizerBuffer = TRecognizeInputBuffer(buffer->begin(),
                                                  buffer->end());
        THtmlParserHandler defaultHandler(recognizerBuffer,
                                          metaTagsHints,
                                          unknownUtf);
        RunParseHtml(data, defaultHandler);
    }
}

void ParseHtml(NHtml::TSegmentedQueueIterator first,
               NHtml::TSegmentedQueueIterator last,
               const TRecognizer::THints& hints,
               TVector<wchar16>* buffer,
               TRecognizeInputBuffer* recognizerBuffer,
               TMetaTagsHints* metaTagsHints,
               bool unknownUtf,
               bool generalParserOnly)
{
    *recognizerBuffer = TRecognizeInputBuffer(buffer->begin(), buffer->end());

    TAutoPtr<THtmlParserHandler> handler = ChooseHandler(hints,
                                                         recognizerBuffer,
                                                         metaTagsHints,
                                                         unknownUtf,
                                                         generalParserOnly);
    RunParseHtml(first, last, *handler.Get());

    if (!handler->IsHtmlHandlerCorrect()) {
        // Retry with default parser
        *recognizerBuffer = TRecognizeInputBuffer(buffer->begin(),
                                                  buffer->end());
        THtmlParserHandler defaultHandler(recognizerBuffer,
                                          metaTagsHints,
                                          unknownUtf);
        RunParseHtml(first, last, defaultHandler);
    }
}

void ParseHtml(const NHtml::TChunksRef& chunks,
               const TRecognizer::THints& hints,
               TVector<wchar16>* buffer,
               TRecognizeInputBuffer* recognizerBuffer,
               TMetaTagsHints* metaTagsHints,
               bool unknownUtf,
               bool generalParserOnly)
{
    *recognizerBuffer = TRecognizeInputBuffer(buffer->begin(), buffer->end());

    TAutoPtr<THtmlParserHandler> handler = ChooseHandler(hints,
                                                         recognizerBuffer,
                                                         metaTagsHints,
                                                         unknownUtf,
                                                         generalParserOnly);
    RunParseHtml(chunks, *handler.Get());

    if (!handler->IsHtmlHandlerCorrect()) {
        // Retry with default parser
        *recognizerBuffer = TRecognizeInputBuffer(buffer->begin(),
                                                  buffer->end());
        THtmlParserHandler defaultHandler(recognizerBuffer,
                                          metaTagsHints,
                                          unknownUtf);
        RunParseHtml(chunks, defaultHandler);
    }
}

}
