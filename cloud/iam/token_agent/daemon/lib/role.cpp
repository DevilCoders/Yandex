#include <filesystem>

#include <yaml-cpp/yaml.h>
#include <library/cpp/logger/global/global.h>

#include "role.h"

namespace NTokenAgent {
    static const TInstant DEFAULT_EXPIRES_AT = TInstant::Days(376200);

    static TInstant ParseInstant(const std::string& str) {
        return str.empty() ? DEFAULT_EXPIRES_AT : TInstant::ParseIso8601(str);
    }

    TRole::TRole(const std::string& name, const YAML::Node& node)
        : Name(name)
        , Token(node["token"].as<std::string>(""),
                ParseInstant(node["expiresAt"].as<std::string>("")))
        , ServiceAccountId(node["serviceAccountId"].as<std::string>(""))
        , KeyId(node["keyId"].as<std::string>(""))
        , KeyHandle(node["keyHandle"].as<ui64>(0ul))
    {
        DEBUG_LOG << "Parsed role for " << name << ":\n"
                  << (std::stringstream() << node).str() << "\n";
    }

    TRole TRole::FromFile(const std::filesystem::path& root, const std::filesystem::path& path) {
        INFO_LOG << "Loading role file " << path.c_str() << "\n";
        auto relative_path = std::string(std::filesystem::relative(path, root));
        // "hwmonitor" or "root/yc-compute"
        auto name = relative_path.substr(0, relative_path.find_last_of('.'));

        return {name, YAML::LoadFile(path)};
    }
}
