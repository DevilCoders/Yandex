UNITTEST()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/library/storage/zoo
    kernel/common_server/library/storage/local
    kernel/common_server/library/storage/postgres
    kernel/common_server/library/storage/ut/library
)

TIMEOUT(600)

SIZE(MEDIUM)

SRCS(
    storage_ut.cpp
)

FORK_SUBTESTS()
SPLIT_FACTOR(50)

END()
