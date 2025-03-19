#include "dict_iface.h"

namespace NNewLemmer {
    const wchar16* GetEmptyFlex() {
        static const wchar16 zeroChar = 0;
        return &zeroChar;
    }
}
