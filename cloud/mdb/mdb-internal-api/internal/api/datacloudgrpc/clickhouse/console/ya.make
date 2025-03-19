GO_LIBRARY()

OWNER(
    g:datacloud
    g:mdb
)

PEERDIR(cloud/mdb/datacloud/private_api/datacloud/clickhouse/console/v1)

SRCS(
    cloudservice.go
    clusterservice.go
)

END()
