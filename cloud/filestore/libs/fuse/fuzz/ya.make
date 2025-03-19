FUZZ()

OWNER(g:cloud-nbs)

SRCS(
    main.cpp
    starter.cpp
)

ADDINCL(
    cloud/contrib/vhost
)

PEERDIR(
    cloud/storage/core/libs/common
    cloud/storage/core/libs/diagnostics
    cloud/storage/core/libs/vhost-client

    cloud/filestore/libs/diagnostics
    cloud/filestore/libs/endpoint_vhost
    cloud/filestore/libs/fuse/vhost
    cloud/filestore/libs/service
    cloud/filestore/libs/service_null

    contrib/libs/virtiofsd
)

END()
