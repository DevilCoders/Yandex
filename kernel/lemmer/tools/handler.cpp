#include "handler.h"
#include "order.h"

namespace NLemmer {
    TLemmerHandler::TLemmerHandler(const TParams& params, ILemmerResultsHandler& handler)
        : Params(params)
        , Handler(handler)
    {
    }

    void TLemmerHandler::SetLangMask(const TLangMask& langMask) {
        Params.LangMask = langMask;
    }

    void TLemmerHandler::SetParams(const TParams& params) {
        Params = params;
    }

    TLemmerHandler::TParams TLemmerHandler::GetParams() const {
        return Params;
    }

    void TLemmerHandler::OnToken(const TWideToken& tok, size_t /*origleng*/, NLP_TYPE type) {
        if (type == NLP_MISCTEXT || GetSpaceType(type) != ST_NOBRK) {
            Handler.OnLemmerResults(tok, type, nullptr);
        } else {
            TWLemmaArray lemmas;
            NLemmer::AnalyzeWord(tok, lemmas, Params.LangMask, Params.DocLangs, Params.WordOpt);
            if (lemmas.empty())
                return;

            StableSort(lemmas.begin(), lemmas.end(), TLemmaOrder());
            size_t cur_end = lemmas[0].GetTokenPos() + lemmas[0].GetTokenSpan();

            TLemmerResults results;
            for (const auto& lm : lemmas) {
                size_t end = lm.GetTokenPos() + lm.GetTokenSpan();
                if (end > cur_end) {
                    if (!results.empty()) {
                        Handler.OnLemmerResults(tok, type, &results);
                        results.clear();
                    }
                    cur_end = end;
                }
                results.push_back(TLemmerResult(lm, 0));
            }
            if (!results.empty())
                Handler.OnLemmerResults(tok, type, &results);
        }
    }

}
