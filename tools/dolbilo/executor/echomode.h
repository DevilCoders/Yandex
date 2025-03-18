#pragma once

#include <util/generic/serialized_enum.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>

enum EEchoMode {
    ECHO_NULL = 0       /* "null"   */,
    ECHO_DOLBILO        /* "dolbilo"   */,
    ECHO_PHANTOM_AMMO   /* "phantom-ammo" */,
    ECHO_PHANTOM_STPD   /* "phantom-stpd" */,
};

