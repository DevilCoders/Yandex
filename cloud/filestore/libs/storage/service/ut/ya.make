UNITTEST_FOR(cloud/filestore/libs/storage/service)

OWNER(g:cloud-nbs)

IF (SANITIZER_TYPE)
    SIZE(MEDIUM)
    TIMEOUT(600)
ENDIF()

SRCS(
    service_ut.cpp
)

PEERDIR(
    cloud/filestore/libs/storage/testlib
    ydb/core/testlib
)

YQL_LAST_ABI_VERSION()

END()
