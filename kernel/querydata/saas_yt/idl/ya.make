PROTO_LIBRARY()

OWNER(
    alexbykov
    mvel
    osidorkin
    velavokr
    g:saas
)

SRCS(
    qd_saas_config.proto
    qd_saas_error_record.proto
    qd_saas_input_meta.proto
    qd_saas_input_record.proto
    qd_saas_snapshot_record.proto
)

PEERDIR(
    kernel/querydata/idl
    mapreduce/yt/interface/protos
)

EXCLUDE_TAGS(GO_PROTO)

END()
