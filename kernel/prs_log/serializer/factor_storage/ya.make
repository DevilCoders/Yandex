LIBRARY()

OWNER(
    crossby
    g:factordev
)

PEERDIR(
    kernel/prs_log/serializer/factor_storage/proto
    kernel/prs_log/serializer/common
    library/cpp/protobuf/util
)

SRCS(
    serializer.cpp
)

END()
