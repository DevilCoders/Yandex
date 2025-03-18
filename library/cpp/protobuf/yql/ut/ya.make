UNITTEST()

OWNER(stanly)

SRCS(
    a.proto
    b.proto
    c.proto
    d.proto
    nested.proto
    dynamic_ut.cpp
)

PEERDIR(
    library/cpp/protobuf/yql
)

END()
