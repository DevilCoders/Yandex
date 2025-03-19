OWNER(g:datacloud)

JAVA_PROGRAM(doublecloud-grpc-api)

UBERJAR()

PEERDIR(
    cloud/mdb/datacloud/private_api/datacloud/clickhouse/v1
    cloud/mdb/datacloud/private_api/datacloud/clickhouse/console/v1
    cloud/mdb/datacloud/private_api/datacloud/console/v1
    cloud/mdb/datacloud/private_api/datacloud/kafka/v1
    cloud/mdb/datacloud/private_api/datacloud/kafka/inner/v1
    cloud/mdb/datacloud/private_api/datacloud/logs/v1
    cloud/mdb/datacloud/private_api/datacloud/kafka/console/v1
    cloud/mdb/datacloud/private_api/datacloud/network/v1
    cloud/mdb/datacloud/private_api/datacloud/v1
)

# excluding non-proto classes (grpc dependencies), they should be imported from other jars
# to see the excluded dependencies run: ya java dependency-tree | less
EXCLUDE(
    contrib/java
)

END()
