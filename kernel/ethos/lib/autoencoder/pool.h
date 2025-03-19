#pragma once

#include <util/generic/vector.h>
#include <util/generic/string.h>

#include <util/stream/file.h>

#include <util/string/cast.h>

namespace NEthosAutoEncoder {

static inline TVector<TVector<double> > ReadFeatures(const TString& featuresPath) {
    TVector<TVector<double> > features;

    TFileInput featuresIn(featuresPath);
    TString dataStr;
    while (featuresIn.ReadLine(dataStr)) {
        TStringBuf dataStrBuf(dataStr);

        dataStrBuf.NextTok('\t'); // query id
        dataStrBuf.NextTok('\t'); // target
        dataStrBuf.NextTok('\t'); // url
        dataStrBuf.NextTok('\t'); // weight

        TVector<double> instance;
        while (dataStrBuf) {
            instance.push_back(FromString(dataStrBuf.NextTok('\t')));
        }

        features.push_back(instance);

        Y_VERIFY(features.front().size() == features.back().size());
    }

    return features;
}

template <typename TEncoder>
static inline void PrintFeatureTransformations(const TString& featuresPath, const TEncoder& encoder) {
    TFileInput featuresIn(featuresPath);
    TString dataStr;
    while (featuresIn.ReadLine(dataStr)) {
        TStringBuf dataStrBuf(dataStr);

        TStringBuf queryId = dataStrBuf.NextTok('\t'); // query id
        TStringBuf target = dataStrBuf.NextTok('\t'); // target
        TStringBuf url = dataStrBuf.NextTok('\t'); // url
        TStringBuf weight = dataStrBuf.NextTok('\t'); // weight

        TVector<double> instance;
        while (dataStrBuf) {
            instance.push_back(FromString(dataStrBuf.NextTok('\t')));
        }

        Cout << queryId << "\t"
             << target << "\t"
             << url << "\t"
             << weight << "\t"
             << JoinSeq("\t", encoder.Encode(instance)) << "\n";
    }
}

}
