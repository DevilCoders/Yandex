#include <filesystem>
#include <iostream>
#include <yaml-cpp/yaml.h>
#include <library/cpp/logger/global/global.h>

#include "config.h"
#include "group.h"

namespace NTokenAgent {
    static const std::string DEFAULT_CACHE_PATH("/var/cache/yc/token-agent");
    static const std::string DEFAULT_KEY_PATH("/var/lib/yc/token-agent");
    static const std::string DEFAULT_UNIX_SOCKET_PATH("/var/run/yc/token-agent/socket");
    static const std::string DEFAULT_HTTP_UNIX_SOCKET_PATH("");
    static const std::string DEFAULT_TOKEN_SERVICE_HOST("ts.private-api.cloud.yandex.net");
    static const std::string DEFAULT_TPM_AGENT_HOST("");
    static const std::string DEFAULT_TPM_AGENT_TIMEOUT("60s");
    static const std::string DEFAULT_TOKEN_SERVICE_TIMEOUT("5s");
    static const std::string DEFAULT_CACHE_UPDATE_INTERVAL("10m");
    static const std::string DEFAULT_JWT_LIFETIME("1h");
    static const std::string DEFAULT_ROLES_PATH("roles.d");
    static const std::string DEFAULT_GROUP_ROLES_PATH("groups.d");
    static const std::string DEFAULT_JWT_AUDIENCE("https://iam.api.cloud.yandex.net/iam/v1/tokens");
    static const std::string DEFAULT_ROOT_CERTIFICATE_PATH("");
    constexpr bool DEFAULT_TOKEN_SERVICE_TLS{true};
    constexpr bool DEFAULT_TPM_AGENT_TLS{false};
    constexpr int DEFAULT_TOKEN_SERVICE_RETRIES{std::numeric_limits<int>::max()};
    constexpr int DEFAULT_TPM_AGENT_RETRIES{std::numeric_limits<int>::max()};
    constexpr int DEFAULT_TOKEN_SERVICE_PORT{4282};
    constexpr int DEFAULT_TPM_AGENT_PORT{USE_UNIX_DOMAIN_SOCKET};
    constexpr ui16 DEFAULT_MONITORING_PORT{1080};
    constexpr int DEFAULT_UNIX_SOCKET_LISTEN_BACKLOG{16};
    constexpr ui16 DEFAULT_UNIX_DOMAIN_SOCKET_MODE{0666};
    constexpr ui32 DEFAULT_UNIX_DOMAIN_SOCKET_GROUP{ui32(-1)};

    TListenUnixSocketConfig::TListenUnixSocketConfig(const YAML::Node& node,
                                                     const std::string& defaultSocketPath) {
        if (!node.IsDefined()) {
            Path = defaultSocketPath;
            Backlog = DEFAULT_UNIX_SOCKET_LISTEN_BACKLOG;
            Mode = DEFAULT_UNIX_DOMAIN_SOCKET_MODE;
            Group = DEFAULT_UNIX_DOMAIN_SOCKET_GROUP;
        } else {
            Path = node["path"].as<std::string>(defaultSocketPath);
            Backlog = node["backlog"].as<int>(DEFAULT_UNIX_SOCKET_LISTEN_BACKLOG);
            Mode = node["mode"].as<ui16>(DEFAULT_UNIX_DOMAIN_SOCKET_MODE);
            auto group_name = node["group"].as<std::string>("");
            if (!group_name.empty()) {
                const auto& group = TGroup::FromName(group_name.c_str());
                if (!group) {
                    ythrow TSystemError() << "No such group" << group_name.c_str();
                }
                Group = group.GetId();
            } else {
                Group = DEFAULT_UNIX_DOMAIN_SOCKET_GROUP;
            }
        }
    }

    TEndpointConfig::TEndpointConfig(const YAML::Node& node,
                                     const std::string& defaultHost,
                                     int defaultPort,
                                     bool defaultTls,
                                     const std::string& defaultTimeout,
                                     int defaultRetries) {
        if (!node.IsDefined()) {
            Port = defaultPort;
            Host = defaultHost;
            UseTls = defaultTls;
            Timeout = TDuration::Parse(defaultTimeout);
            Retries = defaultRetries;
            RootCertificatePath = DEFAULT_ROOT_CERTIFICATE_PATH;
        } else {
            Port = node["port"].as<int>(defaultPort);
            Host = node["host"].as<std::string>(defaultHost);
            UseTls = node["useTls"].as<bool>(defaultTls);
            Timeout = TDuration::Parse(
                node["timeout"].as<std::string>(defaultTimeout));
            Retries = node["retries"].as<int>(defaultRetries);
            RootCertificatePath =
                node["rootCertificatePath"].as<std::string>(DEFAULT_ROOT_CERTIFICATE_PATH);
        }
    }

    TConfig::TConfig(const std::string& path, const YAML::Node& node)
        : ListenUnixSocketConfig(node["listenUnixSocket"],
                                 DEFAULT_UNIX_SOCKET_PATH)
        , HttpListenUnixSocketConfig(node["httpListenUnixSocket"],
                                     DEFAULT_HTTP_UNIX_SOCKET_PATH)
        , TokenServiceEndpointConfig(node["tokenServiceEndpoint"],
                                     DEFAULT_TOKEN_SERVICE_HOST,
                                     DEFAULT_TOKEN_SERVICE_PORT,
                                     DEFAULT_TOKEN_SERVICE_TLS,
                                     DEFAULT_TOKEN_SERVICE_TIMEOUT,
                                     DEFAULT_TOKEN_SERVICE_RETRIES)
        , TpmAgentEndpointConfig(node["tpmAgentEndpoint"],
                                 DEFAULT_TPM_AGENT_HOST,
                                 DEFAULT_TPM_AGENT_PORT,
                                 DEFAULT_TPM_AGENT_TLS,
                                 DEFAULT_TPM_AGENT_TIMEOUT,
                                 DEFAULT_TPM_AGENT_RETRIES)
        , KeyPath(node["keyPath"].as<std::string>(DEFAULT_KEY_PATH))
        , CachePath(node["cachePath"].as<std::string>(DEFAULT_CACHE_PATH))
        , RolesPath(std::filesystem::path(path).parent_path() /
                    node["rolesPath"].as<std::string>(DEFAULT_ROLES_PATH))
        , GroupRolesPath(std::filesystem::path(path).parent_path() /
                    node["groupRolesPath"].as<std::string>(DEFAULT_GROUP_ROLES_PATH))
        , CacheUpdateInterval(TDuration::Parse(
              node["cacheUpdateInterval"].as<std::string>(DEFAULT_CACHE_UPDATE_INTERVAL)))
        , JwtLifetime(TDuration::Parse(
              node["jwtLifetime"].as<std::string>(DEFAULT_JWT_LIFETIME)))
        , JwtAudience(node["jwtAudience"].as<std::string>(DEFAULT_JWT_AUDIENCE))
        , MonitoringPort(node["monitoringPort"].as<ui16>(DEFAULT_MONITORING_PORT))
    {
        DEBUG_LOG << "Parsed config:\n"
                  << (std::stringstream() << node).str() << "\n";
    }

    TConfig TConfig::FromFile(const std::string& path) {
        INFO_LOG << "Loading config file " << path << "\n";

        return TConfig(path, YAML::LoadFile(path));
    }

}
