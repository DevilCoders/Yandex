#include "handler.h"

#include <kernel/common_server/server/server.h>

#include <kernel/common_server/notifications/manager.h>
#include <kernel/common_server/rt_background/manager.h>
#include <kernel/common_server/common/scheme.h>
#include <kernel/common_server/roles/abstract/role.h>
#include <kernel/common_server/roles/abstract/item.h>
#include <kernel/common_server/user_role/abstract/abstract.h>
#include <kernel/common_server/user_auth/abstract/abstract.h>
#include <kernel/common_server/proposition/object.h>
#include <kernel/common_server/processors/rt_background/handler.h>
#include <kernel/common_server/processors/tags/description.h>
#include <kernel/common_server/processors/roles/item.h>
#include <kernel/common_server/processors/roles/role.h>
#include <kernel/common_server/processors/user_role/user_role.h>
#include <kernel/common_server/processors/user_auth/user_auth.h>
#include <kernel/common_server/processors/notifiers/handler.h>
#include <kernel/common_server/processors/settings/handler.h>

namespace NCS {

    namespace NHandlers {

        void TServiceHandlersProcessor::ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr /*permissions*/) {
            const TSet<TString> selectedPathes = GetValuesSet<TString>(GetCgiParameters(), "endpoints");
            TVector<NCS::NScheme::THandlerScheme> schemas = GetServer().BuildHandlersScheme(selectedPathes);
            NCS::NScheme::THandlerSchemasCollection schemasCollection(std::move(schemas));
            g.SetExternalReportString(schemasCollection.SerializeToJson().GetStringRobust(), true);
        }

        void TServiceConstantsProcessor::ProcessRequestWithPermissions(TJsonReport::TGuard& g, TSystemUserPermissions::TPtr permissions) {
            if (Context->GetCgiParameters().Get("format") == "operations_info" || Context->GetCgiParameters().Get("format") == "operations_info_array") {
                NFrontend::TConstantsInfoReport cReport = GetServer().BuildInterfaceConstructor(permissions);
                if (Context->GetCgiParameters().Has("format", "operations_info")) {
                    g.AddReportElement("operationsInfo", cReport.GetReportMap());
                }
                if (Context->GetCgiParameters().Has("format", "operations_info_array")) {
                    g.AddReportElement("operationsInfoArray", cReport.GetReportArray());
                }
                return;
            }
            TSet<EConstantTraits> traitsSet;
            StringSplitter(Context->GetCgiParameters().Get("traits")).SplitBySet(", ").SkipEmpty().ParseInto(&traitsSet);
            const TSet<TString> ids = MakeSet<TString>(GetStrings(Context->GetCgiParameters(), "ids", false));
            if (traitsSet.empty()) {
                for (auto&& t : GetEnumAllValues<EConstantTraits>()) {
                    traitsSet.emplace(t);
                }
            }

            NFrontend::ESchemeFormat formatter = NFrontend::ESchemeFormat::Default;
            auto formatString = Context->GetCgiParameters().Get("format");
            ReqCheckCondition(!formatString || TryFromString(formatString, formatter), ConfigHttpStatus.UserErrorState, "Unknown schemas format");

            NFrontend::IElement::TReportTraits schemeTraits = NFrontend::IElement::ReportAll;

            if (traitsSet.contains(EConstantTraits::ServiceCustoms)) {
                g.MutableReport().AddReportElement("service_customs", GetServer().GetConstantsReport(schemeTraits, formatter));
            }

            {
                TMap<TString, TDBTagDescription> tagDescriptions;
                GetServer().GetTagDescriptionsManager().GetAllObjects(tagDescriptions);
                NJson::TJsonValue iFace = NJson::JSON_MAP;
                for (auto&& [i, d] : tagDescriptions) {
                    if (ids.size() && !ids.contains(i)) {
                        continue;
                    }
                    ITag::TPtr obj = ITag::TFactory::Construct(d->GetClassName());
                    if (obj) {
                        TDBTag dbTag(obj);
                        iFace.InsertValue(i, dbTag.GetScheme(GetServer()).SerializeToJson(schemeTraits, formatter));
                    }
                }
                g.MutableReport().AddReportElement("iface_tags", std::move(iFace));
            }

            g.SetCode(HTTP_OK);
        }
    }
}
