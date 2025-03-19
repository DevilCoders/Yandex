#pragma once

#include <util/datetime/base.h>
#include <filesystem>
#include "token.h"
#include "user.h"

namespace YAML {
    class Node;
}

class TFsPath;
namespace NTokenAgent {
    class TRole {
    public:
        TRole(const std::string& name, const YAML::Node& node);

        [[nodiscard]] const std::string& GetName() const {
            return Name;
        }

        [[nodiscard]] const TToken& GetToken() const {
            return Token;
        }

        [[nodiscard]] const std::string& GetServiceAccountId() const {
            return ServiceAccountId;
        }

        [[nodiscard]] const std::string& GetKeyId() const {
            return KeyId;
        }

        [[nodiscard]] ui64 GetKeyHandle() const {
            return KeyHandle;
        }

        static TRole FromFile(const std::filesystem::path& root, const std::filesystem::path& path);

    private:
        std::string Name;
        TToken Token;
        std::string ServiceAccountId;
        std::string KeyId;
        ui64 KeyHandle;
    };
}
