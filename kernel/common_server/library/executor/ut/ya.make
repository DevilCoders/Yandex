UNITTEST()

OWNER(g:cs_dev)

PEERDIR(
    kernel/daemon/config
    kernel/common_server/library/executor
    kernel/common_server/library/executor/ut/helpers
    kernel/common_server/library/executor/ut/proto
    kernel/common_server/library/executor/vstorage
    kernel/common_server/library/storage/local
    kernel/common_server/library/storage/postgres
)

TAG(
    ya:external
)

TIMEOUT(600)

SIZE(MEDIUM)

SRCS(
    tasks_ut.cpp
)

REQUIREMENTS(
    cpu:4
    network:full
)

FORK_SUBTESTS()

SPLIT_FACTOR(50)

END()
