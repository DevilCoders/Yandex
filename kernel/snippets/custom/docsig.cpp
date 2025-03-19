#include "docsig.h"

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/sent_info/sent_info.h>
#include <kernel/snippets/sent_match/sent_match.h>
#include <kernel/snippets/sent_match/extra_attrs.h>

#include <util/generic/algorithm.h>
#include <util/generic/utility.h>
#include <util/generic/vector.h>
#include <util/string/cast.h>

namespace NSnippets {

// calculates a size_t (currently 64 bit) signature that is dependent on both the document and the query
// uses an algorithm similar to the Simhash
// uses precalculated hash from TPodStroka, usually stable (but can be unstable over releases)
static size_t CalculateSignature(const TReplaceContext& repCtx) {
    const TSentsMatchInfo& smi = repCtx.SentsMInfo;

    TVector<size_t> sigVector;
    const int sigWidth = repCtx.Cfg.GetSignatureWidth();
    const int sigMinLen = repCtx.Cfg.GetSignatureMinLength();
    const int sentsCount = smi.SentsInfo.SentencesCount();

    // form a signature array by selecting words around each match (use 64 bit precalculated hashes)
    for (int sentIdx = 0; sentIdx < sentsCount; ++sentIdx) {
        const int w0 = smi.SentsInfo.FirstWordIdInSent(sentIdx);
        const int w1 = smi.SentsInfo.LastWordIdInSent(sentIdx);

        if (smi.HeaderSentsInRange(sentIdx, sentIdx) == 0 && smi.GetMainContentWords(w0, w1) == 0) {
            // Only use matches in the header and main content segments
            continue;
        }

        for (int wordIdx = w0; wordIdx <= w1; ++wordIdx) {
            if (smi.IsMatch(wordIdx)) {
                int matchWindowStart = Max(w0, wordIdx - sigWidth);
                int matchWindowEnd = Min(w1, wordIdx + sigWidth);
                for (int mwIdx = matchWindowStart; mwIdx <= matchWindowEnd; ++mwIdx) {
                    if (mwIdx == wordIdx)
                        continue;

                    sigVector.push_back(smi.SentsInfo.WordVal[mwIdx].Word.Hash);
                }
            }
        }
    }

    // for the better results leave only a single instance of each word
    Sort(sigVector.begin(), sigVector.end());
    TVector<size_t>::iterator newSigVectorEnd = Unique(sigVector.begin(), sigVector.end());

    if (newSigVectorEnd - sigVector.begin() < sigMinLen) {
        // the length of the signature is too short, do not use it at all
        return 0;
    }

    // lets find the difference between counts of 1's and 0's in each bit position for the signature array
    static const int digestSize = sizeof(size_t) * 8;
    int digest[digestSize] = {};

    for (TVector<size_t>::const_iterator it = sigVector.begin(); it != newSigVectorEnd; ++it) {
        const size_t sigWord = *it;
        for (int digestBit = 0; digestBit < digestSize; ++digestBit) {
            if (sigWord & (static_cast<size_t>(1) << digestBit))
                ++digest[digestBit];
            else
                --digest[digestBit];
        }
    }

    // and now form the signature as 64 bit integer where each bit set only if it is set in more than 50% of words in the array
    size_t resSignature = 0;
    for (int digestBit = 0; digestBit < digestSize; ++digestBit) {
        if (digest[digestBit] > 0) {
            resSignature += (static_cast<size_t>(1) << digestBit);
        }
    }

    return resSignature;
}

void TDocSigDataReplacer::DoWork(TReplaceManager* manager) {
    const TReplaceContext& repCtx = manager->GetContext();
    const TConfig& config = repCtx.Cfg;
    // calculate the doc-query signature when it is requested and the document is not found by link
    if (config.ShouldCalculateSignature() && !repCtx.IsByLink) {
        manager->GetExtraSnipAttrs().SetDocQuerySig(ToString(CalculateSignature(repCtx)));
        config.LogPerformance("FillReply.QuerySignature");
    }

    if (config.ShouldReturnDocStaticSig()) {
        manager->GetExtraSnipAttrs().SetDocStaticSig(ToString(config.DocStaticSig()));
        config.LogPerformance("FillReply.DocSignature");
    }
}

}
