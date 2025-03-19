#pragma once

#include <util/generic/fwd.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

#include <limits>

namespace NAlice {

    namespace NMusic {

        // Is taken from grapheme ASR output
        struct TAsrHypo {
            // Logarithm of the hypothesis probability
            double Confidence;

            // Text of the hypothesis
            TString Hypo;

            TAsrHypo(double confidence, const TStringBuf hypo)
                : Confidence(confidence)
                , Hypo(hypo)
            {
            }
        };

        TString ExtendQueryWithAsrHypos(TStringBuf query,
                                        TVector<TAsrHypo>&& hypos,
                                        double similarityThreshold = .15,
                                        double confidenceThreshold = .5,
                                        double minimalConfidence = 0,
                                        size_t maxExtensionLength = 1,
                                        size_t maxNumHyposToCheck = std::numeric_limits<size_t>::max());

    } // namespace NMusic

} // namespace NAlice
