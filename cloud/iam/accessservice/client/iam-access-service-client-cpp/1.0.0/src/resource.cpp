#include "cloud-auth/resource.h"

#include <iostream>

namespace yandex::cloud::auth {

    static const auto RESOURCE_TYPE_CLOUD = "resource-manager.cloud";
    static const auto RESOURCE_TYPE_FOLDER = "resource-manager.folder";

    std::ostream& operator<<(std::ostream& out, const Resource& resource) {
        return out << "Resource {id='" << resource.Id() << "', type='" << resource.Type() << "'}";
    }

    Resource Resource::Cloud(std::string id) {
        return {std::move(id), RESOURCE_TYPE_CLOUD};
    }

    Resource Resource::Folder(std::string id) {
        return {std::move(id), RESOURCE_TYPE_FOLDER};
    }

}
