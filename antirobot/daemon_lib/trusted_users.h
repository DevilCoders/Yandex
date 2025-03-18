#pragma once

#include <util/generic/hash_set.h>


namespace NAntiRobot {

class TTrustedUsers : TMoveOnly {
private:
    THashSet<ui64> Users;

public:
    TTrustedUsers() = default;

    TTrustedUsers(std::initializer_list<ui64> container)
        : Users(std::begin(container), std::end(container)) {
    }

    inline bool Has(ui64 id) const {
        return Users.contains(id);
    }

    void Load(IInputStream& in);
};

} // namespace NAntiRobot
