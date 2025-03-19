#pragma once

#include "abstract.h"

#include <kernel/common_server/util/json_scanner/scanner.h>

namespace NCS {

    namespace NObfuscator {

        class TJsonExtractorByPath: public NCS::TJsonScannerByPath {
        private:
            using TBase = TJsonScannerByPath;
            NJson::TJsonValue& FinalJson;
            const TString& FinalPath;

        protected:
            virtual bool DoExecute(const NJson::TJsonValue& matchedValue) const override;

        public:
            TJsonExtractorByPath(const NJson::TJsonValue& initialJson, NJson::TJsonValue& finalJson, const TString& finalPath);
        };

        class TDataTransformationObfuscator: public IObfuscatorWithRules {
        public:
            static TString GetTypeName() {
                return "data_transformation_obfuscator";
            }

            virtual TString GetClassName() const override {
                return GetTypeName();
            }

            virtual NJson::TJsonValue DoSerializeToJson() const override;

            virtual bool DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) override;

        private:
            using TBase = IObfuscatorWithRules;

            virtual TString DoObfuscate(const TStringBuf str) const override;

            virtual NJson::TJsonValue DoObfuscate(const NJson::TJsonValue& inputJson) const override;

            virtual bool DoObfuscateInplace(NJson::TJsonValue& json) const override;

            static TFactory::TRegistrator<TDataTransformationObfuscator> Registrator;

            virtual NFrontend::TScheme DoGetScheme(const IBaseServer& /*server*/) const override;

        private:
            using TPathMap = TMap<TString, TString>;

            CSA_DEFAULT(TDataTransformationObfuscator, TPathMap, PathMappings);
        };

    }

}
