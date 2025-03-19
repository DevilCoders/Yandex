UNITTEST()

OWNER(ivanmorozov)

SRCS(
    daemon_ut.cpp
    config_ut.cpp
)

PEERDIR(
    kernel/daemon
    kernel/daemon/common_modules/parent_existence
    search/fetcher
    library/cpp/logger/global
)

DEPENDS(kernel/daemon/ut/daemon)

TAG(
    ya:external
)

REQUIREMENTS(network:full)

SIZE(MEDIUM)

FORK_SUBTESTS()

SPLIT_FACTOR(5)

END()
