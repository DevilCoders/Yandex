#include "cloud-auth/credentials.h"

#include <iostream>

namespace yandex::cloud::auth {

    std::ostream& operator<<(std::ostream& out, const ApiKey& key) {
        return out << "ApiKey {value='" << key.Value() << "'}";
    }

    std::ostream& operator<<(std::ostream& out, const IamToken& token) {
        return out << "IamToken {value='" << token.Value() << "'}";
    }

    std::ostream& operator<<(std::ostream& out, const AccessKeySignature& signature) {
        return out << "AccessKeySignature {access_key_id='" << signature.AccessKeyId()
                   << "', signed_string='" << signature.SignedString()
                   << "', signature='"
                   << "TODO"
                   << "', parameters="
                   << "{TODO}"
                   << "'}";
    }

    std::ostream& operator<<(std::ostream& out, const Credentials& credentials) {
        std::visit([&out](const auto& any) { out << any; }, credentials);
        return out;
    }

}
