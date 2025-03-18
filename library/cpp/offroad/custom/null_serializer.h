#pragma once

#include <util/system/types.h>

namespace NOffroad {
    class TNullSerializer {
    public:
        enum {
            MaxSize = 0
        };

        template <class T>
        static size_t Serialize(const T&, ui8*) {
            return 0;
        }

        template <class T>
        static size_t Deserialize(const ui8*, T*) {
            return 0;
        }
    };

}
