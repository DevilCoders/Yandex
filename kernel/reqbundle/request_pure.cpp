#include "request_pure.h"

#include "reqbundle_accessors.h"
#include "block_pure.h"

namespace NReqBundle {
    void FillRevFreqs(TRequestAcc request, TConstSequenceAcc seq, bool overwriteAll)
    {
        for (auto freqType : NLingBoost::TWordFreq::GetValues()) {
            TVector<float> wordMatchFreqs;
            wordMatchFreqs.resize(request.GetNumWords(), 0.0f);

            for (auto match : request.Matches()) {
                auto elem = seq.GetElem(match);
                i64 matchRevFreq = match.GetRevFreq(freqType);

                Y_ASSERT(elem.HasBlock());

                if (elem.HasBlock() && (!NLingBoost::IsValidRevFreq(matchRevFreq) || overwriteAll)) {
                    matchRevFreq = CalcCompoundRevFreq(elem.GetBlock(), freqType);
                    match.SetRevFreq(freqType, matchRevFreq);
                }

                if (NLingBoost::IsValidRevFreq(matchRevFreq)) {
                    for (size_t wordIndex : match.GetWordIndexes()) {
                        // TODO Consider using Min(revFreq) instead
                        wordMatchFreqs[wordIndex] += NLingBoost::RevFreqToFreq(matchRevFreq);
                        Y_ASSERT(wordMatchFreqs[wordIndex] > 0.0f);
                    }
                }
            }

            for (size_t wordIndex : xrange(request.GetNumWords())) {
                auto word = request.Word(wordIndex);
                i64 wordRevFreq = word.GetRevFreq(freqType);

                if (!NLingBoost::IsValidRevFreq(wordRevFreq) || overwriteAll) {
                    word.SetRevFreq(freqType, NLingBoost::FreqToRevFreq(Min(wordMatchFreqs[wordIndex], 1.0f)));
                }
            }
        }
    }

    void FillRevFreqs(TReqBundleAcc bundle, bool overwriteAll)
    {
        for (auto request : bundle.Requests()) {
            FillRevFreqs(request, bundle.GetSequence(), overwriteAll);
        }
    }
} // NReqBundle
