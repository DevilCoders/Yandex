#pragma once

#include <set>

#include <contrib/libs/tbb/include/tbb/concurrent_hash_map.h>
#include <util/system/types.h>

#include "token.h"

namespace NTokenAgent {
    class TRoleCache {
    public:
        using TRoleHashMap = tbb::concurrent_hash_map<std::string, TToken>;

        std::set<std::string> GetRoleIds() const;

        void Insert(const std::string& roleId, const TToken& token);
        TToken Update(const std::string& roleId, const TToken& token);
        void Erase(const std::string& roleId);

        TToken FindToken(const std::string& roleId) const;

    private:
        TRoleHashMap RoleMap;
    };
}
