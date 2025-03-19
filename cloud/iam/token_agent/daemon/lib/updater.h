#pragma once

#include <filesystem>
#include <thread>
#include <mutex>

#include <util/datetime/base.h>

#include "iam_token_client.h"
#include "role_cache.h"
#include "token_service.h"

class TFsPath;
namespace NTokenAgent {
    class TConfig;
    class TIamTokenClient;
    class TTokenUpdater {
    public:
        TTokenUpdater(const TConfig& config, std::shared_ptr<TIamTokenClient> iamTokenClient, TRoleCache& roleCache);
        ~TTokenUpdater();

        void ScheduleUpdateImmediately();

    private:
        TVector<std::filesystem::path> GetRoles(const std::filesystem::path& rolesPath) const;
        void ReadFsCache(const std::filesystem::path& rolesPath, const std::string& rolesPrefix);
        void Update();
        void UpdateRoles(const std::filesystem::path& rolesPath, const std::string& rolesPrefix, std::set<std::string>& unknown_roles);
        std::filesystem::path FsCachePath;
        std::filesystem::path RolesPath;
        std::filesystem::path GroupRolesPath;
        std::shared_ptr<TIamTokenClient> IamTokenClient;
        TRoleCache& RoleCache;
        TDuration UpdateInterval;
        std::timed_mutex UpdateMutex;
        std::thread UpdateThread;
        std::atomic_bool ForceUpdate;
    };
}
