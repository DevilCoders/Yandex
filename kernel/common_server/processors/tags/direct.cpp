#include "direct.h"
#include <kernel/common_server/util/algorithm/container.h>

namespace NCS {
    namespace NHandlers {
        IItemPermissions::TFactory::TRegistrator<TTagPermissions> TTagPermissions::Registrator(TTagPermissions::GetTypeName());

        NFrontend::TScheme TTagPermissions::DoGetScheme(const IBaseServer& server) const {
            NFrontend::TScheme result = TBase::DoGetScheme(server);
            result.Add<TFSVariants>("tags").SetVariants(server.GetTagDescriptionsManager().GetAvailableNames()).SetMultiSelect(true).AddVariant("*");
            result.Add<TFSString>("object_tags_filter").SetRequired(false);
            result.Add<TFSVariants>("performing_policy").InitVariants<EPerformingPolicy>().SetDefault(::ToString(EPerformingPolicy::OnlyMineOrEmpty));
            return result;
        }

        NJson::TJsonValue TTagPermissions::SerializeToJson() const {
            NJson::TJsonValue result = TBase::SerializeToJson();
            TJsonProcessor::WriteAsString(result, "performing_policy", PerformingPolicy);
            TJsonProcessor::WriteContainerArrayStrings(result, "tags", Tags);
            if (ObjectTagsFilter) {
                TJsonProcessor::Write(result, "object_tags_filter", ObjectTagsFilter->SerializeToString());
            }
            return result;
        }

        Y_WARN_UNUSED_RESULT bool TTagPermissions::DeserializeFromJson(const NJson::TJsonValue& info) {
            if (!TBase::DeserializeFromJson(info)) {
                return false;
            }
            if (info["object_tags_filter"].IsString()) {
                NTags::TObjectFilter filter;
                if (!filter.DeserializeFromString(info["object_tags_filter"].GetString())) {
                    TFLEventLog::Error("cannot parse filter");
                    return false;
                }
                ObjectTagsFilter = std::move(filter);
            }
            if (!TJsonProcessor::ReadFromString(info, "performing_policy", PerformingPolicy)) {
                return false;
            }
            return TJsonProcessor::ReadContainer(info, "tags", Tags);
        }

    }
}
