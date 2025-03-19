PROTO_LIBRARY()

OWNER(
    tserakhau
    g:data-transfer
)

PEERDIR(
    cloud/dataplatform/api/schemaregistry/options
    tasklet/api/v2
)

SRCS(
    arcdep.proto
    smol.proto
    some.proto
    table.proto
)

END()
