LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    service.cpp
)

PEERDIR(
    cloud/filestore/libs/service

    cloud/storage/core/libs/common
)

END()
