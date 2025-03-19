#pragma once

#include <kernel/ethos/lib/logistic_regression/logistic_regression.h>
#include <kernel/ethos/lib/text_classifier/document_factory.h>
#include <kernel/ethos/lib/util/index_weighter.h>
#include <kernel/lemmas_merger/lemmas_merger.h>

#include <library/cpp/getopt/last_getopt.h>

#include <util/generic/ptr.h>
#include <util/ysaveload.h>

using namespace NEthos;

namespace NEthos_1562208 {

enum class EWordWeightsTransformation {
    NO_TRANSFORMATION /* "none" */,
    LOG /* "log" */,
    SQUARED_ROOT /* "sqrt" */,
    CONSTANT /* "constant" */,
};

class TLemmerOptions {
public:
    THolder<TDocumentFactory> DocumentFactory = MakeHolder<TSimpleHashFactory>();

    bool CustomFactorySetup = false;

public:
    TLemmerOptions() {}

    TLemmerOptions(const TLemmerOptions& options) {
        DocumentFactory = options.DocumentFactory->Clone();
        CustomFactorySetup = options.CustomFactorySetup;
    }

    TLemmerOptions& operator = (const TLemmerOptions& options) {
        DocumentFactory = options.DocumentFactory->Clone();
        CustomFactorySetup = options.CustomFactorySetup;
        return *this;
    }

    inline void Save(IOutputStream* s) const {
        SaveDocumentFactory(s, DocumentFactory.Get());
    }

    inline void Load(IInputStream* s) {
        DocumentFactory = LoadDocumentFactory(s);
    }

    void AddOpts(NLastGetopt::TOpts& opts) {
        opts.AddLongOption("lm-index", "lemmas merger index file name")
            .Optional()
            .Handler1(std::bind(&TLemmerOptions::SetupLemmasMergerFactory, this, std::placeholders::_1));

        opts.AddLongOption("kiwi-lemmer", "use kiwi lemmer)")
            .Optional()
            .NoArgument()
            .Handler0(std::bind(&TLemmerOptions::SetupDirectTextLemmerFactory, this));
    }

private:
    void SetupLemmasMergerFactory(const NLastGetopt::TOptsParser* p) {
        Y_VERIFY(!CustomFactorySetup, "You can't use lemmas merger and kiwi lemmer simultaneously");

        DocumentFactory = MakeHolder<TLemmasMergerFactory>(p->CurVal());
        CustomFactorySetup = true;
    }

    void SetupDirectTextLemmerFactory() {
        Y_VERIFY(!CustomFactorySetup, "You can't use lemmas merger and kiwi lemmer simultaneously");

        DocumentFactory = MakeHolder<TDirectTextLemmerFactory>();
        CustomFactorySetup = true;
    }
};

class TTextClassifierModelOptions {
public:
    EWordWeightsTransformation WordWeightsTransformation = EWordWeightsTransformation::NO_TRANSFORMATION;
    bool UseBigrams = false;
    TIndexWeighter IndexWeighter;
    float WeightsLowerBound = 0.f;

    TLemmerOptions LemmerOptions;
public:

    Y_SAVELOAD_DEFINE(WordWeightsTransformation, UseBigrams, IndexWeighter, WeightsLowerBound, LemmerOptions);

    void AddOpts(NLastGetopt::TOpts& opts) {
        LemmerOptions.AddOpts(opts);

        opts.AddLongOption("ww-transform", "word weights transformation (log|sqr|constant)")
            .Optional()
            .Handler1(std::bind(&TTextClassifierModelOptions::SetWordWeightsTransformation, this, std::placeholders::_1));

        opts.AddLongOption("bigrams", "use bigrams in model)")
            .Optional()
            .NoArgument()
            .StoreValue(&UseBigrams, true);

        opts.AddLongOption("factor", "sigmoid factor")
             .Optional()
             .Handler1(std::bind(&TTextClassifierModelOptions::SetFactor, this, std::placeholders::_1));

        opts.AddLongOption("threshold", "sigmoid threshold")
             .Optional()
             .Handler1(std::bind(&TTextClassifierModelOptions::SetThreshold, this, std::placeholders::_1));

        opts.AddLongOption("weights-lower-bound", "lower bound for saving weights")
             .Optional()
             .StoreResult(&WeightsLowerBound);
    }

private:
    void SetWordWeightsTransformation(const NLastGetopt::TOptsParser* p) {
        WordWeightsTransformation = FromString<EWordWeightsTransformation>(p->CurValStr());
    }

    void SetThreshold(const NLastGetopt::TOptsParser* p) {
        IndexWeighter.SetThreshold(FromString<size_t>(p->CurValStr()));
    }

    void SetFactor(const NLastGetopt::TOptsParser* p) {
        IndexWeighter.SetFactor(FromString<double>(p->CurValStr()));
    }
};

class TTextClassifierOptions {
public:
    TLinearClassifierOptions LogisticRegressionOptions;
    TTextClassifierModelOptions ModelOptions;
    // If empty use multi-classification
    TString TargetLabel;

public:
    void AddOpts(NLastGetopt::TOpts& opts) {
        LogisticRegressionOptions.AddOpts(opts);
        ModelOptions.AddOpts(opts);

        opts.AddLongOption("target-label", "target label for binary classification mode")
            .Optional()
            .StoreResult(&TargetLabel);
    }
};

}
