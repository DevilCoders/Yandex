#include <library/cpp/testing/gtest/gtest.h>
#include "iam_token_client_utils.h"
#include <library/cpp/logger/global/global.h>
#include <yaml-cpp/yaml.h>

const std::string FILE_CONTENT = "Exists";

TEST(IamTokenClientUtilsTest, ReadFileTest) {
    auto path = std::filesystem::temp_directory_path()
                / "iam_token_client_utils_test";
    std::filesystem::create_directories(path);
    std::ofstream (path / "file_exists") << FILE_CONTENT;
    auto gotString = NTokenAgent::TIamTokenClientUtils::ReadFile((path / "file_exists").string());
    EXPECT_EQ(gotString, FILE_CONTENT);
    EXPECT_THROW_MESSAGE_HAS_SUBSTR({
        try {
            NTokenAgent::TIamTokenClientUtils::ReadFile((path / "file_does_not_exists").string());
        } catch (...){
            throw;
        }}, yexception, "Failed to read");
    std::filesystem::remove_all(path);
}
