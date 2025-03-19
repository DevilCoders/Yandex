UNITTEST_FOR(cloud/blockstore/libs/spdk)

OWNER(g:cloud-nbs)

TAG(
    ya:not_autocheck
    ya:manual
)

FORK_SUBTESTS()

SRCS(
    address_ut.cpp
    device_ut.cpp
    env_ut.cpp
    histogram_ut.cpp
    target_ut.cpp
)

ADDINCL(
    contrib/libs/spdk/include
    contrib/libs/spdk/module
)

END()
