UNITTEST_FOR(cloud/blockstore/libs/storage/volume_proxy)

OWNER(g:cloud-nbs)

FORK_SUBTESTS()
SPLIT_FACTOR(30)

IF (SANITIZER_TYPE OR WITH_VALGRIND)
    TIMEOUT(600)
    SIZE(MEDIUM)
    REQUIREMENTS(
        ram:16
    )
ENDIF()

SRCS(
    volume_proxy_ut.cpp
)

PEERDIR(
    cloud/blockstore/libs/storage/testlib
)


   YQL_LAST_ABI_VERSION()


END()
