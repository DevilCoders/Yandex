LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    dump.cpp
)

PEERDIR(
    cloud/blockstore/libs/diagnostics/events
    cloud/blockstore/libs/service
)

END()
