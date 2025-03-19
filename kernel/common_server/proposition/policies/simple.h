#pragma once
#include "abstract.h"

namespace NCS {
    namespace NPropositions {
        class TSimplePropositionPolicy: public IPropositionPolicy {
        private:
            static TFactory::TRegistrator<TSimplePropositionPolicy> Registrator;
            CS_ACCESS(TSimplePropositionPolicy, ui32, NeedApprovesCount, 2);
        public:
            virtual NCS::NScheme::TScheme GetScheme(const IBaseServer& server) const override;

            virtual NJson::TJsonValue SerializeToJson() const override {
                NJson::TJsonValue result = NJson::JSON_MAP;
                result.InsertValue("app_count", NeedApprovesCount);
                return result;
            }
            Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) override {
                if (!TJsonProcessor::Read(jsonInfo, "app_count", NeedApprovesCount)) {
                    return false;
                }
                return true;
            }

            virtual EVerdict BuildFinalVerdict(const TDBProposition& proposition, const TVector<TDBVerdict>& verdicts) const override;

            static TString GetTypeName() {
                return "simple";
            }

            virtual TString GetClassName() const override {
                return GetTypeName();
            }

            TSimplePropositionPolicy() = default;
            TSimplePropositionPolicy(const ui32 appCount)
                : NeedApprovesCount(appCount)
            {

            }

            bool IsValid(const IBaseServer& server) const override;
        };
    }
}

