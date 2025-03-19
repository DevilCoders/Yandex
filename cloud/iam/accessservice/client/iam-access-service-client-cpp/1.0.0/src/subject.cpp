#include "cloud-auth/subject.h"

#include <iostream>

namespace yandex::cloud::auth {

    std::ostream& operator<<(std::ostream& out, const UserAccount& userAccount) {
        return out << "UserAccount {id='" << userAccount.Id() << "', federationId='" << userAccount.FederationId() << "'}";
    }

    std::ostream& operator<<(std::ostream& out, const ServiceAccount& serviceAccount) {
        return out << "ServiceAccount {id='" << serviceAccount.Id() << "', folderId='" << serviceAccount.FolderId() << "'}";
    }

    std::ostream& operator<<(std::ostream& out, const Anonymous&) {
        return out << "Anonymous {}";
    }

    std::ostream& operator<<(std::ostream& out, const Subject& subject) {
        std::visit([&out](const auto& any) { out << any; }, subject);
        return out;
    }

}
