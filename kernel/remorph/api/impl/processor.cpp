#include "processor.h"
#include "results.h"

#include <kernel/remorph/text/word_input_symbol.h>
#include <kernel/remorph/facts/fact.h>

#include <util/generic/ptr.h>

namespace NRemorphAPI {

namespace NImpl {

class TFactCallback {
private:
    TResults* Res;
public:
    TFactCallback(TResults* res)
        : Res(res)
    {
    }

    void operator()(const NToken::TSentenceInfo& sentInfo, NText::TWordSymbols& words, TVector<NFact::TFactPtr>& facts) const {
        Res->Add(sentInfo.Text, facts, words);
    }
};



TProcessor::TProcessor(const NText::TTextFactProcessorPtr& pr, const NToken::TTokenizeOptions& opts, const TLangMask& lng)
    : Processor(pr)
    , Opts(opts)
    , Lang(lng)
{
    Processor->SetResolveFactAmbiguity(false);
}

IResults* TProcessor::Process(const char* text) const {
    TAutoPtr<TResults> res(new TResults(this));
    TFactCallback cb(res.Get());
    Processor->ProcessText(cb, text, CODES_UTF8, Opts, Lang);
    return res.Release();
}

} // NImpl

} // NRemorphAPI

