#pragma once
#include <kernel/common_server/util/json_processing.h>
#include <kernel/common_server/abstract/frontend.h>
#include <kernel/common_server/proposition/actions/abstract.h>
#include <kernel/common_server/settings/abstract/object.h>

namespace NCS {
    namespace NPropositions {

        class TSettingsAction: public IProposedActionWithDelay {
        private:
            using TBase = IProposedActionWithDelay;
            CSA_DEFAULT(TSettingsAction, TString, ActionId);
            CSA_MUTABLE_DEF(TSettingsAction, TVector<NFrontend::TSetting>, Settings);

        public:
            TSettingsAction() = default;

            virtual NJson::TJsonValue SerializeToJson() const override {
                NJson::TJsonValue result = TBase::SerializeToJson();
                JWRITE(result, "settings_action_id", ActionId)
                TJsonProcessor::WriteObjectsArray(result, "settings", Settings);
                return result;
            }

            virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonData) override {
                JREAD_STRING_OPT(jsonData, "settings_action_id", ActionId);
                if (!TJsonProcessor::ReadObjectsContainer(jsonData, "settings", Settings)) {
                    return false;
                }
                return TBase::DeserializeFromJson(jsonData);
            }

            virtual NCS::NScheme::TScheme GetScheme(const IBaseServer& server) const override {
                NCS::NScheme::TScheme scheme = TBase::GetScheme(server);
                scheme.Add<TFSString>("settings_action_id", "Идентификатор запроса").SetRequired(true);
                scheme.Add<TFSArray>("settings", "Настройки").SetElement(NFrontend::TSetting::GetScheme(server));
                return scheme;
            }

            virtual TString GetObjectId() const override {
                return ActionId;
            }

            static TString GetTypeName() {
                return "settings_action";
            }

            virtual TString GetCategoryId() const override {
                return GetTypeName();
            }

            virtual TString GetClassName() const override {
                return GetTypeName();
            }

        protected:
            virtual bool DoExecute(const TString& userId, const IBaseServer& server) const override {
                Y_UNUSED(userId);
                return server.GetSettings().SetValues(Settings, userId);
            }
        };

    }
}
