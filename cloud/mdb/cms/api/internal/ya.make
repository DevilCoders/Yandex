GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    autoduty_bin.go
    config.go
)

END()

RECURSE(
    authentication
    clusterdiscovery
    cmsdb
    dom0discovery
    grpcserver
    healthiness
    lockcluster
    logic
    metadb
    models
    settings
    shipments
    tasksclient
    webservice
)
