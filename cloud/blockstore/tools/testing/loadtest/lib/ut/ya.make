UNITTEST_FOR(cloud/blockstore/tools/testing/loadtest/lib)

OWNER(g:cloud-nbs)

FORK_SUBTESTS()

IF (SANITIZER_TYPE OR WITH_VALGRIND)
    TIMEOUT(300)
    SIZE(MEDIUM)
ENDIF()

SRCS(
    range_map_ut.cpp
)

END()
