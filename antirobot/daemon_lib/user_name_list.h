#pragma once

#include <util/generic/hash_set.h>

class IInputStream;

namespace NAntiRobot {
    class TUserNameList {
    public:
        void Load(IInputStream& in);
        bool UserInList(const TString& userName) const;
        size_t Size() const {
            return Set.size();
        }
    private:
        THashSet<TString> Set;
    };
}
