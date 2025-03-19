UNITTEST_FOR(cloud/blockstore/libs/storage/service)

OWNER(g:cloud-nbs)

FORK_SUBTESTS()
SPLIT_FACTOR(30)

IF (SANITIZER_TYPE OR WITH_VALGRIND)
    TIMEOUT(600)
    SIZE(MEDIUM)
    REQUIREMENTS(
        ram:16
    )
ENDIF()

SRCS(
    service_state_ut.cpp
    service_ut.cpp
    service_ut_actions.cpp
    service_ut_alter.cpp
    service_ut_describe.cpp
    service_ut_describe_model.cpp
    service_ut_create.cpp
    service_ut_forward.cpp
    service_ut_inactive_clients.cpp
    service_ut_list.cpp
    service_ut_mount.cpp
    service_ut_placement.cpp
    service_ut_read_write.cpp
    service_ut_start.cpp
    service_ut_update_config.cpp
)

PEERDIR(
    cloud/blockstore/libs/storage/testlib
)


   YQL_LAST_ABI_VERSION()



END()
