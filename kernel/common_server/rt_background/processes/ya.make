LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/rt_background/processes/common
    kernel/common_server/rt_background/processes/db_cleaner
    kernel/common_server/rt_background/processes/dumper
    kernel/common_server/rt_background/processes/queue_executor
    kernel/common_server/rt_background/processes/propositions_executor
    kernel/common_server/rt_background/processes/snapshots_diff
)

END()
