#include "htmlparse.h"
#include "recshell.h"

#include <util/charset/utf8.h>
#include <util/generic/string.h>

using namespace NRecognizer;
using namespace NRecognizerShell;

TRecognizerShell::TRecognizerShell(const TRecognizer* rec, size_t bufsize)
    : Recognizer(rec)
    , Buffer(bufsize, 0)
{
}

TRecognizerShell::TRecognizerShell(const TString& dictfile, size_t bufsize)
    : Buffer(bufsize, 0)
{
    StoredRecognizer.Reset(new TRecognizer(dictfile));
    Recognizer = StoredRecognizer.Get();
}

const TRecognizer* TRecognizerShell::GetRecognizer() const {
    return Recognizer;
}

static TString LastWord(const TString& words) {
    TString r(words);
    size_t pos = 0;
    if ((pos = words.rfind('\t')) != TString::npos)
        r.erase(0, pos + 1);
    return r;
}

TRecognizer::THints MergeHints(const TMetaTagsHints& meta,
                               const TRecognizer::THints& hints)
{
    /// @todo: correctly process multiple languages from parser
    /// now just get last word to simulate prev behaviour
    TRecognizer::THints res(hints);

    if (meta.Encoding.size()) {
        const char* encname = meta.Encoding.data();
        const char* pos = encname;
        const char* endpos = strchr(pos, '\t');
        TString enctoken;

        while (pos) {
            if (endpos) {
                enctoken.assign(pos, endpos);
                pos = endpos + 1;
                endpos = strchr(pos, '\t');
            } else {
                enctoken.assign(pos);
                pos = nullptr;
            }

            ECharset enc = EncodingHintByName(enctoken.data());
            if (ValidCodepage(enc))
                res.HtmlCodepage.Set(enc);
        }
     }

    if (meta.Language.size()) {
        const char* langname = meta.Language.data();
        res.HtmlLanguage = LastWord(langname);
    }
    return res;
}

void TRecognizerShell::RecognizeImpl(TRecognizeInputBuffer& textBuffer,
                                     ECharset& encoding,
                                     TRecognizer::TLanguages& languages,
                                     const THints& hints)
{
    encoding = Recognizer->RecognizeEncoding(textBuffer, hints);
    if (encoding != CODES_UNKNOWN) {
        DecodeUnknownPlane(textBuffer.Buffer, textBuffer.TextEnd, encoding);
        DecodeUnknownPlane(textBuffer.Attrs, textBuffer.End, encoding);
        Recognizer->RecognizeLanguage(textBuffer.Buffer,
                                      textBuffer.TextEnd,
                                      languages,
                                      hints);
    } else {
        languages.clear();
        languages.emplace_back();
    }
}

void TRecognizerShell::RecognizeHtml(const char* text,
                                     size_t len,
                                     ECharset& encoding,
                                     TRecognizer::TLanguages& languages,
                                     const THints& hints,
                                     bool unkUtf)
{
    TStringBuf data(text, len);
    TRecognizeInputBuffer textBuffer(Buffer.begin(), Buffer.end());
    TMetaTagsHints metaTagsHints;

    ParseHtml(data, hints, &Buffer, &textBuffer, &metaTagsHints, unkUtf);
    TRecognizer::THints fullHints(MergeHints(metaTagsHints, hints));
    RecognizeImpl(textBuffer, encoding, languages, fullHints);
}

void TRecognizerShell::RecognizeImpl(TRecognizeInputBuffer* textBuffer,
                                     ECharset* encoding,
                                     ELanguage* lang,
                                     ELanguage* secondLang,
                                     const TRecognizer::THints& hints)
{
    ECharset enc = Recognizer->RecognizeEncoding(*textBuffer, hints);
    if (encoding) {
        *encoding = enc;
    }

    if (enc != CODES_UNKNOWN) {
        DecodeUnknownPlane(textBuffer->Buffer, textBuffer->TextEnd, enc);
        DecodeUnknownPlane(textBuffer->Attrs, textBuffer->End, enc);
        if (lang) {
            *lang = Recognizer->RecognizeLanguage(
                textBuffer->Buffer, textBuffer->TextEnd, secondLang, hints);
        }
    } else {
        if (lang) {
            *lang = LANG_UNK;
        }
        if (secondLang) {
            *secondLang = LANG_UNK;
        }
    }
}

