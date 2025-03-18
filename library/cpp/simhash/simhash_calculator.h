#pragma once

#include "common.h"

#include <util/generic/noncopyable.h>
#include <util/generic/ptr.h>

template <
    class T,
    template <class> class TNGrammerType,
    template <class> class TVectorizerType,
    class TAlgorithm>
class TSimhashCalculator: private TNonCopyable {
public:
    typedef TNGrammerType<T> TNGrammer;
    typedef TNGrammer TInput;
    typedef typename TNGrammer::TValue TNGrammerOutValue;
    typedef typename TNGrammer::TOut TNGrammerOut;
    typedef TVectorizerType<TNGrammerOutValue> TVectorizer;
    typedef typename TVectorizer::TInValue TVectorizerInValue;
    typedef typename TVectorizer::TOutValue TVectorizerOutValue;
    typedef typename TVectorizer::TIn TVectorizerIn;
    typedef typename TVectorizer::TOut TVectorizerOut;
    typedef typename TAlgorithm::TIn TAlgorithmIn;
    typedef typename TAlgorithm::TOut TAlgorithmOut;
    typedef TAlgorithmOut TOutput;

public:
    TSimhashCalculator(const TSimHashVersion& version)
        : Version(version)
        , NGrammerOut()
        , VectorizerOut()
        , AlgorithmOut()
        , NGrammer()
        , Vectorizer()
        , Algorithm()
        , VectorizerOutputSize(0)
    {
        NGrammer.Reset(new TNGrammer(&NGrammerOut));
        Vectorizer.Reset(new TVectorizer(&NGrammerOut, &VectorizerOut));
        Algorithm.Reset(new TAlgorithm(Version, &VectorizerOut, &AlgorithmOut));
    }

public:
    TNGrammer& Input() {
        return *NGrammer.Get();
    }

    void Calculate() {
        Algorithm->ClearInput();
        Algorithm->ClearOutput();

        VectorizerOutputSize = Vectorizer->Vectorize();

        Algorithm->Calculate();
    }

    const TNGrammerOut& NGrammerOutput() const {
        return NGrammerOut;
    }

    const TVectorizerOut& VectorizerOutput() const {
        return VectorizerOut;
    }

    const TOutput& Output() const {
        return AlgorithmOut;
    }

    TSimHashVersion GetVersion() const {
        TSimHashVersion version =
            ::GetVersion<TNGrammerType, TVectorizerType, TAlgorithm>(Version.RandomVersion);

        return version;
    }

    ui32 GetBitCount() const {
        return Algorithm->GetBitCount();
    }

    ui32 GetVectorizerOutputSize() const {
        return VectorizerOutputSize;
    }

private:
    const TSimHashVersion& Version;

    TNGrammerOut NGrammerOut;
    TVectorizerOut VectorizerOut;
    TAlgorithmOut AlgorithmOut;

    THolder<TNGrammer> NGrammer;
    THolder<TVectorizer> Vectorizer;
    THolder<TAlgorithm> Algorithm;

    ui32 VectorizerOutputSize;
};
