#pragma once

#include "public.h"

#include <cloud/storage/core/libs/common/public.h>

#include <library/cpp/lwtrace/all.h>

////////////////////////////////////////////////////////////////////////////////

#define FILESTORE_STORAGE_PROVIDER(PROBE, EVENT, GROUPS, TYPES, NAMES)         \
    PROBE(BackgroundTaskStarted_Tablet,                                        \
        GROUPS("NFSBackground"),                                               \
        TYPES(TString, ui32, ui64, TString),                                   \
        NAMES(                                                                 \
            "requestType",                                                     \
            NCloud::NProbeParam::MediaKind,                                    \
            "requestId",                                                       \
            "fsId"))                                                           \
    PROBE(RequestReceived_Tablet,                                              \
        GROUPS("NFSRequest"),                                                  \
        TYPES(TString, ui64),                                                  \
        NAMES("requestType", "requestId"))                                     \
    PROBE(ResponseSent_Tablet,                                                 \
        GROUPS("NFSRequest"),                                                  \
        TYPES(TString, ui64),                                                  \
        NAMES("requestType", "requestId"))                                     \
    PROBE(RequestReceived_TabletWorker,                                        \
        GROUPS("NFSRequest"),                                                  \
        TYPES(TString, ui64),                                                  \
        NAMES("requestType", "requestId"))                                     \
    PROBE(ResponseSent_TabletWorker,                                           \
        GROUPS("NFSRequest"),                                                  \
        TYPES(TString, ui64),                                                  \
        NAMES("requestType", "requestId"))                                     \
    PROBE(TxInit,                                                              \
        GROUPS("NFSRequest"),                                                  \
        TYPES(TString, ui64),                                                  \
        NAMES("name", "requestId"))                                            \
    PROBE(TxPrepare,                                                           \
        GROUPS("NFSRequest"),                                                  \
        TYPES(TString, ui64),                                                  \
        NAMES("name", "requestId"))                                            \
    PROBE(TxExecute,                                                           \
        GROUPS("NFSRequest"),                                                  \
        TYPES(TString, ui64),                                                  \
        NAMES("name", "requestId"))                                            \
    PROBE(TxComplete,                                                          \
        GROUPS("NFSRequest"),                                                  \
        TYPES(TString, ui64),                                                  \
        NAMES("name", "requestId"))                                            \
    PROBE(ForkFailed,                                                          \
        GROUPS("NFSRequest"),                                                  \
        TYPES(TString, ui64),                                                  \
        NAMES("name", "requestId"))                                            \
// FILESTORE_STORAGE_PROVIDER

LWTRACE_DECLARE_PROVIDER(FILESTORE_STORAGE_PROVIDER)
