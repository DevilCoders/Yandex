#include <library/cpp/logger/global/global.h>

#include "http_server/ucred_option.hpp"
#include "http_token_service.h"
#include "mon.h"
#include "role_cache.h"
#include "user.h"
#include "group.h"

namespace NTokenAgent {
    void THttpTokenService::RegisterHandlers(NHttpServer::TRequestHandler& requestHandler) const {
        requestHandler.AddHandler("GET", "/tokenAgent/v1/token", [this](auto& connection) { return GetIamToken(connection); });
    }

    NHttpServer::TResponse THttpTokenService::GetIamToken(const NHttpServer::TConnection& connection) const {
        TMon::Get()->Request();

        NHttpServer::TUcredOption ucredOption{};
        connection.GetSocket().get_option(ucredOption);
        DEBUG_LOG << "User id " << ucredOption.Uid()
                  << " group id " << ucredOption.Gid()
                  << " process id " << ucredOption.Pid()
                  << "\n";

        const auto& user = TUser::FromId(ucredOption.Uid());
        std::string user_name;
        if (user) {
            user_name = USER_PREFIX + user.GetName();
            TToken token = RoleCache.FindToken(user_name);
            if (token.IsEmpty()) {
                std::string group_name = GROUP_PREFIX + TGroup::FromId(user.GetGroupId()).GetName();
                token = RoleCache.FindToken(group_name);
            }
            if (!token.IsEmpty()) {
                NHttpServer::TResponse response;

                response.Content
                    .append(R"({"iam_token":")")
                    .append(token.GetValue())
                    .append(R"(","expires_at":")")
                    .append(token.GetExpiresAt().ToString().c_str())
                    .append(R"("})");

                response.AddHeader("Content-Type", "application/json");

                return response;
            } else {
                WARNING_LOG << "User '" << user_name << "' id " << ucredOption.Uid()
                            << " has no associated service account\n";
            }
        } else {
            user_name = std::string("id:").append(std::to_string(ucredOption.Uid()));
            WARNING_LOG << "User " << user_name << " is unknown\n";
        }

        TMon::Get()->Denied();
        return NHttpServer::TResponse::GetStockReply(
            NHttpServer::TResponse::EStatus::Forbidden,
            "User '" + user_name + "' access denied");
    }
}
