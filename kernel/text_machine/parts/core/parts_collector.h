#pragma once

#include "types.h"
#include "parts_base.h"

#include <kernel/text_machine/parts/common/scatter.h>

#include <kernel/text_machine/module/module_def.inc>

namespace NTextMachine {
namespace NCore {
    MACHINE_PARTS(Core) {
        UNIT_INFO_BEGIN(TCollectorUnit)
        UNIT_INFO_END()

        UNIT(TCollectorUnit) {
            UNIT_STATE {
            };

            UNIT_PROCESSOR {
            public:
                UNIT_PROCESSOR_METHODS

                void Scatter(TScatterMethod::NotifyQueryFeatures, const TNotifyQueryFeaturesInfo& info) {
                    if (IInfoCollector* collector = Vars<TCoreUnit>().Collector) {
                        collector->AddQueryFeatures(
                            info.Expansion,
                            info.Index,
                            info.Ids,
                            {info.Features.begin(), info.Features.end()});
                    }
                }
            };
        };
    } // MACHINE_PARTS(Core)
} // NCore
} // NTextMachine

#include <kernel/text_machine/module/module_undef.inc>
