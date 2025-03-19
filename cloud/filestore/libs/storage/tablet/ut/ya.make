UNITTEST_FOR(cloud/filestore/libs/storage/tablet)

OWNER(g:cloud-nbs)

FORK_SUBTESTS()

SPLIT_FACTOR(30)

IF (SANITIZER_TYPE)
    SIZE(MEDIUM)
    TIMEOUT(600)
ENDIF()

SRCS(
    rebase_logic_ut.cpp
    tablet_database_ut.cpp
    tablet_ut.cpp
    tablet_ut_channels.cpp
    tablet_ut_checkpoints.cpp
    tablet_ut_data.cpp
    tablet_ut_nodes.cpp
    tablet_ut_sessions.cpp
)

PEERDIR(
    cloud/filestore/libs/storage/testlib
    ydb/core/testlib
)

YQL_LAST_ABI_VERSION()

REQUIREMENTS(ram:12)

END()
