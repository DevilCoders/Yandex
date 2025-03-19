PROTO_LIBRARY()

OWNER(g:cloud-nbs)

EXCLUDE_TAGS(JAVA_PROTO)

SRCS(
    config.proto
    console_base.proto
    console_config.proto
    console_tenant.proto
    console.proto
    msgbus.proto
)

END()
