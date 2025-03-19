#pragma once

#include "host_stats.h"
#include "answer_models.h"

#include <util/memory/blob.h>
#include <kernel/dssm_applier/nn_applier/lib/layers.h>
#include <kernel/relevfml/rank_models_factory.h>

namespace NSnippets {

    struct TFactorDataDynamicOptions {
        TBlob HostStats;
        TBlob AnswerModels;
        TBlob RuFactSnippetDssmApplierModel;
        TBlob TomatoDssmApplierModel;
        TSimpleSharedPtr<IRankModelsFactory> BaseDynamicModels;
        TString DefaultExps;

        bool HasHostStats() const {
            return !HostStats.IsNull();
        }

        bool HasAnswerModels() const {
            return !AnswerModels.IsNull();
        }

        bool HasRuFactSnippetDssmApplier() const {
            return !RuFactSnippetDssmApplierModel.IsNull();
        }

        bool HasTomatoDssmApplier() const {
            return !TomatoDssmApplierModel.IsNull();
        }

        bool Empty() const {
            return !((HasHostStats() && HasAnswerModels()) || HasRuFactSnippetDssmApplier());
        }
    };

    struct TFactorDataDynamic {
        THostStats HostStats;
        TAnswerModels AnswerModels;
        NNeuralNetApplier::TModel RuFactSnippetDssmApplier;
        bool RuFactSnippetDssmApplierEmpty = true;
        TSimpleSharedPtr<IRankModelsFactory> BaseDynamicModels;
        TString DefaultExps;
        NNeuralNetApplier::TModel TomatoDssmApplier;
        bool TomatoDssmApplierEmpty = true;

        TFactorDataDynamic(const TFactorDataDynamicOptions& options) {
            if (!options.AnswerModels.Empty()) {
                AnswerModels.InitFromBlob(options.AnswerModels);
            }

            if (!options.HostStats.Empty()) {
                HostStats.InitFromBlob(options.HostStats);
            }

            if (options.HasRuFactSnippetDssmApplier()) {
                RuFactSnippetDssmApplier.LoadNoLock(options.RuFactSnippetDssmApplierModel);
                RuFactSnippetDssmApplier.Init();
                RuFactSnippetDssmApplierEmpty = false;
            }

            if (options.HasTomatoDssmApplier()) {
                TomatoDssmApplier.LoadNoLock(options.TomatoDssmApplierModel);
                TomatoDssmApplier.Init();
                TomatoDssmApplierEmpty = false;
            }

            if (options.BaseDynamicModels) {
                BaseDynamicModels = options.BaseDynamicModels;
            }

            DefaultExps = options.DefaultExps;
        }
    };
}
