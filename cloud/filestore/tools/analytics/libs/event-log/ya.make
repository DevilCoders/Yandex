LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    dump.cpp
)

PEERDIR(
    cloud/filestore/libs/diagnostics/events
    cloud/filestore/libs/service

    cloud/storage/core/libs/common
)

END()
