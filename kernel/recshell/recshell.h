#pragma once

#include <util/generic/ptr.h>
#include <library/cpp/html/storage/queue.h>
#include <dict/recognize/docrec/recognizer.h>

namespace NHtml {
    class TChunksRef;
}

class TRecognizerShell {
public:
    typedef TRecognizer::THints THints;

private:
    const TRecognizer* Recognizer;
    TAtomicSharedPtr<TRecognizer> StoredRecognizer;

private:
    void RecognizeImpl(TRecognizeInputBuffer* textBuffer,
                       ECharset* encoding,
                       ELanguage* lang,
                       ELanguage* secondLang,
                       const TRecognizer::THints& hints);
    void RecognizeImpl(TRecognizeInputBuffer& textBuffer,
                       ECharset& encoding,
                       TRecognizer::TLanguages& languages,
                       const TRecognizer::THints& hints);


protected:
    TVector<wchar16> Buffer;

public:
    enum {
        DefaultBufSize = 1024 * 1024
    };

    TRecognizerShell(const TRecognizer* rec, size_t bufsize = DefaultBufSize);
    TRecognizerShell(const TString& dictfile, size_t bufsize = DefaultBufSize);

    const TRecognizer* GetRecognizer() const;

    void Recognize(NHtml::TSegmentedQueueIterator first, NHtml::TSegmentedQueueIterator last,
        ECharset* encoding, ELanguage* lang, ELanguage* secondLang, const THints& hints);

    void Recognize(const NHtml::TChunksRef& chunks,
        ECharset* encoding, ELanguage* lang, ELanguage* secondLang, const THints& hints);

    void Recognize(NHtml::TSegmentedQueueIterator first, NHtml::TSegmentedQueueIterator last,
        ECharset& encoding, TRecognizer::TLanguages& languages, const THints& hints);

    void RecognizeHtml(const char* text, size_t len, ECharset& encoding, TRecognizer::TLanguages& languages, const THints& hints = THints(), bool unkUtf = false);
    void RecognizeHtml(const char* text, size_t len, ECharset* encoding, ELanguage* lang, ELanguage* secondLang = nullptr, const THints& hints = THints());

    ECharset RecognizeEncoding(const TStringBuf& html, const THints& hints = THints());
    ECharset RecognizeEncoding(NHtml::TSegmentedQueueIterator first, NHtml::TSegmentedQueueIterator last,
        const THints& hints = THints());
};
