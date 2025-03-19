#include <filesystem>
#include <fstream>
#include <library/cpp/testing/gtest/gtest.h>
#include <yaml-cpp/yaml.h>

#include <role.h>

TEST(Role, CreateEmpty) {
    YAML::Node yaml;
    const auto& role = NTokenAgent::TRole("user", yaml);

    EXPECT_EQ(role.GetName(), "user");
    EXPECT_THAT(role.GetToken().GetValue(), testing::IsEmpty());
    EXPECT_THAT(role.GetServiceAccountId(), testing::IsEmpty());
    EXPECT_THAT(role.GetKeyId(), testing::IsEmpty());
    EXPECT_EQ(role.GetKeyHandle(), 0UL);
}

TEST(Role, MasterToken) {
    const auto& yaml = YAML::Load("token: TOKEN");
    const auto& role = NTokenAgent::TRole("root", yaml);

    EXPECT_EQ(role.GetName(), "root");
    EXPECT_EQ(role.GetToken().GetValue(), "TOKEN");
    EXPECT_EQ(role.GetToken().GetExpiresAt(), TInstant::ParseIso8601("3000-01-01T00:00:00"));
}

TEST(Role, MasterTokenExplicitExpiresAt) {
    const auto& yaml = YAML::Load("token: TOKEN\nexpiresAt: 2020-02-02T20:00:02");
    const auto& role = NTokenAgent::TRole("root", yaml);

    EXPECT_EQ(role.GetName(), "root");
    EXPECT_EQ(role.GetToken().GetValue(), "TOKEN");
    EXPECT_EQ(role.GetToken().GetExpiresAt(), TInstant::ParseIso8601("2020-02-02T20:00:02"));
}

TEST(Role, ServiceAccount) {
    const auto& yaml = YAML::Load("serviceAccountId: SA_ID\nkeyId: KEY_ID\nkeyHandle: 42");
    const auto& role = NTokenAgent::TRole("sa", yaml);

    EXPECT_EQ(role.GetName(), "sa");
    EXPECT_THAT(role.GetToken().GetValue(), testing::IsEmpty());
    EXPECT_EQ(role.GetServiceAccountId(), "SA_ID");
    EXPECT_EQ(role.GetKeyId(), "KEY_ID");
    EXPECT_EQ(role.GetKeyHandle(), 42UL);
}

TEST(Role, FromFile) {
    const auto& tmp_dir_name = "token-agent-temp-prefix-" + std::to_string(std::rand());
    const auto& profile_dir = std::filesystem::path(testing::TempDir()) / tmp_dir_name;
    const auto& profile = profile_dir / "service.yaml";

    std::filesystem::create_directory(profile_dir);
    std::ofstream tmp_file(profile.c_str());
    tmp_file << "token: TOKEN";
    tmp_file.close();

    const auto& role = NTokenAgent::TRole::FromFile(testing::TempDir(), profile);
    std::filesystem::remove_all(profile_dir);

    EXPECT_EQ(role.GetName(), tmp_dir_name + "/service");
    EXPECT_EQ(role.GetToken().GetValue(), "TOKEN");
    EXPECT_THAT(role.GetServiceAccountId(), testing::IsEmpty());
    EXPECT_THAT(role.GetKeyId(), testing::IsEmpty());
    EXPECT_EQ(role.GetKeyHandle(), 0UL);
}
