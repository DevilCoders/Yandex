#include <fstream>

#include <library/cpp/logger/global/global.h>

#include "config.h"
#include "mon.h"
#include "token_service.h"
#include "user.h"
#include "group.h"

namespace NTokenAgent {
    extern ui32 GetUserId(const grpc::ServerContext* context);

    grpc::Status TTokenServiceImpl::GetToken(
        grpc::ServerContext* context,
        const NProtoIam::GetTokenRequest* request,
        NProtoIam::GetTokenResponse* response) {
        TMon::Get()->Request();
        auto user_id = GetUserId(context);
        std::string user_name;
        std::string group_name;
        const auto& user = TUser::FromId(user_id);
        if (user) {
            user_name = USER_PREFIX + user.GetName();
            group_name = GROUP_PREFIX + TGroup::FromId(user.GetGroupId()).GetName();
            if (!request->tag().empty()) {
                user_name += "/" + request->tag();
                group_name += "/" + request->tag();
            }
            auto token = RoleCache.FindToken(user_name);
            if (token.IsEmpty()) {
                token = RoleCache.FindToken(group_name);
            }
            if (!token.IsEmpty()) {
                response->set_iam_token(token.GetValue().c_str());
                response->mutable_expires_at()->set_seconds(long(token.GetExpiresAt().Seconds()));
                response->mutable_expires_at()->set_nanos(int(token.GetExpiresAt().NanoSecondsOfSecond()));
                return grpc::Status::OK;
            } else {
                WARNING_LOG << "Role '" << user_name << " " << group_name << "'"
                            << " id " << user_id << ":" << user.GetGroupId()
                            << " has no associated service account\n";
            }
        } else {
            user_name = std::string("id:").append(std::to_string(user_id));
            WARNING_LOG << "Role '" + user_name + " " + group_name + "' is unknown\n";
        }

        TMon::Get()->Denied();
        return grpc::Status(grpc::StatusCode::PERMISSION_DENIED,
                            "Role '" + user_name + " " + group_name + "' access denied");
    }

}
