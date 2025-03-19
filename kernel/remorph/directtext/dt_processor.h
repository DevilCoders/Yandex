#pragma once

#include "dt_factory.h"
#include "dt_input_symbol.h"

#include <kernel/remorph/facts/fact.h>
#include <kernel/remorph/facts/factprocessor.h>
#include <kernel/remorph/cascade/processor.h>
#include <kernel/remorph/common/verbose.h>

#include <kernel/indexer/direct_text/dt.h>
#include <kernel/gazetteer/gazetteer.h>

#include <library/cpp/langmask/langmask.h>

#include <util/generic/vector.h>
#include <util/generic/typetraits.h>
#include <util/generic/ptr.h>
#include <library/cpp/containers/sorted_vector/sorted_vector.h>

namespace NDT {

template <class TRootMatcher>
class TDTProcessor: public NCascade::TProcessor<TRootMatcher> {
public:
    typedef typename NCascade::TProcessor<TRootMatcher>::TResult TResult;
    typedef typename NCascade::TProcessor<TRootMatcher>::TResultPtr TResultPtr;
    typedef typename NCascade::TProcessor<TRootMatcher>::TResults TResults;

    typedef TIntrusivePtr<TDTProcessor> TPtr;

private:
    const TGazetteer* Gazetteer;

private:
    template <class TResCallback>
    struct TCallbackProxy {
        const TDTProcessor& Processor;
        TResCallback& Callback;

        TCallbackProxy(const TDTProcessor& processor, TResCallback& callback)
            : Processor(processor)
            , Callback(callback)
        {
        }

        inline void operator()(const NIndexerCore::TDirectTextEntry2* /*entries*/, size_t /*count*/,
            const NDT::TDTInputSymbols& symbols) const {

            TDTInput input;
            TResults results;
            Processor.ProcessSentence(symbols, input, results);
            Callback(input, results);
        }
    };


public:
    TDTProcessor(const TString& rulePath, const TGazetteer* gzt)
        : NCascade::TProcessor<TRootMatcher>(rulePath, gzt)
        , Gazetteer(gzt)
    {
    }

    TDTProcessor(const TString& rulePath, const TGazetteer* gzt, const TString& baseDir)
        : NCascade::TProcessor<TRootMatcher>(rulePath, gzt, baseDir)
        , Gazetteer(gzt)
    {
    }

    template <class TResCallback>
    inline void ProcessDirectText(TResCallback& callback, const NIndexerCore::TDirectTextData2& dt,
        const TLangMask& langs = LANG_RUS, const TDTSegmenterPtr& segmenter = TDTSegmenterPtr()) const {

        TCallbackProxy<TResCallback> proxy(*this, callback);
        SplitDirectText(proxy, dt, langs, segmenter);
    }

    void ProcessSentence(const TDTInputSymbols& symbols, TDTInput& input, TResults& results) const {
        if (!NCascade::TProcessor<TRootMatcher>::CreateInput(input, symbols, Gazetteer)) {
            REPORT(VERBOSE, "No usable gazetteer results");
            return;
        }

        NCascade::TProcessor<TRootMatcher>::Process(input, results, TVector<TUtf16String>());
        if (int(TRACE_DEBUG) <= GetVerbosityLevel()) {
            for (typename TResults::const_iterator iRes = results.begin(); iRes != results.end(); ++iRes) {
                REPORT(DEBUG, "Rule " << iRes->Get()->ToDebugString(GetVerbosityLevel(), input));
            }
        }
    }
};

class TDTFactProcessor: public NFact::TFactProcessor {
private:

    Y_HAS_MEMBER(Preprocess);

    template<class TFactCallback>
    std::enable_if_t<TClassHasPreprocess<TFactCallback>::value, void>
    static CallOnSymbols(TFactCallback& callback, const NDT::TDTInputSymbols& symbols) {
        callback.Preprocess(symbols);
    }

    template<class TFactCallback>
    std::enable_if_t<!TClassHasPreprocess<TFactCallback>::value, void>
    static CallOnSymbols(TFactCallback&, const NDT::TDTInputSymbols&) {
    }

    template <class TFactCallback>
    class TFactCallbackProxy {
    private:
        const TDTFactProcessor& Processor;
        TFactCallback& Callback;

    public:
        TFactCallbackProxy(const TDTFactProcessor& proc, TFactCallback& callback)
            : Processor(proc)
            , Callback(callback)
        {
        }

        inline void operator()(const NIndexerCore::TDirectTextEntry2* /*entries*/, size_t /*count*/,
            const NDT::TDTInputSymbols& symbols) const {
            TVector<NFact::TFactPtr> facts;
            TDTFactProcessor::CallOnSymbols(Callback, symbols);
            Processor.ProcessSentence(symbols, facts);
            Callback(symbols, facts);
        }
    };


public:
    TDTFactProcessor()
        : NFact::TFactProcessor()
    {
    }
    TDTFactProcessor(const TString& path, const TGazetteer* externalGzt = nullptr)
        : NFact::TFactProcessor(path, externalGzt)
    {
    }

    // Collects facts from the specified Unicode text. Creates internal tokenizer to tokenize the document
    template <class TFactCallback>
    inline void ProcessDirectText(TFactCallback& callback, const NIndexerCore::TDirectTextData2& dt,
        const TLangMask& langs = LANG_RUS, const TDTSegmenterPtr& segmenter = TDTSegmenterPtr()) const {

        TFactCallbackProxy<TFactCallback> proxy(*this, callback);
        SplitDirectText(proxy, dt, langs, segmenter);
    }

    inline void ProcessSentence(const TDTInputSymbols& symbols, TVector<NFact::TFactPtr>& facts) const {
        NFact::TFactProcessor::CollectFacts(symbols, facts);
    }
};

typedef TIntrusivePtr<TDTFactProcessor> TDTFactProcessorPtr;

} // NDT
