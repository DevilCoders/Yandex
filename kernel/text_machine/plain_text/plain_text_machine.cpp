#include "plain_text_machine.h"

#include <util/generic/vector.h>

namespace {
    using namespace NTextMachine;
    using namespace NTextMachine::NPlainText;

    TString GetQueryString(const NTextMachine::TQuery* query) {
        TString result;
        for (size_t i = 0; i < query->Words.size(); ++i) {

            if (i != 0) {
                result += " ";
            }
            result += query->Words[i].Text;
        }
        return result;
    }

    bool CompareModelsByName(IPlainTextModel* model1, IPlainTextModel* model2) {
        return TStringBuf(model1->Name()) < TStringBuf(model2->Name());
    }
}

namespace NTextMachine {
namespace NPlainText {
    TPlainTextTracker::TPlainTextTracker(const TPlainTextModelsVector& models) {
        Models.reserve(models.size());
        Ids.reserve(models.size());

        for (auto& model : models) {
            Models.push_back(model);
            Ids.emplace_back(TStringBuf(model->Name()));
        }
        Sort(Models.begin(), Models.end(), &CompareModelsByName);
        State = NOTHING_DONE;
    }

    void TPlainTextTracker::NewQuery(const TQuery* query) {
        Y_ASSERT(query);
        Y_ASSERT(State != STARTED_DOC);
        State = STARTED_QUERY;
        Query = query;
    }

    void TPlainTextTracker::NewDoc() {
        Y_ASSERT(State == STARTED_QUERY);
        State = STARTED_DOC;
        DocAnnotations.clear();
    }

    void TPlainTextTracker::FinishDoc() {
        Y_ASSERT(State == STARTED_DOC);
        State = STARTED_QUERY;
    }

    void TPlainTextTracker::AddDocAnnotation(const TAnnotation* annotation) {
        DocAnnotations.push_back(*annotation);
    }

    void TPlainTextTracker::SaveFeatures(TVector<float>& features) {
        Y_ASSERT(Query);
        features.reserve(features.size() + Models.size());
        TString queryString = GetQueryString(Query);
        for (size_t i = 0; i < Models.size(); ++i) {
            features.push_back(Models[i]->Compute(queryString, DocAnnotations));
        }
    }

    void TPlainTextTracker::SaveFeatureIds(TFFIds& ids) {
        SaveFeatureIds(Models, ids);
    }

    void TPlainTextTracker::SaveFeatureIds(const TPlainTextModelsVector& models, TFFIds& ids) {
        ids.reserve(ids.size() + models.size());
        for (const auto& model : models) {
            ids.emplace_back(TStringBuf(model->Name()));
        }
    }

    TPlainTextMachine::TPlainTextMachine(const TPlainTextModelsVector& models)
        : Models(models)
    {
        TPlainTextTracker::SaveFeatureIds(Models, BaseIds);
        Aggregator.Init(Pool, BaseIds.size());
        Ids.resize(Aggregator.GetNumFeatures());
        TFFIdsBuffer idsBuffer(Ids.data(), Ids.size());
        TPlainTextAggregator::SaveFeatureIds(TConstFFIdsBuffer::FromVector(BaseIds), idsBuffer);
    }

    void TPlainTextMachine::AddExpansionType(EExpansionType expType) {
        ExpansionTypes.push_back(expType);
    }

    void TPlainTextMachine::NewQuery(const TMultiQuery& multiQuery) {
        MultiQuery = &multiQuery;
        Trackers.resize(MultiQuery->Queries.size());
        for (size_t i = 0; i < MultiQuery->Queries.size(); ++i) {
            Trackers[i].Reset(new TPlainTextTracker(Models));
            Trackers[i]->NewQuery(&MultiQuery->Queries[i]);
        }
    }

    void TPlainTextMachine::NewDoc() {
        for (auto& tracker : Trackers) {
            tracker->NewDoc();
        }
    }

    void TPlainTextMachine::AddDocAnnotation(const TAnnotation* annotation) {
        for (auto& tracker : Trackers) {
            tracker->AddDocAnnotation(annotation);
        }
    }

    void TPlainTextMachine::FinishDoc() {
        for (auto& tracker : Trackers) {
            tracker->FinishDoc();
        }
    }

    void TPlainTextMachine::SaveFeatures(TFeatures& features) {
        const size_t numFeatures = Aggregator.GetNumFeatures() * ExpansionTypes.size();
        features.reserve(features.size() + numFeatures);

        TVector<float> values;

        for (auto expType : ExpansionTypes) {
            Aggregator.Clear();

            for (size_t queryId : xrange(Trackers.size())) {
                const auto& query = MultiQuery->Queries[queryId];
                if (query.ExpansionType != expType) {
                    continue;
                }
                values.clear();
                Trackers[queryId]->SaveFeatures(values);
                Aggregator.AddFeatures(TConstFloatsBuffer::FromVector(values), query.GetMaxValue());
            }

            values.resize(Aggregator.GetNumFeatures());
            auto buf = TFloatsBuffer::FromVector(values, EStorageMode::Empty);
            Aggregator.SaveFeatures(buf);

            features.resize(features.size() + buf.Count());

            const size_t offset = features.size();
            for (size_t i : xrange(offset, features.size())) {
                features[i].Value = buf[i - offset];
                features[i].Id = Ids[i - offset];
                features[i].Id.Set(expType);
            }
        }
    }

    void TPlainTextMachine::SaveFeatureIds(TFFIds& ids) {
        for (auto expType : ExpansionTypes) {
            const size_t offset = ids.size();
            TPlainTextTracker::SaveFeatureIds(Models, ids);
            for (size_t i : xrange(offset, ids.size())) {
                ids[i].Set(expType);
            }
        }
    }
} // NPlainText
} // NTextMachine
