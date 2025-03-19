PROTO_LIBRARY()

OWNER(
    alexbykov
    mvel
    velavokr
)

SRCS(
    querydata_common.proto
    querydata_structs.proto
    querydata_structs_client.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
