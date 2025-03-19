GO_LIBRARY()

OWNER(
    g:datacloud
    g:mdb
)

PEERDIR(cloud/mdb/datacloud/private_api/datacloud/clickhouse/v1)

SRCS(
    backupservice.go
    clusterservice.go
    models.go
    operationservice.go
    versionservice.go
)

END()

RECURSE(console)
