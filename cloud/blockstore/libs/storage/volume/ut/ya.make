UNITTEST_FOR(cloud/blockstore/libs/storage/volume)

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
    volume_database_ut.cpp
    volume_state_ut.cpp
    volume_ut.cpp
)

PEERDIR(
    cloud/blockstore/libs/storage/testlib
)

END()
