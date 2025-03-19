#include <fstream>
#include <library/cpp/testing/gtest/gtest.h>
#include <yaml-cpp/yaml.h>

#include <config.h>

TEST(Config, CreateEmpty) {
    YAML::Node yaml;
    const auto& config = NTokenAgent::TConfig("/path/to/config", yaml);
    const auto& ts_endpoint = config.GetTokenServiceEndpoint();

    EXPECT_EQ(config.GetRolesPath(), "/path/to/roles.d");
    EXPECT_EQ(ts_endpoint.GetAddress(), "ts.private-api.cloud.yandex.net:4282");
    EXPECT_EQ(ts_endpoint.GetRootCertificatePath(), "");
    EXPECT_EQ(ts_endpoint.IsInproc(), false);
    EXPECT_EQ(ts_endpoint.IsUnixDomainSocket(), false);
    EXPECT_EQ(ts_endpoint.IsUseTls(), true);
}

TEST(Config, EndpointUseTls) {
    const auto& yaml = YAML::Load("tpmAgentEndpoint:\n  useTls: true");
    const auto& config = NTokenAgent::TConfig("/path/to/config", yaml);

    EXPECT_EQ(config.GetTpmAgentEndpoint().IsUseTls(), true);
}

TEST(Config, EndpointRetries) {
    const auto& yaml = YAML::Load("tpmAgentEndpoint:\n  retries: 42");
    const auto& config = NTokenAgent::TConfig("/path/to/config", yaml);

    EXPECT_EQ(config.GetTpmAgentEndpoint().GetRetries(), 42);
}

TEST(Config, EndpointTimeout) {
    const auto& yaml = YAML::Load("tpmAgentEndpoint:\n  timeout: 42");
    const auto& config = NTokenAgent::TConfig("/path/to/config", yaml);

    EXPECT_EQ(config.GetTpmAgentEndpoint().GetTimeout(), TDuration::Parse("42s"));
}

TEST(Config, EndpointUnixSocket) {
    const auto& yaml = YAML::Load("tpmAgentEndpoint:\n  host: /var/run/yc/tpm-agent");
    const auto& config = NTokenAgent::TConfig("/path/to/config", yaml);

    EXPECT_EQ(config.GetTpmAgentEndpoint().IsUnixDomainSocket(), true);
    EXPECT_EQ(config.GetTpmAgentEndpoint().GetAddress(), "unix:///var/run/yc/tpm-agent");
}

TEST(Config, EndpointRootCertificatePath) {
    const auto& yaml = YAML::Load("tokenServiceEndpoint:\n  rootCertificatePath: /etc/ssl/certs/root.pem");
    const auto& config = NTokenAgent::TConfig("/path/to/config", yaml);

    EXPECT_EQ(config.GetTokenServiceEndpoint().GetRootCertificatePath(), "/etc/ssl/certs/root.pem");
}
