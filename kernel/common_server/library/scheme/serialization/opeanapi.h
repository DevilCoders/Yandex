#pragma once
#include <kernel/common_server/util/types/camel_case.h>
#include <kernel/common_server/util/json_processing.h>
#include "abstract.h"

namespace NCS {
    namespace NScheme {
        class TOpenApiSerializer: public ISchemeSerializer {
        private:
            CS_ACCESS(TOpenApiSerializer, IElement::TReportTraits, Traits, IElement::ReportAll);
            mutable TMap<TString, NJson::TJsonValue> ComponentSchemas;
            mutable ui32 ConflictsCounter = 0;
            TString StoreAsRef(NJson::TJsonValue&& jsonInfo, const TString& firstName) const;
        public:
            TOpenApiSerializer(const IElement::TReportTraits traits = IElement::ReportAll)
                : Traits(traits)
            {

            }

            virtual NJson::TJsonValue SerializeToJson(const TScheme& scheme) const override;
            virtual NJson::TJsonValue SerializeToJson(const THandlerSchemasCollection& schemasCollection) const override;
        private:
            NJson::TJsonValue SerializeToJson(const TVector<TScheme>& scheme) const;
            NJson::TJsonValue SerializeToJson(const IElement& element) const;
            void SerializeWideToJson(const TFSWideVariants& element, NJson::TJsonValue& result) const;
            NJson::TJsonValue SerializeToJson(const THandlerRequestBody& scheme) const;
            NJson::TJsonValue SerializeToJson(const THandlerScheme& scheme) const;
            NJson::TJsonValue SerializeToJson(const THandlerResponse& scheme) const;
            NJson::TJsonValue SerializeToJson(const THandlerRequestParameters& scheme) const;
            NJson::TJsonValue SerializeToJson(const THandlerRequestMethod& scheme) const;
            void SerializeToJsonImpl(const TStructureImpl& scheme, NJson::TJsonValue& result) const;
        };
    }
}
