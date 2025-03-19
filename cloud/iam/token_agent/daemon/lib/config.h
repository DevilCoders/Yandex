#pragma once

#include <string>

#include <util/datetime/base.h>

namespace YAML {
    class Node;
}

namespace NTokenAgent {
    const std::string USER_PREFIX = "user:";
    const std::string GROUP_PREFIX = "group:";
    constexpr static int USE_UNIX_DOMAIN_SOCKET{-1};

    class TListenUnixSocketConfig {
    public:
        TListenUnixSocketConfig(const YAML::Node& node, const std::string& defaultSocketPath);

        [[nodiscard]] const std::string& GetPath() const {
            return Path;
        }

        [[nodiscard]] int GetBackLog() const {
            return Backlog;
        }

        [[nodiscard]] ui16 GetMode() const {
            return Mode;
        }

        [[nodiscard]] ui32 GetGroup() const {
            return Group;
        }

    private:
        std::string Path;
        int Backlog;
        ui16 Mode;
        ui32 Group;
    };

    class TEndpointConfig {
    public:
        TEndpointConfig(const YAML::Node& node,
                        const std::string& defaultHost,
                        int defaultPort,
                        bool defaultTls,
                        const std::string& defaultDuration,
                        int defaultRetries);

        const std::string GetAddress() const {
            return IsInproc()
                       ? "<inpoc server>"
                   : IsUnixDomainSocket()
                       ? "unix://" + Host
                       : Host + ":" + std::to_string(Port);
        }

        bool IsUseTls() const {
            return UseTls;
        }

        bool IsInproc() const {
            return Host.empty();
        }

        bool IsUnixDomainSocket() const {
            return Port == USE_UNIX_DOMAIN_SOCKET;
        }

        const TDuration& GetTimeout() const {
            return Timeout;
        }

        int GetRetries() const {
            return Retries;
        }

        const std::string GetRootCertificatePath() const {
            return RootCertificatePath;
        }

    private:
        std::string Host;
        int Port;
        bool UseTls;
        TDuration Timeout;
        int Retries;
        std::string RootCertificatePath;
    };

    class TConfig {
    public:
        TConfig(const std::string& path, const YAML::Node& node);

        const TListenUnixSocketConfig& GetListenUnixSocket() const {
            return ListenUnixSocketConfig;
        }

        const TListenUnixSocketConfig& GetHttpListenUnixSocket() const {
            return HttpListenUnixSocketConfig;
        }

        const TEndpointConfig& GetTokenServiceEndpoint() const {
            return TokenServiceEndpointConfig;
        }

        const TEndpointConfig& GetTpmAgentEndpoint() const {
            return TpmAgentEndpointConfig;
        }

        const std::string& GetKeyPath() const {
            return KeyPath;
        }

        const std::string& GetCachePath() const {
            return CachePath;
        }

        const std::string& GetRolesPath() const {
            return RolesPath;
        }

        const std::string& GetGroupRolesPath() const {
            return GroupRolesPath;
        }

        const TDuration& GetCacheUpdateInterval() const {
            return CacheUpdateInterval;
        }

        const TDuration& GetJwtLifetime() const {
            return JwtLifetime;
        }

        const std::string& GetJwtAudience() const {
            return JwtAudience;
        }

        ui16 GetMonitoringPort() const {
            return MonitoringPort;
        }

        static TConfig FromFile(const std::string& path);

    private:
        TListenUnixSocketConfig ListenUnixSocketConfig;
        TListenUnixSocketConfig HttpListenUnixSocketConfig;
        TEndpointConfig TokenServiceEndpointConfig;
        TEndpointConfig TpmAgentEndpointConfig;
        std::string KeyPath;
        std::string CachePath;
        std::string RolesPath;
        std::string GroupRolesPath;
        TDuration CacheUpdateInterval;
        TDuration JwtLifetime;
        std::string JwtAudience;
        ui16 MonitoringPort;
    };

}
