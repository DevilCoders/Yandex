#pragma once

#include <kernel/ethos/lib/data/dataset.h>
#include <kernel/ethos/lib/data/linear_model.h>
#include <kernel/ethos/lib/logistic_regression/logistic_regression.h>
#include "document_factory.h"
#include <kernel/ethos/lib/util/index_weighter.h>
#include <kernel/lemmas_merger/lemmas_merger.h>

#include <library/cpp/getopt/last_getopt.h>

#include <util/generic/ptr.h>
#include <util/ysaveload.h>

namespace NEthos {

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
        opts.AddLongOption("hashes-lemmer", "read documents as hash sequences")
            .Optional()
            .NoArgument()
            .Handler0(std::bind(&TLemmerOptions::SetupHashesFactory, this));

        opts.AddLongOption("lm-index", "lemmas merger index file name")
            .Optional()
            .Handler1(std::bind(&TLemmerOptions::SetupLemmasMergerFactory, this, std::placeholders::_1));

        opts.AddLongOption("kiwi-lemmer", "use kiwi lemmer")
            .Optional()
            .NoArgument()
            .Handler0(std::bind(&TLemmerOptions::SetupDirectTextLemmerFactory, this));
    }

    TDocumentFactory* GetDocumentFactory() const {
        return DocumentFactory.Get();
    }

    void MinimizeLemmasMerger(const TCompactSingleLabelFloatWeights& weights) {
        MinimizeLemmasMergerTemplate(weights);
    }

    void MinimizeLemmasMerger(const TCompactMultiLabelFloatWeights& weights) {
        MinimizeLemmasMergerTemplate(weights.GetWeightOffsets());
    }
private:
    template <typename THashesMapType>
    void MinimizeLemmasMergerTemplate(const THashesMapType& weights) {
        TLemmasMergerFactory* documentFactory = dynamic_cast<TLemmasMergerFactory*>(DocumentFactory.Get());
        if (!documentFactory) {
            return;
        }

        TCompactTrieBuilder<wchar16, ui64> minimizedLemmasMergerBuilder;

        NLemmasMerger::TLemmasMerger& lemmasMerger = documentFactory->LemmasMerger;
        NLemmasMerger::TLemmasMerger::TConstIterator lmIterator = lemmasMerger.Begin();
        for (; lmIterator != lemmasMerger.End(); ++lmIterator) {
            ui64 wordHash = lemmasMerger.Hash(lmIterator.GetValue());
            if (weights.Has(wordHash)) {
                minimizedLemmasMergerBuilder.Add(lmIterator.GetKey(), lmIterator.GetValue());
            }
        }

        TBufferOutput raw;
        minimizedLemmasMergerBuilder.Save(raw);

        TBufferOutput compacted;
        CompactTrieMinimize<TCompactTrie<>::TPacker>(compacted, raw.Buffer().Data(), raw.Buffer().Size(), false);

        lemmasMerger.Init(TBlob::FromBuffer(compacted.Buffer()));
    }

    void SetupHashesFactory() {
        Y_VERIFY(!CustomFactorySetup, "You can't use two different lemmers simultaneously");

        DocumentFactory = MakeHolder<THashFactory>();
        CustomFactorySetup = true;
    }

    void SetupLemmasMergerFactory(const NLastGetopt::TOptsParser* p) {
        Y_VERIFY(!CustomFactorySetup, "You can't use two different lemmers simultaneously");

        DocumentFactory = MakeHolder<TLemmasMergerFactory>(p->CurVal());
        CustomFactorySetup = true;
    }

    void SetupDirectTextLemmerFactory() {
        Y_VERIFY(!CustomFactorySetup, "You can't use twowi different lemmers simultaneously");

        DocumentFactory = MakeHolder<TDirectTextLemmerFactory>();
        CustomFactorySetup = true;
    }
};

class TTextClassifierModelOptions {
public:
    EWordWeightsTransformation WordWeightsTransformation = EWordWeightsTransformation::LOG;
    bool UseBigrams = false;
    TIndexWeighter IndexWeighter;

    TLemmerOptions LemmerOptions;
public:
    Y_SAVELOAD_DEFINE(WordWeightsTransformation, UseBigrams, IndexWeighter, LemmerOptions);

    void AddOpts(NLastGetopt::TOpts& opts) {
        LemmerOptions.AddOpts(opts);

        opts.AddLongOption("ww-transform", "word weights transformation (none|log|sqrt|constant)")
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
    TLinearClassifierOptions LinearClassifierOptions;
    TTextClassifierModelOptions ModelOptions;
    // If empty use multi-classification
    TString TargetLabel;
public:
    Y_SAVELOAD_DEFINE(LinearClassifierOptions,
                    ModelOptions,
                    TargetLabel);

    void AddOpts(NLastGetopt::TOpts& opts) {
        LinearClassifierOptions.AddOpts(opts);
        ModelOptions.AddOpts(opts);

        opts.AddLongOption("target-label", "target label for binary classification mode")
            .Optional()
            .StoreResult(&TargetLabel);
    }
};

}
