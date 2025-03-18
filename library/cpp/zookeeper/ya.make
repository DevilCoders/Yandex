LIBRARY()

OWNER(stakanviski)

SRCS(
    data.cpp
    zookeeper.cpp
)

GENERATE_ENUM_SERIALIZATION(defines.h)

PEERDIR(
    contrib/libs/zookeeper
    library/cpp/threading/future
)

END()
