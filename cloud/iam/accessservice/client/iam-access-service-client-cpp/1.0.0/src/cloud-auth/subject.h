#pragma once

#include <ostream>
#include <string>
#include <variant>

namespace yandex::cloud::auth {

    class UserAccount {
    public:
        UserAccount(std::string id, std::string federationId)
            : Id_(std::move(id))
            , FederationId_(std::move(federationId))
        {
        }

        bool operator==(const UserAccount& other) const {
            return Id() == other.Id() && FederationId() == other.FederationId();
        }

        const std::string& Id() const {
            return Id_;
        }

        const std::string& FederationId() const {
            return FederationId_;
        }

    private:
        std::string Id_;
        std::string FederationId_;
    };

    std::ostream& operator<<(std::ostream& out, const UserAccount& userAccount);

    class ServiceAccount {
    public:
        ServiceAccount(std::string id, std::string folderId)
            : Id_(std::move(id))
            , FolderId_(std::move(folderId))
        {
        }

        bool operator==(const ServiceAccount& other) const {
            return Id() == other.Id() && FolderId() == other.FolderId();
        }

        const std::string& Id() const {
            return Id_;
        }

        const std::string& FolderId() const {
            return FolderId_;
        }

    private:
        std::string Id_;
        std::string FolderId_;
    };

    std::ostream& operator<<(std::ostream& out, const ServiceAccount& serviceAccount);

    class Anonymous {
    public:
        bool operator==(const Anonymous& /*other*/) const {
            // All anonymous users are equal
            return true;
        }
    };

    std::ostream& operator<<(std::ostream& out, const Anonymous&);

    using Subject = std::variant<UserAccount, ServiceAccount, Anonymous>;

    struct SubjectId {
        const std::string& operator()(const UserAccount& userAccount) {
            return userAccount.Id();
        }

        const std::string& operator()(const ServiceAccount& serviceAccount) {
            return serviceAccount.Id();
        }

        const std::string& operator()(const Anonymous&) {
            throw std::invalid_argument("Anonymous account has no id");
        }

        static const std::string& from(const Subject& subject) {
            return std::visit(SubjectId{}, subject);
        }
    };

    std::ostream& operator<<(std::ostream& out, const Subject& subject);

}
