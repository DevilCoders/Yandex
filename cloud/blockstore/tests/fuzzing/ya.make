FUZZ()

OWNER(g:cloud-nbs)

SIZE(LARGE)

TAG(ya:fat)

SRCS(
    main.cpp
    starter.cpp
)

PEERDIR(
    cloud/blockstore/libs/daemon

    cloud/storage/core/libs/common

    library/cpp/testing/common
)

INCLUDE(${ARCADIA_ROOT}/cloud/blockstore/tests/fuzzing/config.inc)

END()

RECURSE_FOR_TESTS(config)
