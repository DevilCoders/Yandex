GO_PROGRAM()

OWNER(atsygane)

PEERDIR(
    cloud/netinfra/tflow/collector
    cloud/netinfra/tflow/config
    cloud/netinfra/tflow/consumer
    cloud/netinfra/tflow/ipfix
    cloud/netinfra/tflow/metrics
    cloud/netinfra/tflow/sflow
    cloud/netinfra/tflow/util
    cloud/netinfra/tflow/worker
)

SRCS(tflow.go)

END()

RECURSE(
    collector
    config
    consumer
    ipfix
    metrics
    sflow
    util
    worker
)
