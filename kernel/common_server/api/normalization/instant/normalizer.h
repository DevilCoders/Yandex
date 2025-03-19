#pragma once
#include <kernel/common_server/api/normalization/abstract/normalizer.h>

namespace NCS {
    namespace NNormalizer {

        enum class EInstantView {
            RFC822 /* "rfc_822" */,
            ISO8601 /* "iso_8601" */,
            Seconds /* "seconds" */,
            Custom /* "custom" */
        };

        class TStringNormalizerInstant: public IStringNormalizer {
        private:
            using TBase = IStringNormalizer;
            static TBase::TFactory::TRegistrator<TStringNormalizerInstant> Registrator;
            CS_ACCESS(TStringNormalizerInstant, EInstantView, InstantView, EInstantView::RFC822);
            CSA_DEFAULT(TStringNormalizerInstant, TString, Template);
        protected:
            virtual bool DoNormalize(const TStringBuf sbValue, TString& result) const override;
        public:
            virtual NCS::NScheme::TScheme GetScheme(const IBaseServer& /*server*/) const override {
                NCS::NScheme::TScheme result;
                result.Add<TFSVariants>("instant_view").InitVariants<EInstantView>().SetDefault(::ToString(EInstantView::RFC822));
                result.Add<TFSString>("template");
                return result;
            }

            Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) override {
                if (!TJsonProcessor::ReadFromString(jsonInfo, "instant_view", InstantView)) {
                    return false;
                }
                if (!TJsonProcessor::Read(jsonInfo, "template", Template)) {
                    return false;
                }
                if (InstantView == EInstantView::Custom && !Template) {
                    TFLEventLog::Error("empty template for custom datetime format");
                    return false;
                }
                return true;
            }
            virtual NJson::TJsonValue SerializeToJson() const override {
                NJson::TJsonValue result = NJson::JSON_MAP;
                TJsonProcessor::WriteAsString(result, "instant_view", InstantView);
                TJsonProcessor::Write(result, "template", Template);
                return result;
            }

            static TString GetTypeName() {
                return "instant";
            }

            virtual TString GetClassName() const override {
                return GetTypeName();
            }
        };
    }
}
