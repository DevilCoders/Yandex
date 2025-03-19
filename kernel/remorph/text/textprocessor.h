#pragma once

#include "word_input_symbol.h"
#include "word_symbol_factory.h"

#include <kernel/remorph/tokenizer/tokenizer.h>
#include <kernel/remorph/tokenizer/worker_pool.h>
#include <kernel/remorph/cascade/processor.h>
#include <kernel/remorph/facts/factprocessor.h>
#include <kernel/remorph/common/verbose.h>
#include <kernel/gazetteer/gazetteer.h>

#include <library/cpp/tokenizer/tokenizer.h>

#include <util/generic/ptr.h>
#include <util/string/vector.h>

namespace NText {

template <class TRootMatcher>
class TTextProcessor: public NCascade::TProcessor<TRootMatcher> {
public:
    typedef typename NCascade::TProcessor<TRootMatcher>::TResult TResult;
    typedef typename NCascade::TProcessor<TRootMatcher>::TResultPtr TResultPtr;
    typedef typename NCascade::TProcessor<TRootMatcher>::TResults TResults;

    typedef TIntrusivePtr<TTextProcessor> TPtr;

private:
    const TGazetteer* Gazetteer;
    NToken::TWorkerPoolPtr Pool;
    bool WithMorphology = true;

private:
    template <class TResCallback>
    struct TCallbackProxy: public NToken::ITokenizerCallback {
        const TTextProcessor& Processor;
        TResCallback& Callback;
        TLangMask Lang;

        TCallbackProxy(const TTextProcessor& processor, TResCallback& callback, const TLangMask& lang)
            : Processor(processor)
            , Callback(callback)
            , Lang(lang)
        {
        }

        void OnSentence(const NToken::TSentenceInfo& sentInfo) override {
            TWordSymbols words = CreateWordSymbols(sentInfo, Lang, Processor.WithMorphology);
            TWordInput input;
            TResults results;
            Processor.ProcessSentence(words, input, results);
            Callback(sentInfo, input, results);
        }
    };

public:
    TTextProcessor(const TString& rulePath, const TGazetteer* gzt)
        : NCascade::TProcessor<TRootMatcher>(rulePath, gzt)
        , Gazetteer(gzt)
    {
    }

    TTextProcessor(IInputStream& ruleStream, const TGazetteer* gzt)
        : NCascade::TProcessor<TRootMatcher>(ruleStream, gzt)
        , Gazetteer(gzt)
    {
    }

    TTextProcessor(const TString& rulePath, const TGazetteer* gzt, const TString& baseDir)
        : NCascade::TProcessor<TRootMatcher>(rulePath, gzt, baseDir)
        , Gazetteer(gzt)
    {
    }

    void SetThreads(size_t threads) {
        Y_ASSERT(threads != 0);
        if (threads > 1) {
            Pool = new NToken::TWorkerPool(threads);
        } else {
            Pool = nullptr;
        }
    }

    void SetMorphologyUse(bool use = true) {
        WithMorphology = use;
    }

    // Finds results in the stream and calls the specified callback for all found results using the following parameters:
    // callback(const TSentenceInfo& sent, const TWordSymbols& words, const NText::TResults& results);
    // The callback is called for each detected sentence.
    // The method doesn't close the stream.
    template <class TResCallback>
    inline void ProcessStream(TResCallback& callback, IInputStream& input, ECharset enc,
        const NToken::TTokenizeOptions& opts, const TLangMask& lang = LANG_RUS) const {

        TCallbackProxy<TResCallback> proxy(*this, callback, lang);
        TokenizeStream(proxy, input, enc, opts, Pool.Get());
    }

    // Finds results in the file and calls the specified callback for all found results using the following parameters:
    // callback(const TSentenceInfo& sent, const TWordSymbols& words, const NText::TResults& results);
    // The callback is called for each detected sentence.
    template <class TResCallback>
    inline void ProcessFile(TResCallback& callback, const TString& file, ECharset enc,
        const NToken::TTokenizeOptions& opts, const TLangMask& lang = LANG_RUS) const {

        TCallbackProxy<TResCallback> proxy(*this, callback, lang);
        NToken::TokenizeFile(proxy, file, enc, opts, Pool.Get());
    }

    template <class TResCallback>
    inline void ProcessText(TResCallback& callback, const TString& text, ECharset enc,
        const NToken::TTokenizeOptions& opts, const TLangMask& lang = LANG_RUS) const {

        TCallbackProxy<TResCallback> proxy(*this, callback, lang);
        NToken::TokenizeText(proxy, text, enc, opts, Pool.Get());
    }

    template <class TResCallback>
    inline void ProcessText(TResCallback& callback, const TWtringBuf& text, const NToken::TTokenizeOptions& opts,
        const TLangMask& lang = LANG_RUS) const {

        TCallbackProxy<TResCallback> proxy(*this, callback, lang);
        NToken::TokenizeText(proxy, text, opts, Pool.Get());
    }

