#pragma once
#include <kernel/common_server/api/normalization/abstract/normalizer.h>

namespace NCS {
    namespace NNormalizer {
        class TCharFilter {
        private:
            CS_ACCESS(TCharFilter, char, MinChar, 'a');
            CS_ACCESS(TCharFilter, char, MaxChar, 'z');
        public:
            static NCS::NScheme::TScheme GetScheme(const IBaseServer& /*server*/) {
                NCS::NScheme::TScheme result;
                result.Add<TFSString>("min_char");
                result.Add<TFSString>("max_char");
                return result;
            }
            bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
                TString minCharString;
                if (!TJsonProcessor::Read(jsonInfo, "min_char", minCharString)) {
                    return false;
                }
                if (minCharString.size() != 1) {
                    TFLEventLog::Error("incorrect min_char size != 1")("min_char", minCharString);
                    return false;
                }
                TString maxCharString;
                if (!TJsonProcessor::Read(jsonInfo, "max_char", maxCharString)) {
                    return false;
                }
                if (maxCharString.size() != 1) {
                    TFLEventLog::Error("incorrect max_char size != 1")("min_char", minCharString);
                    return false;
                }
                MinChar = minCharString[0];
                MaxChar = maxCharString[0];
                return true;
            }
            NJson::TJsonValue SerializeToJson() const {
                NJson::TJsonValue result = NJson::JSON_MAP;
                TJsonProcessor::Write(result, "min_char", TString(&MinChar, 1));
                TJsonProcessor::Write(result, "max_char", TString(&MaxChar, 1));
                return result;
            }
        };

        enum class ERegisterNormalization {
            LowerCase /* "lower_case" */,
            UpperCase /* "upper_case" */,
            None /* "none" */
        };

        class TFilterStringNormalizer: public IStringNormalizer {
        private:
            using TBase = IStringNormalizer;
            static TBase::TFactory::TRegistrator<TFilterStringNormalizer> Registrator;
            CS_ACCESS(TFilterStringNormalizer, ERegisterNormalization, RegisterNormalization, ERegisterNormalization::LowerCase);
            CSA_DEFAULT(TFilterStringNormalizer, TVector<TCharFilter>, Filters);
        protected:
            virtual bool DoNormalize(const TStringBuf sbValue, TString& result) const override;
        public:
            virtual NCS::NScheme::TScheme GetScheme(const IBaseServer& server) const override {
                NCS::NScheme::TScheme result;
                result.Add<TFSVariants>("register_normalization").InitVariants<ERegisterNormalization>();
                result.Add<TFSArray>("filter_characters").SetElement(TCharFilter::GetScheme(server));
                return result;
            }

            Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) override {
                if (!TJsonProcessor::ReadFromString(jsonInfo, "register_normalization", RegisterNormalization)) {
                    return false;
                }
                if (!TJsonProcessor::ReadObjectsContainer(jsonInfo, "filter_characters", Filters)) {
                    return false;
                }
                return true;
            }
            virtual NJson::TJsonValue SerializeToJson() const override {
                NJson::TJsonValue result = NJson::JSON_MAP;
                TJsonProcessor::WriteAsString(result, "register_normalization", RegisterNormalization);
                TJsonProcessor::WriteObjectsArray(result, "filter_characters", Filters);
                return result;
            }

            static TString GetTypeName() {
                return "filter";
            }

            virtual TString GetClassName() const override {
                return GetTypeName();
            }
        };
    }
}
