#pragma once

#include <string>
#include <util/system/types.h>

namespace NTokenAgent {
    class TGroup {
    public:
        TGroup(std::string name, ui32 id);

        TGroup();

        std::string GetName() const {
            return Name;
        }

        ui32 GetId() const {
            return Id;
        }

        explicit operator bool() const {
            return !Name.empty();
        }

        static TGroup FromName(const char* name);

        static TGroup FromId(ui32 id);

    private:
        std::string Name;
        ui32 Id;
    };
}
