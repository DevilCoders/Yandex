#pragma once

#include <kernel/relevfml/rank_models_factory_fwd.h>

#include <util/generic/map.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

class TEventLogFrame;
class TWordFilter;

namespace NNeuralNetApplier {
    class TModel;
}

namespace NSnippets {
    namespace NProto {
        class TSnipReqParams;
    }
    struct IGeobase;
    class THostStats;
    class ISentsFilter;
    class TAnswerModels;

    class TConfigParams {
    public:
        const NProto::TSnipReqParams* SRP = nullptr;
        const TWordFilter* StopWords = nullptr;
        TString DefaultExps;
        TVector<TString> AppendExps;
        const IGeobase* Geobase = nullptr;
        const ISentsFilter* SentsFilter = nullptr;
        const TAnswerModels* AnswerModels = nullptr;
        const THostStats* HostStats = nullptr;
        const NNeuralNetApplier::TModel* RuFactSnippetDssmApplier = nullptr;
        const NNeuralNetApplier::TModel* TomatoDssmApplier = nullptr;
        TEventLogFrame* EventLog = nullptr;
        ui32 DocId = 0;
        TString ReqId;
        const IRankModelsFactory* DynamicModels = nullptr;
    };
}
