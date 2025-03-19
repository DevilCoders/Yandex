#include "configured.h"
#include <kernel/common_server/emulation/abstract/http.h>

namespace NCS {

    TConfiguredEmulationsManagerConfig::TFactory::TRegistrator<TConfiguredEmulationsManagerConfig> TConfiguredEmulationsManagerConfig::Registrator(TConfiguredEmulationsManagerConfig::GetTypeName());

    void TConfiguredEmulationsManagerConfig::DoToString(IOutputStream& os) const {
        os << "<Cases>" << Endl;
        for (auto&& i : EmulationCases) {
            os << "<Case>" << Endl;
            i.ToString(os);
            os << "</Case>" << Endl;
        }
        os << "</Cases>" << Endl;
    }

    void TConfiguredEmulationsManagerConfig::DoInit(const TYandexConfig::Section* section) {
        auto children = section->GetAllChildren();
        auto it = children.find("Cases");
        if (it != children.end() && it->second) {
            auto cases = it->second->GetAllChildren();
            for (auto&& [_, caseInfo] : cases) {
                TEmulationCaseContainer eCase;
                eCase.Init(caseInfo);
                EmulationCases.emplace_back(std::move(eCase));
            }
        }
    }

    THolder<IEmulationsManager> TConfiguredEmulationsManagerConfig::BuildManager(const IBaseServer& /*server*/) const {
        return MakeHolder<TConfiguredEmulationsManager>(*this);
    }

    TMaybe<NUtil::THttpReply> TConfiguredEmulationsManager::GetHttpReply(const IReplyContext& request) const {
        for (auto&& i : Config.GetEmulationCases()) {
            auto httpCase = i.GetPtrAs<IEmulationHttpCase>();
            if (!httpCase) {
                continue;
            }
            if (httpCase->CheckHttpRequest(request)) {
                return httpCase->GetHttpReply(request);
            }
        }
        return Nothing();
    }
}
