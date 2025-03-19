#pragma once
#include <kernel/common_server/api/normalization/abstract/normalizer.h>

namespace NCS {
    namespace NNormalizer {
        class TStringNormalizerNumeric: public IStringNormalizer {
        private:
            using TBase = IStringNormalizer;
            static TBase::TFactory::TRegistrator<TStringNormalizerNumeric> Registrator;
        protected:
            virtual bool DoNormalize(const TStringBuf sbValue, TString& result) const override;
        public:
            virtual NCS::NScheme::TScheme GetScheme(const IBaseServer& /*server*/) const override {
                NCS::NScheme::TScheme result;
                return result;
            }

            Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& /*jsonInfo*/) override {
                return true;
            }
            virtual NJson::TJsonValue SerializeToJson() const override {
                return NJson::JSON_MAP;
            }

            static TString GetTypeName() {
                return "numeric";
            }

            virtual TString GetClassName() const override {
                return GetTypeName();
            }
        };
    }
}
