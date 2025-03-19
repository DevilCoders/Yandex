UNITTEST_FOR(cloud/filestore/libs/service_kikimr)

OWNER(g:cloud-nbs)

SRCS(
    kikimr_test_env.cpp
    service_ut.cpp
)

PEERDIR(
    ydb/core/testlib
    ydb/core/testlib/basics
)

REQUIREMENTS(ram:12)

END()
