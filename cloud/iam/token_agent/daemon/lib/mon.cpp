#include <library/cpp/monlib/dynamic_counters/page.h>
#include <library/cpp/monlib/service/pages/version_mon_page.h>

#include "config.h"
#include "mon.h"

namespace NTokenAgent {
    static const std::string ROLE_GROUP_NAME("role");
    static const std::string AGE_NAME("age");
    static const std::string DENIED_NAME("denied");
    static const std::string REQUEST_NAME("request");
    static const std::string TPM_AGENT_ERROR("tpm_agent_error");
    static const std::string TOKEN_SERVICE_ERROR("token_service_error");
    static const std::string REVOKED_KEY("revoked_key");

    TMon::TMon(ui16 port)
        : MonService(port, "::1", 0)
        , Counters(new NMonitoring::TDynamicCounters())
    {
        MonService.Register(new NMonitoring::TVersionMonPage);
        MonService.Register(new NMonitoring::TDynamicCountersPage("counters", "Counters", Counters, [this]() {
            auto now = TInstant::Now();
            for (auto& role : AgeMap) {
                auto grp = Counters.Get()->GetSubgroup(ROLE_GROUP_NAME.c_str(), role.first.c_str());
                auto age = now - role.second;
                grp->GetCounter(AGE_NAME.c_str())->operator=((long)age.Seconds());
            }
        }));
    }

    TMon* TMon::Get() {
        return Singleton<THolder<TMon>>()->Get();
    }

    void TMon::Start(ui16 port) {
        auto holder = Singleton<THolder<TMon>>();
        holder->Reset(new TMon(port));
        holder->Get()->MonService.StartOrThrow();
    }

    void TMon::Stop() {
        Get()->MonService.Shutdown();
    }

    void TMon::ResetAge(const std::string& role, const TInstant& time) {
        TAgeHashMap::accessor accessor;
        if (AgeMap.find(accessor, role)) {
            accessor->second = time;
        } else {
            AgeMap.insert(std::make_pair(role, time));
        }
    }

    void TMon::Denied() {
        Counters.Get()->GetCounter(DENIED_NAME.c_str())->Inc();
    }

    void TMon::Request() {
        Counters.Get()->GetCounter(REQUEST_NAME.c_str())->Inc();
    }

    void TMon::TpmAgentError() {
        Counters.Get()->GetCounter(TPM_AGENT_ERROR.c_str())->Inc();
    }

    void TMon::TokenServiceError() {
        Counters.Get()->GetCounter(TOKEN_SERVICE_ERROR.c_str())->Inc();
    }

    void TMon::RevokedKey() {
        Counters.Get()->GetCounter(REVOKED_KEY.c_str())->Inc();
    }

}
