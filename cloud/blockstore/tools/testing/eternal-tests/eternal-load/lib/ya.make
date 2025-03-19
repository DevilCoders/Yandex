LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    config.cpp
    config.sc
    test_executor.cpp
)

PEERDIR(
    cloud/blockstore/libs/diagnostics
    library/cpp/config
    library/cpp/aio
    library/cpp/deprecated/atomic
)

END()
