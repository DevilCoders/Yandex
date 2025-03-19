#pragma once

#include "public.h"

#include <library/cpp/lwtrace/all.h>

////////////////////////////////////////////////////////////////////////////////

#define FILESTORE_FUSE_PROVIDER(PROBE, EVENT, GROUPS, TYPES, NAMES)            \
    PROBE(RequestReceived,                                                     \
        GROUPS("NFSRequest"),                                                  \
        TYPES(TString, ui64),                                                  \
        NAMES("requestType", "requestId")                                      \
    )                                                                          \
    PROBE(RequestCompleted,                                                    \
        GROUPS("NFSRequest"),                                                  \
        TYPES(TString, ui64),                                                  \
        NAMES("replyType", "requestId")                                        \
    )                                                                          \
// FILESTORE_FUSE_PROVIDER

LWTRACE_DECLARE_PROVIDER(FILESTORE_FUSE_PROVIDER)
