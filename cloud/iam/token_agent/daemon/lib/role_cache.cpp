#include <algorithm>
#include <iterator>
#include <set>

#include "role_cache.h"

namespace NTokenAgent {

    std::set<std::string> TRoleCache::GetRoleIds() const {
        std::set<std::string> roleIds;
        std::transform(
            RoleMap.begin(), RoleMap.end(), std::inserter(roleIds, roleIds.begin()),
            [](const auto& pair) { return pair.first; });

        return roleIds;
    }

    void TRoleCache::Insert(const std::string& roleId, const TToken& token) {
        RoleMap.insert(std::make_pair(roleId, token));
    }

    TToken TRoleCache::Update(const std::string& roleId, const TToken& token) {
        TRoleHashMap::accessor accessor;
        if (RoleMap.find(accessor, roleId)) {
            TToken oldToken(accessor->second);
            accessor->second = token;

            return oldToken;
        }

        Insert(roleId, token);
        return {};
    }

    void TRoleCache::Erase(const std::string& roleId) {
        RoleMap.erase(roleId);
    }

    TToken TRoleCache::FindToken(const std::string& roleId) const {
        TRoleHashMap::accessor accessor;
        if (RoleMap.find(accessor, roleId)) {
            return accessor->second;
        }

        return {};
    }

}
