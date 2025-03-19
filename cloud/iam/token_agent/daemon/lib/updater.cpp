#include <algorithm>
#include <chrono>
#include <exception>
#include <fstream>
#include <iostream>
#include <set>
#include <thread>
#include <library/cpp/logger/global/global.h>

#include "config.h"
#include "mon.h"
#include "role.h"
#include "updater.h"
#include <random>
#include "server.h"

// Randomizing first update time by -10%
const auto RANDOMIZED_FIRST_UPDATE_INTERVAL = 0.1;

namespace NTokenAgent {
    TTokenUpdater::TTokenUpdater(const TConfig& config, std::shared_ptr<TIamTokenClient> iamTokenClient, TRoleCache& roleCache)
            : FsCachePath(config.GetCachePath())
            , RolesPath(config.GetRolesPath())
            , GroupRolesPath(config.GetGroupRolesPath())
            , IamTokenClient(iamTokenClient)
            , RoleCache(roleCache)
            , UpdateInterval(config.GetCacheUpdateInterval())
            , ForceUpdate(false)
    {
        std::filesystem::create_directories(FsCachePath);
        std::filesystem::permissions(FsCachePath, std::filesystem::perms::owner_all);

        ReadFsCache(RolesPath, USER_PREFIX);
        ReadFsCache(GroupRolesPath, GROUP_PREFIX);

        INFO_LOG << "Starting update thread\n";
        UpdateMutex.lock();
        UpdateThread = std::thread(&TTokenUpdater::Update, this);
    }

    TTokenUpdater::~TTokenUpdater() {
        try {
            UpdateMutex.unlock();
            if (UpdateThread.joinable()) {
                INFO_LOG << "Stopping update thread\n";
                UpdateThread.join();
            }
        } catch (const std::exception& ex) {
            ERROR_LOG << "Failed to release TokenUpdater resources" << ex.what();
        }
    }

    void NTokenAgent::TTokenUpdater::ScheduleUpdateImmediately()
    {
        ForceUpdate = true;
        UpdateMutex.unlock();
        while (ForceUpdate) {
            usleep(10);
        }
        UpdateMutex.lock();
    }

