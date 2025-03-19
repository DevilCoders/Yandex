UNITTEST_FOR(cloud/blockstore/libs/ydbstats)

OWNER(g:cloud-nbs)

SRCS(
    ydbstats_ut.cpp
    ydbwriters_ut.cpp
)

PEERDIR(
    ydb/core/testlib
)

END()
