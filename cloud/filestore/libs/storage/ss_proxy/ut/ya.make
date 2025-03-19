UNITTEST_FOR(cloud/filestore/libs/storage/ss_proxy)

OWNER(g:cloud-nbs)

IF (SANITIZER_TYPE)
    SIZE(MEDIUM)
    TIMEOUT(600)
ENDIF()

SRCS(
    ss_proxy_ut.cpp
)

PEERDIR(
    cloud/filestore/libs/storage/testlib
)


   YQL_LAST_ABI_VERSION()


END()
