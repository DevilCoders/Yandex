LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    components.cpp
    events.cpp
    service.cpp
    ss_proxy.cpp
    tablet.cpp
    tablet_proxy.cpp
)

PEERDIR(
    cloud/filestore/libs/service
    cloud/filestore/private/api/protos
    cloud/filestore/public/api/protos
    cloud/storage/core/libs/common
    library/cpp/actors/core
    ydb/core/base
    ydb/core/protos
)

END()
