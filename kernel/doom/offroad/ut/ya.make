UNITTEST_FOR(kernel/doom/offroad)

OWNER(
    elric
    g:base
)

SRCS(
    offroad_panther_io_ut.cpp
)

PEERDIR(
    library/cpp/digest/md5
    kernel/doom/algorithm
    kernel/doom/simple_map
)

REQUIREMENTS(network:full)

# Needed because of win-x86_64-debug, which is too slow for a 60 seconds limit
TIMEOUT(120)
SIZE(MEDIUM)

END()
