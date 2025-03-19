#pragma once

#include "switch_hit_codegen.h"
#include <kernel/text_machine/module/interfaces.h>
#include <kernel/lingboost/constants.h>

namespace NTextMachine {
namespace NCore {
    class TTextMachineInterfaces
        : public ::NModule::TMachineInterfaces
    {
    public:
        TTextMachineInterfaces() {
            using namespace ::NModule;

            AddProcessor("MatchAccumulator", TProcessorGenerator()
                .AddMethod(
                    TProcessorMethod("Clear")
                )
                .AddMethod(
                    TProcessorMethod("Update")
                        .AddArg("float", "match")
                        .AddArg("float", "value")
                )
            );

            AddProcessor("PositionlessProxy", TProcessorGenerator()
                .AddMethod(
                    TProcessorMethod("NewQuery")
                        .AddArg("TMemoryPool&", "pool")
                        .AddArg("const TQuery&", "query")
                         .SetOutOfLineInstantiated()
                )
                .AddMethod(
                    TProcessorMethod("NewDoc")
                         .SetOutOfLineInstantiated()
                )
                .AddMethod(
                    TProcessorMethod("AddHitSynonym")
                        .AddArg("size_t", "wordId")
                        .AddArg("size_t", "formId")
                        .AddArg("float", "value")
                )
                .AddMethod(
                    TProcessorMethod("FinishDoc")
                         .SetOutOfLineInstantiated()
                )
            );

            AddProcessor("PositionlessAccumulator", TProcessorGenerator()
                .AddMethod(
                    TProcessorMethod("NewMultiQuery")
                        .AddArg("TMemoryPool&", "pool")
                        .AddArg("size_t", "numBlocks")
                        .SetOutOfLineInstantiated()
                )
                .AddMethod(
                    TProcessorMethod("NewDoc")
                         .SetOutOfLineInstantiated()
                )
                .AddMethod(
                    TProcessorMethod("AddHitExact")
                        .AddArg("size_t", "wordId")
                        .AddArg("size_t", "formId")
                        .AddArg("float", "value")
                )
                .AddMethod(
                    TProcessorMethod("AddHitLemma")
                        .AddArg("size_t", "wordId")
                        .AddArg("size_t", "formId")
                        .AddArg("float", "value")
                )
                .AddMethod(
                    TProcessorMethod("AddHitOther")
                        .AddArg("size_t", "wordId")
                        .AddArg("size_t", "formId")
                        .AddArg("float", "value")
                )
                .AddMethod(
                    TProcessorMethod("FinishDoc")
                         .SetOutOfLineInstantiated()
                )
            );

            AddProcessor("AnnotationAccumulator", TProcessorGenerator()
                .AddMethod(
                    TProcessorMethod("NewQuery")
                        .AddArg("TMemoryPool&", "pool")
                        .AddArg("const TQueryInfo&", "info")
                        .SetOutOfLineInstantiated()
                )
                .AddMethod(
                    TProcessorMethod("NewDoc")
                        .AddArg("const TDocInfo&", "info")
                )
                .AddMethod(
                    TProcessorMethod("AddHit")
                        .AddArg("const THitInfo&", "info")
                        .SetOutOfLineInstantiated()
                )
                .AddMethod(
                    TProcessorMethod("FinishHit")
                        .AddArg("const THitInfo&", "info")
                        .SetOutOfLineInstantiated()
                )
                .AddMethod(
                    TProcessorMethod("StartAnnotation")
                        .AddArg("const TAnnotationInfo&", "info")
                        .SetOutOfLineInstantiated()
                )
                .AddMethod(
                    TProcessorMethod("FinishAnnotation")
                        .AddArg("const TAnnotationInfo&", "info")
                        .SetOutOfLineInstantiated()
                )
                .AddMethod(
                    TProcessorMethod("FinishDoc")
                        .AddArg("const TDocInfo&", "info")
                        .SetOutOfLineInstantiated()
                )
            );

            AddProcessor("PlaneAccumulator", TProcessorGenerator()
                .AddMethod(
                    TProcessorMethod("NewQuery")
                        .AddArg("TMemoryPool&", "pool")
                        .AddArg("const TQueryInfo&", "info")
                        .SetOutOfLineInstantiated()
                )
                .AddMethod(
                    TProcessorMethod("NewDoc")
                         .SetOutOfLineInstantiated()
                )
                .AddMethod(
                    TProcessorMethod("FinishDoc")
                )
                .AddMethod(
                    TProcessorMethod("AddHit")
                        .AddArg("const THitInfo&", "info")
                )
                .AddMethod(
                    TProcessorMethod("FinishHit")
                        .AddArg("const THitInfo&", "info")
                )
            );

            AddProcessor("BagOfWordsAccumulator", TProcessorGenerator()
                .AddMethod(
                    TProcessorMethod("NewBagOfWords")
                        .AddArg("TMemoryPool&", "pool")
                        .AddArg("const TBagOfWordsInfo&", "info")
                )
                .AddMethod(
                    TProcessorMethod("NewDoc")
                        .SetOutOfLineInstantiated()
                )
                .AddMethod(
                    TProcessorMethod("AddHit")
                        .AddArg("const THitInfo&", "info")
                )
                .AddMethod(
                    TProcessorMethod("StartAnnotation")
                        .AddArg("const TAnnotationInfo&", "info")
                )
                .AddMethod(
                    TProcessorMethod("FinishAnnotation")
                        .AddArg("const TAnnotationInfo&", "info")
                )
                .AddMethod(
                    TProcessorMethod("FinishDoc")
                        .SetOutOfLineInstantiated()
                )
            );

            AddProcessor("AggregatorArray", TProcessorGenerator()
                .AddMethod(
                    TProcessorMethod("Init")
                        .AddArg("const TInitInfo&", "info")
                )
                .AddMethod(
                    TProcessorMethod("Clear")
                )
                .AddMethod(
                    TProcessorMethod("AddFeatures")
                        .AddTypeArg("typename", "BufType")
                        .AddArg("const TAddFeaturesInfo<BufType>&", "info")
                )
                .AddMethod(
                    TProcessorMethod("CountFeatures").SetStatic()
                        .AddArg("const TCountFeaturesInfo&", "info")
                )
                .AddMethod(
                    TProcessorMethod("SaveFeatureIds").SetStatic()
                        .AddArg("const TSaveFeatureIdsInfo&", "info")
                )
                .AddMethod(
                    TProcessorMethod("SaveFeatures")
                        .AddArg("const TSaveFeaturesInfo&", "info")
                )
            );

            AddProcessor("Aggregator", TProcessorGenerator()
                .AddMethod(
                    TProcessorMethod("Init")
                        .AddArg("const TInitInfo&", "info")
                )
                .AddMethod(
                    TProcessorMethod("Clear")
                )
                .AddMethod(
                    TProcessorMethod("AddFeatures")
                        .AddArg("const TAddFeaturesInfo&", "info")
                )
                .AddMethod(
                    TProcessorMethod("CountFeatures").SetStatic()
                        .AddArg("const TCountFeaturesInfo&", "info")
                )
                .AddMethod(
                    TProcessorMethod("SaveFeatureIds").SetStatic()
                        .AddArg("const TSaveFeatureIdsInfo&", "info")
                )
                .AddMethod(
                    TProcessorMethod("SaveFeatures")
                        .AddArg("const TSaveFeaturesInfo&", "info")
                )
            );

            AddProcessor("Tracker", TProcessorGenerator()
                .AddMethod(
                    TProcessorMethod("NewMultiQuery")
                        .AddArg("TMemoryPool&", "pool")
                        .AddArg("const TMultiQueryInfo&", "info")
                        .SetOutOfLineInstantiated()
                )
                .AddMethod(
                    TProcessorMethod("NewQuery")
                        .AddArg("TMemoryPool&", "pool")
                        .AddArg("const TQueryInfo&", "info")
                        .SetOutOfLineInstantiated()
                )
                .AddMethod(
                    TProcessorMethod("NewDoc")
                        .AddArg("const TDocInfo&", "info")
                        .SetOutOfLineInstantiated()
                )
                .AddMethod(
                    TProcessorMethod("AddHit")
                        .AddArg("const THitInfo&", "info")
                        .SetOutOfLineInstantiated()
                )
                .AddMethod(
                    TProcessorMethod("AddStreamHit")
                        .AddTypeArg("EStreamType", "StreamType")
                        .AddArg("const THitInfo&", "info")
                        .AddArg("TStreamSelector<StreamType>", "")
                        .InstantiateAs(TInstantiateData("AddStreamHitBasic")
                            .UseTypeArg("StreamType")
                            .UseArg("info")
                        )
                )
                .AddMethod(
                    TProcessorMethod("FinishHit")
                        .AddArg("const THitInfo&", "info")
                )
                .AddMethod(
                    TProcessorMethod("AddBlockHit")
                        .AddArg("const TBlockHitInfo&", "info")
                )
                .AddMethod(
                    TProcessorMethod("AddStreamBlockHit")
                        .AddTypeArg("EStreamType", "StreamType")
                        .AddArg("const TBlockHitInfo&", "info")
                        .AddArg("TStreamSelector<StreamType>", "")
                        .InstantiateAs(TInstantiateData("AddStreamBlockHitBasic")
                            .UseTypeArg("StreamType")
                            .UseArg("info")
                        )
                )
                .AddMethod(
                    TProcessorMethod("FinishBlockHit")
                        .AddArg("const TBlockHitInfo&", "info")
                )
                .AddMethod(
                    TProcessorMethod("FinishDoc")
                        .AddArg("const TDocInfo&", "info")
                        .SetOutOfLineInstantiated()
                )
            );

            AddProcessor("Core", TProcessorGenerator()
                .AddMethod(
                    TProcessorMethod("NewMultiQuery").SetGuarded()
                        .AddArg("TMemoryPool&", "pool")
                        .AddArg("const TMultiQueryInfo&", "info")
                )
                .AddMethod(
                    TProcessorMethod("NewDoc").SetGuarded()
                        .AddArg("const TDocInfo&", "info")
                )
                .AddMethod(
                    TProcessorMethod("StartAnnotation").SetGuarded()
                        .AddArg("const TAnnotationInfo&", "info")
                )
                .AddMethod(
                    TProcessorMethod("AddBlockHit").SetGuarded()
                        .AddArg("const TBlockHitInfo&", "info")
                )
                .AddMethod(
                    TProcessorMethod("FinishBlockHit").SetGuarded()
                        .AddArg("const TBlockHitInfo&", "info")
                )
                .AddMethod(
                    TProcessorMethod("AddHit").SetGuarded()
                        .AddArg("const THitInfo&", "info")
                )
                .AddMethod(
                    TProcessorMethod("FinishHit").SetGuarded()
                        .AddArg("const THitInfo&", "info")
                )
                .AddMethod(
                    TProcessorMethod("FinishAnnotation").SetGuarded()
                        .AddArg("const TAnnotationInfo&", "info")
                )
                .AddMethod(
                    TProcessorMethod("FinishDoc").SetGuarded()
                        .AddArg("const TDocInfo&", "info")
                )
                .AddMethod(
                    TProcessorMethod("SaveFeatures").SetGuarded()
                        .AddArg("const TSaveFeaturesInfo&", "info")
                )
                .AddMethod(
                    TProcessorMethod("SwitchHitCodegen").SetGuarded()
                        .AddArg("const TMultiHit&", "hit")
                        .AddArg("EStreamType", "streamType")
                        .AddArg("const TQueriesHelper&", "queriesHelper")
                        .SetCustomGenerator(new TSwitchHitCodegen)
                )
            );
        }
    };

    using TStaticTextMachineInterfaces = ::NModule::TStaticMachineInterfaces<TTextMachineInterfaces>;
} // NTextMachine
} // NCore
