LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/solutions/fallback_proxy/src/rt_background/audit_logs_resender
    kernel/common_server/solutions/fallback_proxy/src/rt_background/queue_reader
    kernel/common_server/solutions/fallback_proxy/src/rt_background/queue_resender
)

END()
