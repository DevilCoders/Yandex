LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    validator.cpp
)

PEERDIR(
    cloud/blockstore/libs/diagnostics

    cloud/blockstore/tools/testing/eternal-tests/eternal-load/lib
)

END()

RECURSE_FOR_TESTS(ut)
