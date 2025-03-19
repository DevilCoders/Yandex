#pragma once

#include <string>
#include <util/system/types.h>

namespace NTokenAgent {
    class TUser {
    public:
        TUser(std::string name, ui32 id, ui32 group_id);

        TUser();

        const std::string GetName() const {
            return Name;
        }

        ui32 GetId() const {
            return Id;
        }

        ui32 GetGroupId() const {
            return GroupId;
        }

        operator bool() const {
            return !Name.empty();
        }

        static TUser FromName(const char* name);

        static TUser FromId(ui32 id);

    private:
        std::string Name;
        ui32 Id;
        ui32 GroupId;
    };
}
