#include "tvm.h"

#include <kernel/common_server/library/logging/events.h>
#include <kernel/common_server/auth/common/tvm_config.h>
#include <kernel/common_server/abstract/frontend.h>

NJson::TJsonValue TTvmAuthInfo::GetInfo() const {
    NJson::TJsonValue result;
    if (UserId) {
        result["user_id"] = UserId;
    }
    return result;
}

TTvmAuthModule::TTvmAuthModule(TAtomicSharedPtr<NTvmAuth::TTvmClient> client, const TTvmAuthConfig& config)
    : Client(client)
    , Config(config)
{
}

IAuthInfo::TPtr TTvmAuthModule::DoRestoreAuthInfo(IReplyContext::TPtr requestContext) const {
    if (!requestContext) {
        return nullptr;
    }
    const auto& rd = requestContext->GetBaseRequestData();
    const auto& headers = rd.HeadersIn();
    const auto p = headers.find("X-Ya-Service-Ticket");
    if (p == headers.end()) {
        return MakeAtomicShared<TTvmAuthInfo>("header X-Ya-Service-Ticket is missing");
    }
    const TString& serializedTicket = p->second;

    const auto up = headers.find("X-Ya-User-Ticket");
    const TStringBuf serializedUserTicket = up != headers.end() ? up->second : TStringBuf();
    try {
        if (!Client) {
            return MakeAtomicShared<TTvmAuthInfo>("TVM client is missing");
        }

        if (!Config.GetTicketPass() || !serializedTicket.StartsWith(Config.GetTicketPass())) {
            const NTvmAuth::TCheckedServiceTicket serviceTicket = Client->CheckServiceTicket(serializedTicket);
            if (serviceTicket.GetStatus() != NTvmAuth::ETicketStatus::Ok) {
                return MakeAtomicShared<TTvmAuthInfo>("incorrect service ticket status: " + ToString(serviceTicket.GetStatus()));
            }
            NTvmAuth::TTvmId clientId = serviceTicket.GetSrc();
            if (!Config.GetAcceptedClientIds().contains(clientId)) {
                return MakeAtomicShared<TTvmAuthInfo>("ClientId " + ToString(clientId) + " is forbidden");
            }
        }

        if (Config.GetUserId()) {
            return MakeAtomicShared<TTvmAuthInfo>(true, Config.GetUserId(), Config.GetServiceId());
        }

        TString userId;
        if (Config.GetUserIdHeader()) {
            const auto it = headers.find(Config.GetUserIdHeader());
            if (it != headers.end()) {
                userId = it->second;
            }
            TFLEventLog::Log("detected user auth info")("user_id", !!userId ? userId : "<no header>");
        }
        if (serializedUserTicket) {
            NTvmAuth::TCheckedUserTicket userTicket = Client->CheckUserTicket(serializedUserTicket);
            if (userTicket.GetStatus() != NTvmAuth::ETicketStatus::Ok) {
                return MakeAtomicShared<TTvmAuthInfo>("incorrect user ticket status: " + ToString(static_cast<int>(userTicket.GetStatus())));
            }
            const auto newUserId = ToString(userTicket.GetDefaultUid());
            if (userId && newUserId != userId) {
                return MakeAtomicShared<TTvmAuthInfo>("Inconsistent userId: " + userId + " and " + newUserId);
            }
            userId = newUserId;
        }

        if (!userId) {
            userId = Config.GetDefaultUserId();
        }

        if (!userId) {
            return MakeAtomicShared<TTvmAuthInfo>("neither UserTicket nor default user are provided");
        }

        return MakeAtomicShared<TTvmAuthInfo>(true, userId, Config.GetServiceId());
    } catch (...) {
        return MakeAtomicShared<TTvmAuthInfo>("cannot check ServiceTicket: " + CurrentExceptionMessage());
    }
}

THolder<IAuthModule> TTvmAuthConfig::DoConstructAuthModule(const IBaseServer* server) const {
    if (!server) {
        ERROR_LOG << "nullptr IBaseServer" << Endl;
        return nullptr;
    }
    auto client = server->GetTvmManager()->GetTvmClient(TvmClientName);
    if (!client) {
        ERROR_LOG << "cannot find TvmClient for TvmClientName " << TvmClientName << Endl;
        return nullptr;
    }
    return MakeHolder<TTvmAuthModule>(client, *this);
}

void TTvmAuthConfig::DoInit(const TYandexConfig::Section* section) {
    if (!section) {
        WARNING_LOG << "nullptr YandexConfig section" << Endl;
        return;
    }

    const auto& directives = section->GetDirectives();
    {
        StringSplitter(directives.Value("AcceptedClientIds", JoinStrings(AcceptedClientIds.begin(), AcceptedClientIds.end(), ","))).SplitBySet(", ").SkipEmpty().ParseInto(&AcceptedClientIds);
        if (!directives.GetNonEmptyValue("TvmClientName", TvmClientName)) {
            TvmClientName = ::ToString(directives.Value("SelfClientId", 0));
        }
        UserId = directives.Value("UserId", UserId);
        DefaultUserId = directives.Value("DefaultUserId", DefaultUserId);
        ServiceId = directives.Value("ServiceId", ServiceId);
        TicketPass = directives.Value("TicketPass", TicketPass);
        UserIdHeader = directives.Value("UserIdHeader", UserIdHeader);

        {
            TString tmpAttrs;
            tmpAttrs = directives.Value<TString>("BBAttributes", tmpAttrs);
            if (tmpAttrs) {
                BBAttributes = SplitString(tmpAttrs, ",");
            }
        }
    }
}

void TTvmAuthConfig::DoToString(IOutputStream& os) const {
    os << "AcceptedClientIds: " << JoinStrings(AcceptedClientIds.begin(), AcceptedClientIds.end(), ",") << Endl;
    os << "TvmClientName: " << TvmClientName << Endl;
    os << "UserId: " << UserId << Endl;
    os << "ServiceId: " << ServiceId << Endl;
    os << "TicketPass: " << TicketPass << Endl;
    os << "UserIdHeader: " << UserIdHeader << Endl;
    os << "DefaultUserId: " << DefaultUserId << Endl;
    os << "BBAttributes: " << JoinStrings(BBAttributes, ",") << Endl;
}

TTvmAuthConfig::TFactory::TRegistrator<TTvmAuthConfig> TTvmAuthConfig::Registrator("tvm");

