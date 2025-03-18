#include "os_family.h"

#include <util/generic/array_size.h>
#include <util/generic/strbuf.h>
#include <util/generic/yexception.h>
#include <util/string/escape.h>
#include <util/system/compiler.h>

namespace {
    const TStringBuf ENUM_TO_NAME[] = {
        TStringBuf("unknown"),
        TStringBuf("windows"),
        TStringBuf("android"),
        TStringBuf("ios"),
        TStringBuf("linux"),
        TStringBuf("macos"),
        TStringBuf("java"),
        TStringBuf("windowsphone"),
        TStringBuf("windowsrt"),
        TStringBuf("symbian"),
        TStringBuf("windowsmobile"),
        TStringBuf("tizen"),
        TStringBuf("chromeos"),
        TStringBuf("series40"),
        TStringBuf("freebsd"),
        TStringBuf("bada"),
        TStringBuf("blackberry"),
        TStringBuf("sunos"),
        TStringBuf("rimtabletos"),
        TStringBuf("meego"),
        TStringBuf("nucleus"),
        TStringBuf("webos"),
        TStringBuf("openbsd"),
        TStringBuf("os/2"),
        TStringBuf("firefoxos"),
        TStringBuf("netbsd"),
        TStringBuf("unknownnix"),
        TStringBuf("unix"),
        TStringBuf("brew"),
        TStringBuf("qnx"),
        TStringBuf("moblin"),
        TStringBuf("aix"),
        TStringBuf("irix"),
        TStringBuf("hp-ux"),
        TStringBuf("palmos"),
    };

    static_assert(Y_ARRAY_SIZE(ENUM_TO_NAME) == EOSFamily_ARRAYSIZE, "size doesn't match");
    static_assert(0 == EOSFamily_MIN, "first element must be 0");
}

static bool DoTryFromString(const TStringBuf& name, EOSFamily& value) {
    for (size_t index = 0; index < EOSFamily_ARRAYSIZE; ++index) {
        if (name == ENUM_TO_NAME[index]) {
            value = static_cast<EOSFamily>(index);
            return true;
        }
    }

    return false;
}

template <>
bool TryFromStringImpl<EOSFamily, char>(const char* data, size_t len, EOSFamily& value) {
    return DoTryFromString(TStringBuf{data, len}, value);
}

template <>
EOSFamily FromStringImpl<EOSFamily>(const char* data, size_t len) {
    auto value = EOSFamily{};
    if (Y_UNLIKELY(!TryFromString(data, len, value))) {
        ythrow TFromStringException{} << "failed for value '" << EscapeC(TStringBuf{data, len}) << '\'';
    }

    return value;
}

template <>
void Out<EOSFamily>(IOutputStream& out, TTypeTraits<EOSFamily>::TFuncParam value) {
    if (Y_UNLIKELY(!EOSFamily_IsValid(value))) {
        ythrow yexception() << "invalid value '" << static_cast<int>(value) << '\'';
    }

    out << ENUM_TO_NAME[static_cast<size_t>(value)];
}
