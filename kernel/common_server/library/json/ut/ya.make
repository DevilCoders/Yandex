UNITTEST_FOR(kernel/common_server/library/json)

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/library/geometry
)

SRCS(
    adapters_ut.cpp
    builder_ut.cpp
    cast_ut.cpp
)

END()
