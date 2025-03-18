#include "work_mode.h"

#include <util/stream/output.h>
#include <util/string/cast.h>

using namespace NAntiRobot;

namespace NAntiRobot {
    static const TStringBuf WORK_MODE_STRS[] = {
        "enabled",
        "disabled"
    };
}

template<>
EWorkMode FromStringImpl(const char* data, size_t len) {
    static_assert(Y_ARRAY_SIZE(WORK_MODE_STRS) == WORK_NUM_MODES, "expect Y_ARRAY_SIZE(WORK_MODE_STRS) == WORK_NUM_MODES");
    TStringBuf dataBuf(data, len);
    for (size_t i = 0; i < WORK_NUM_MODES; ++i) {
        if (dataBuf == WORK_MODE_STRS[i])
            return (EWorkMode)i;
    }
    ythrow yexception() << "Invalid work mode: "sv << dataBuf;
}

template<>
void Out<EWorkMode>(IOutputStream& out, EWorkMode workMode) {
    out << WORK_MODE_STRS[workMode];
}
