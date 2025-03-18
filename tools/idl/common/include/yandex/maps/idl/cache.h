#pragma once

#include <yandex/maps/idl/utils/common.h>
#include <yandex/maps/idl/utils/errors.h>
#include <yandex/maps/idl/utils/paths.h>

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace yandex {
namespace maps {
namespace idl {

template <typename Item>
class Cache {
public:
    std::size_t size() const { return cache_.size(); }

    /**
     * Returns an object (Idl or Framework in our case) by first searching
     * in a local cache, and if it isn't there, by searching and parsing
     * its corresponding file.
     */
    Item* get(
        const utils::SearchPaths searchPaths,
        const utils::Path& relativePath,
        const std::function<std::unique_ptr<Item>(
            const utils::Path& searchPath,
            const utils::Path& relativePath,
            const std::string& fileContents)>& itemBuilder)
    {
        auto iterator = cache_.find(relativePath);
        if (iterator != cache_.end()) {
            return iterator->second.get();
        }

        auto pathIndex = searchPaths.resolve(relativePath);
        if (pathIndex == std::size_t(-1)) {
            throw utils::UsageError() <<
                utils::asConsoleBold(
                    "Couldn't find file " + relativePath.inQuotes());
        }

        auto contents = (searchPaths[pathIndex] + relativePath).read();

        auto item = itemBuilder(searchPaths[pathIndex], relativePath, contents);
        return cache_.emplace(relativePath, std::move(item)).first->second.get();
    }

    /**
     * Returns an object (Idl or Framework in our case) by searching in a
     * local cache only. If the object isn't there, returns nullptr.
     */
    Item* getCached(const utils::Path& relativePath)
    {
        auto iterator = cache_.find(relativePath);
        if (iterator == cache_.end()) {
            return nullptr;
        } else {
            return iterator->second.get();
        }
    }

private:
    std::unordered_map<std::string, std::unique_ptr<Item>> cache_;
};

} // namespace idl
} // namespace maps
} // namespace yandex