    void ProcessSentence(const TWordSymbols& words, TWordInput& input, TResults& results) const {
        if (!NCascade::TProcessor<TRootMatcher>::CreateInput(input, words, Gazetteer)) {
            REPORT(VERBOSE, "No usable gazetteer results");
            return;
        }

        NCascade::TProcessor<TRootMatcher>::Process(input, results);
        if (int(TRACE_DEBUG) <= GetVerbosityLevel()) {
            for (typename TResults::const_iterator iRes = results.begin(); iRes != results.end(); ++iRes) {
                REPORT(DEBUG, "Rule " << iRes->Get()->ToDebugString(GetVerbosityLevel(), input));
            }
        }
    }
};

class TTextFactProcessor: public NFact::TFactProcessor {
private:
    NToken::TWorkerPoolPtr Pool;
    bool WithMorphology = true;
    bool UseDisamb = false;

private:
    template <class TFactCallback>
    class TFactCallbackProxy: public NToken::ITokenizerCallback {
    private:
        const TTextFactProcessor& Processor;
        TFactCallback& Callback;
        TLangMask Lang;

    public:
        TFactCallbackProxy(const TTextFactProcessor& proc, TFactCallback& callback, const TLangMask& lang)
            : Processor(proc)
            , Callback(callback)
            , Lang(lang)
        {
        }

        void OnSentence(const NToken::TSentenceInfo& sentInfo) override {
            TWordSymbols words = CreateWordSymbols(sentInfo, Lang, Processor.WithMorphology, Processor.UseDisamb);
            TVector<NFact::TFactPtr> facts;
            Processor.ProcessSentence(words, facts);
            Callback(sentInfo, words, facts);
        }
    };

public:
    TTextFactProcessor()
        : NFact::TFactProcessor()
    {
    }

    TTextFactProcessor(const TString& path, const TGazetteer* externalGzt = nullptr)
        : NFact::TFactProcessor(path, externalGzt)
    {
    }

    void SetThreads(size_t threads) {
        Y_ASSERT(threads != 0);
        if (threads > 1) {
            Pool = new NToken::TWorkerPool(threads);
        } else {
            Pool = nullptr;
        }
    }

    void SetMorphologyUse(bool use = true) {
        WithMorphology = use;
    }

    void SetDisambUse(bool use = true) {
        UseDisamb = use;
    }

    // Collects facts from the specified stream. Creates internal tokenizer to tokenize the document
    template <class TFactCallback>
    inline void ProcessStream(TFactCallback& callback, IInputStream& stream, ECharset enc, const NToken::TTokenizeOptions& opts,
        const TLangMask& lang = LANG_RUS) const {
        TFactCallbackProxy<TFactCallback> proxy(*this, callback, lang);
        NToken::TokenizeStream(proxy, stream, enc, opts, Pool.Get());
    }

    // Finds results in the file and calls the specified callback for all found results using the following parameters:
    // callback(const TSentenceInfo& sent, const TWordSymbols& words, const NText::TResults& results);
    // The callback is called for each detected sentence.
    template <class TFactCallback>
    inline void ProcessFile(TFactCallback& callback, const TString& file, ECharset enc, const NToken::TTokenizeOptions& opts,
        const TLangMask& lang = LANG_RUS) const {
        TFactCallbackProxy<TFactCallback> proxy(*this, callback, lang);
        NToken::TokenizeFile(proxy, file, enc, opts, Pool.Get());
    }

    // Collects facts from the specified UTF8 text. Creates internal tokenizer to tokenize the document
    template <class TFactCallback>
    inline void ProcessText(TFactCallback& callback, const TString& text, ECharset enc, const NToken::TTokenizeOptions& opts,
        const TLangMask& lang = LANG_RUS) const {
        TFactCallbackProxy<TFactCallback> proxy(*this, callback, lang);
        NToken::TokenizeText(proxy, text, enc, opts, Pool.Get());
    }

    // Collects facts from the specified Unicode text. Creates internal tokenizer to tokenize the document
    template <class TFactCallback>
    inline void ProcessText(TFactCallback& callback, const TWtringBuf& text, const NToken::TTokenizeOptions& opts,
        const TLangMask& lang = LANG_RUS) const {
        TFactCallbackProxy<TFactCallback> proxy(*this, callback, lang);
        NToken::TokenizeText(proxy, text, opts, Pool.Get());
    }

    inline void ProcessSentence(const TWordSymbols& symbols, TVector<NFact::TFactPtr>& facts) const {
        NFact::TFactProcessor::CollectFacts(symbols, facts);
    }
};

typedef TIntrusivePtr<TTextFactProcessor> TTextFactProcessorPtr;

} // NText
