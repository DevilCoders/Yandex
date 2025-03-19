#pragma once
#include <kernel/common_server/processors/common/handler.h>

namespace NCS {

    namespace NPropositions {
        class TProposedActionContainer;
        class TDBVerdict;
    }

    namespace NHandlers {

        enum class EPropositionAction: ui64 {
            Add = 1 /* "add" */,
            Modify = 1 << 1 /* "modify" */,
            Observe = 1 << 2 /* "observe" */,
            Remove = 1 << 3 /* "remove" */,
            Confirm = 1 << 4 /* "confirm" */,
            HistoryObserve = 1 << 5 /* "history_observe" */,
        };

        class TPropositionPermissions: public TAdministrativePermissionsImpl<EPropositionAction, ui64> {
        private:
            using TBase = TAdministrativePermissionsImpl<EPropositionAction, ui64>;
            TSet<TString> Categories;
            TSet<TString> ClassNames;
            static TFactory::TRegistrator<TPropositionPermissions> Registrator;
        protected:
            virtual NFrontend::TScheme DoGetScheme(const IBaseServer& server) const override;
        public:
            virtual NJson::TJsonValue SerializeToJson() const override;
            Y_WARN_UNUSED_RESULT virtual bool DeserializeFromJson(const NJson::TJsonValue& info) override;

            static TString GetTypeName() {
                return "proposition";
            }

            virtual TString GetClassName() const override {
                return GetTypeName();
            }

            bool Check(const EObjectAction& action) const;
            bool Check(const EObjectAction& action, const NPropositions::TProposedActionContainer& container) const;
            bool Check(const EObjectAction& action, const NPropositions::TDBVerdict& veridct) const;
        };
    }
}