    TVector<std::filesystem::path> TTokenUpdater::GetRoles(const std::filesystem::path &rolesPath) const {
        TVector<std::filesystem::path> paths;
        if (std::filesystem::exists(rolesPath)) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(rolesPath)) {
                if (entry.is_regular_file()) {
                    if (entry.path().extension() == ".yaml") {
                        paths.push_back(entry.path());
                    } else {
                        WARNING_LOG << "File '" << entry.path().c_str() << "' has no 'yaml' suffix, ignored.\n";
                    }
                }
            }
        } else {
            WARNING_LOG << rolesPath.c_str() << " does not exist\n";
        }
        return paths;
    }

    void TTokenUpdater::ReadFsCache(const std::filesystem::path& rolesPath, const std::string& rolesPrefix) {
        auto deadline = TInstant::Now() + UpdateInterval;
        for (const auto& path : GetRoles(rolesPath)) {
            const auto& role = TRole::FromFile(rolesPath, path);
            const auto& role_name = rolesPrefix + role.GetName();
            TToken iam_token;
            if (!role.GetToken().IsEmpty()) {
                iam_token = role.GetToken();
                INFO_LOG << "Loaded master IAM token for " << role_name
                         << " expires at " << iam_token.GetExpiresAt() << "\n";
            } else if (role.GetKeyId().empty()) {
                WARNING_LOG << "Role " << path.c_str() << " has no key id.\n";
                continue;
            } else {
                auto entry = FsCachePath / role_name;
                if (!std::filesystem::exists(entry)) {
                    DEBUG_LOG << "FS cache entry " << entry.c_str() << " does not exist\n";
                } else {
                    std::ifstream stream(entry.c_str());
                    std::string token, expires_at;
                    std::getline(stream, token);
                    std::getline(stream, expires_at);

                    iam_token = TToken(token, TInstant::ParseIso8601(expires_at.c_str()));
                    INFO_LOG << "Loaded IAM token '" << iam_token.GetValue().substr(0, 8)
                             << "...' expires at " << iam_token.GetExpiresAt()
                             << " from " << entry.c_str() << "\n";
                }
            }

            // If the cached token is missing or is about to be expired, we must create a new one
            if (iam_token.IsEmpty() || iam_token.GetExpiresAt() < deadline) {
                INFO_LOG << "IAM token for " << role_name << " is outdated, creating new one\n";
                try {
                    iam_token = IamTokenClient->CreateIamToken(role);
                } catch (const yexception& ex) {
                    WARNING_LOG << "Failed to update token for " << role_name << ": " << ex << "\n";
                    continue;
                }
            }

            RoleCache.Insert(role_name, iam_token);
            TMon::Get()->ResetAge(role_name);
        }
    }

    void TTokenUpdater::UpdateRoles(const std::filesystem::path& rolesPath, const std::string& rolesPrefix, std::set<std::string>& unknown_roles) {
        for (auto const& path : GetRoles(rolesPath)) {
            if (!TServer::IsRunning()) {
                return;
            }
            if (ForceUpdate) {
                // Force update request in the middle of update - we should restart.
                return;
            }
            try {
                const auto& role = TRole::FromFile(rolesPath, path);
                const auto& role_name = rolesPrefix + role.GetName();
                unknown_roles.erase(role_name);
                TToken iam_token;
                if (!role.GetToken().IsEmpty()) {
                    iam_token = role.GetToken();
                    DEBUG_LOG << role_name << ": master iam token expires at "
                              << iam_token.GetExpiresAt() << "\n";
                } else if (role.GetKeyId().empty()) {
                    WARNING_LOG << "Role " << path.c_str() << " has no key id.\n";
                    continue;
                } else {
                    iam_token = IamTokenClient->CreateIamToken(role);
                    DEBUG_LOG << role_name << ": updated iam token '"
                              << iam_token.GetValue().substr(0, 8)
                              << "...' expires at " << iam_token.GetExpiresAt() << "\n";
                    TMon::Get()->ResetAge(role_name);
                    // Save to the FS cache.
                    auto fs_entry = FsCachePath / role_name;
                    std::ofstream(fs_entry.c_str()) << iam_token.GetValue() << "\n"
                                                    << iam_token.GetExpiresAt().ToString();
                }

                TToken oldToken = RoleCache.Update(role_name, iam_token);
                if (oldToken.IsEmpty()) {
                    INFO_LOG << "Iam token for user '" << role_name << "' inserted\n";
                }
            } catch (const yexception& ex) {
                ERROR_LOG << "Error updating " << path.c_str() << ": " << ex << "\n";
            } catch (const std::exception& ex) {
                // This is not expected. All c++ exceptions should be wrapped in yexceptions.
                ERROR_LOG << "Error updating " << path.c_str() << ": " << ex.what() << "\n";
            }
        }
    }

    void TTokenUpdater::Update() {
        INFO_LOG << "Update thread started\n";
        auto interval = std::chrono::microseconds(UpdateInterval.MicroSeconds());
        auto now = std::chrono::system_clock::now();
        std::mt19937 engine(now.time_since_epoch().count());
        auto random_start_time_coefficient = (1 - RANDOMIZED_FIRST_UPDATE_INTERVAL) +
                                             engine() / (engine.max() / RANDOMIZED_FIRST_UPDATE_INTERVAL);
        auto deadline = now + random_start_time_coefficient * interval;
        while (TServer::IsRunning()) {
            if (!ForceUpdate && UpdateMutex.try_lock_until(deadline)) {
                UpdateMutex.unlock();
            }
            ForceUpdate = false;
            INFO_LOG << "Updating tokens\n";

            // Save all keys to local set.
            std::set<std::string> unknown_roles(RoleCache.GetRoleIds());

            UpdateRoles(RolesPath, USER_PREFIX, unknown_roles);
            UpdateRoles(GroupRolesPath, GROUP_PREFIX, unknown_roles);

            if (!TServer::IsRunning()) {
                INFO_LOG << "Update thread stopped\n";
                return;
            }

            if (ForceUpdate) {
                // Restarting update process.
                continue;
            }

            deadline += interval;

            // Remove entries for nonexistent users.
            for (const auto& role_name : unknown_roles) {
                INFO_LOG << "User '" << role_name << "' removed\n";
                RoleCache.Erase(role_name);
            }
            INFO_LOG << "Done tokens update\n";
        }
        INFO_LOG << "Update thread stopped\n";
    }

}
