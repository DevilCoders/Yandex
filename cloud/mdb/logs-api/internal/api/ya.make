GO_LIBRARY()

OWNER(
    g:datacloud
    g:mdb
)

PEERDIR(
    cloud/mdb/datacloud/private_api/datacloud/logs/v1
    cloud/mdb/datacloud/private_api/datacloud/console/v1
)

SRCS(
    config.go
    consoleservice.go
    health.go
    logsservice.go
    models.go
)

END()
