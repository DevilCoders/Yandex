#pragma once

#include <ostream>
#include <string>

namespace yandex::cloud::auth {

    class Resource {
    public:
        Resource(std::string id, std::string type)
            : Id_(std::move(id))
            , Type_(std::move(type))
        {
        }

        const std::string& Id() const {
            return Id_;
        }

        const std::string& Type() const {
            return Type_;
        }

        static Resource Cloud(std::string id);

        static Resource Folder(std::string id);

    private:
        std::string Id_;
        std::string Type_;
    };

    std::ostream& operator<<(std::ostream& out, const Resource& resource);

}