void TRecognizerShell::RecognizeHtml(const char* text,
                                     size_t len,
                                     ECharset* encoding,
                                     ELanguage* lang,
                                     ELanguage* secondLang,
                                     const THints& hints)
{
    TStringBuf data(text, len);
    TRecognizeInputBuffer textBuffer(Buffer.begin(), Buffer.end());
    TMetaTagsHints metaTagsHints;

    ParseHtml(data, hints, &Buffer, &textBuffer, &metaTagsHints);
    TRecognizer::THints fullHints = MergeHints(metaTagsHints, hints);
    RecognizeImpl(&textBuffer, encoding, lang, secondLang, fullHints);
}

void TRecognizerShell::Recognize(NHtml::TSegmentedQueueIterator first,
                                 NHtml::TSegmentedQueueIterator last,
                                 ECharset& encoding,
                                 TRecognizer::TLanguages& languages,
                                 const THints& hints)
{
    TRecognizeInputBuffer textBuffer(Buffer.begin(), Buffer.end());
    TMetaTagsHints metaTagsHints;

    ParseHtml(first, last, hints, &Buffer, &textBuffer, &metaTagsHints);
    TRecognizer::THints fullHints(MergeHints(metaTagsHints, hints));
    RecognizeImpl(textBuffer, encoding, languages, fullHints);
}

void TRecognizerShell::Recognize(NHtml::TSegmentedQueueIterator first,
                                 NHtml::TSegmentedQueueIterator last,
                                 ECharset* encoding,
                                 ELanguage* lang,
                                 ELanguage* secondLang,
                                 const THints& hints)
{
    TRecognizeInputBuffer textBuffer(Buffer.begin(), Buffer.end());
    TMetaTagsHints metaTagsHints;

    ParseHtml(first, last, hints, &Buffer, &textBuffer, &metaTagsHints);
    TRecognizer::THints fullHints(MergeHints(metaTagsHints, hints));
    RecognizeImpl(&textBuffer, encoding, lang, secondLang, fullHints);
}

void TRecognizerShell::Recognize(const NHtml::TChunksRef& chunks,
    ECharset* encoding, ELanguage* lang, ELanguage* secondLang, const THints& hints)
{
    TRecognizeInputBuffer textBuffer(Buffer.begin(), Buffer.end());
    TMetaTagsHints metaTagsHints;

    ParseHtml(chunks, hints, &Buffer, &textBuffer, &metaTagsHints);
    TRecognizer::THints fullHints(MergeHints(metaTagsHints, hints));
    RecognizeImpl(&textBuffer, encoding, lang, secondLang, fullHints);
}

ECharset TRecognizerShell::RecognizeEncoding(const TStringBuf& html, const THints& hints) {
    TRecognizeInputBuffer textBuffer(Buffer.begin(), Buffer.end());
    TMetaTagsHints metaTagsHints;

    ParseHtml(html, hints, &Buffer, &textBuffer, &metaTagsHints);

    return Recognizer->RecognizeEncoding(textBuffer, MergeHints(metaTagsHints, hints));
}

ECharset TRecognizerShell::RecognizeEncoding(NHtml::TSegmentedQueueIterator first,
                                             NHtml::TSegmentedQueueIterator last,
                                             const THints& hints)
{
    TRecognizeInputBuffer textBuffer(Buffer.begin(), Buffer.end());
    TMetaTagsHints metaTagsHints;

    ParseHtml(first, last, hints, &Buffer, &textBuffer, &metaTagsHints);

    return Recognizer->RecognizeEncoding(textBuffer, MergeHints(metaTagsHints, hints));
}
