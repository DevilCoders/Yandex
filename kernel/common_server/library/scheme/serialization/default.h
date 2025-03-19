#pragma once
#include <kernel/common_server/util/types/camel_case.h>
#include <kernel/common_server/util/json_processing.h>
#include "abstract.h"

namespace NCS {
    namespace NScheme {

        class TDefaultSerializer: public ISchemeSerializer {
        private:
            using EReportTraits = IElement::EReportTraits;
            CS_ACCESS(TDefaultSerializer, IElement::TReportTraits, Traits, IElement::ReportAll);
            NJson::TJsonValue SerializeToJsonImpl(const TStructureImpl& scheme) const;
            NJson::TJsonValue SerializeToJsonImpl(const IElement& element) const;
        public:
            TDefaultSerializer(const IElement::TReportTraits traits = IElement::ReportAll)
                : Traits(traits)
            {
            }
            virtual NJson::TJsonValue SerializeToJson(const TScheme& scheme) const override;
        };
    }
}
