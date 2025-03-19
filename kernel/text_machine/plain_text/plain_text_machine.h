#pragma once

#include "plain_text_aggregator.h"
#include "plain_text_model_if.h"

#include <kernel/text_machine/interface/text_machine.h>

#include <util/generic/string.h>

namespace NTextMachine {
namespace NPlainText {
    // Class that produces features for pairs like (plain query, title).
    // The expected usage looks like this:
    // TPlainTextTracker tracker(models);
    // tracker.NeqQuery(q1);
    // tracker.NewDoc();
    // tracker.AddDocAnnotation("title", "web page title");
    // tracker.FinishDoc();
    // tracker.CalcFeatures(features);
    //
    // tracker.NewDoc();
    // etc...
    class TPlainTextTracker {
    public:
        // Does not take ownership of models.
        TPlainTextTracker(const TPlainTextModelsVector& models);

        void NewQuery(const TQuery* query);
        void NewDoc();
        void AddDocAnnotation(const TAnnotation* annotation);
        void FinishDoc();

        void SaveFeatures(TVector<float>& features);
        void SaveFeatureIds(TFFIds& features);

        static void SaveFeatureIds(const TPlainTextModelsVector& models, TFFIds& ids);

    private:
        enum ETrackerState {
            NOTHING_DONE,
            STARTED_QUERY,
            STARTED_DOC
        };

    private:
        TVector<TAnnotation> DocAnnotations;

        TPlainTextModelsVector Models;
        TVector<TFFIdWithHash> Ids;

        ETrackerState State;
        const TQuery* Query;
    };

    // The same as TPlainTextTracker but works for TMultiQuery.
    class TPlainTextMachine {
    public:
        // Does not take ownership of models.
        TPlainTextMachine(const TPlainTextModelsVector& models);
        void AddExpansionType(EExpansionType expType);

        void NewQuery(const TMultiQuery& multiQuery);
        void NewDoc();
        void AddDocAnnotation(const TAnnotation* annotation);
        void FinishDoc();
        void SaveFeatures(TFeatures& features);
        void SaveFeatureIds(TFFIds& ids);

    private:
        TVector<EExpansionType> ExpansionTypes;
        TPlainTextModelsVector Models;
        TVector<TAutoPtr<TPlainTextTracker>> Trackers;
        const TMultiQuery* MultiQuery;
        TFFIds BaseIds, Ids;
        TOptFeatures Features;
        TMemoryPool Pool{32UL << 10};
        TPlainTextAggregator Aggregator;
    };
} // NPlainText
} // NTextMachine
