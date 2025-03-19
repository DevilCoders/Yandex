#include "permission.h"
#include <kernel/common_server/proposition/object.h>
#include <kernel/common_server/proposition/verdict.h>

namespace NCS {
    namespace NHandlers {
        TPropositionPermissions::TFactory::TRegistrator<TPropositionPermissions> TPropositionPermissions::Registrator(TPropositionPermissions::GetTypeName());

        NJson::TJsonValue TPropositionPermissions::SerializeToJson() const {
            NJson::TJsonValue result = TBase::SerializeToJson();
            TJsonProcessor::WriteContainerArrayStrings(result, "categories", Categories);
            TJsonProcessor::WriteContainerArrayStrings(result, "class_names", ClassNames);
            return result;
        }

        bool TPropositionPermissions::DeserializeFromJson(const NJson::TJsonValue & info) {
            if (!TBase::DeserializeFromJson(info)) {
                return false;
            }
            if (!TJsonProcessor::ReadContainer(info, "categories", Categories)) {
                return false;
            }
            if (!TJsonProcessor::ReadContainer(info, "class_names", ClassNames)) {
                return false;
            }
            return true;
        }

        NFrontend::TScheme TPropositionPermissions::DoGetScheme(const IBaseServer& server) const {
            NFrontend::TScheme result = TBase::DoGetScheme(server);
            result.Add<TFSArray>("categories").SetElement<TFSString>();
            result.Add<TFSVariants>("class_names").InitVariantsClass<NPropositions::IProposedAction>().SetMultiSelect(true);
            return result;
        }

        bool TPropositionPermissions::Check(const EObjectAction& action) const {
            return TBase::Check(action);
        }

        bool TPropositionPermissions::Check(const EObjectAction& action, const NPropositions::TProposedActionContainer& container) const {
            if (!TBase::Check(action)) {
                return false;
            }
            if (Categories.size() && !Categories.contains(container.GetCategoryId())) {
                return false;
            }
            if (ClassNames.size() && !ClassNames.contains(container.GetClassName())) {
                return false;
            }
            return true;
        }

        bool TPropositionPermissions::Check(const EObjectAction& action, const NPropositions::TDBVerdict& /*veridct*/) const {
            return TBase::Check(action);
        }

    }
}

