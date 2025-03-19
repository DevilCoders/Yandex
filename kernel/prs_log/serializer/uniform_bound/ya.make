LIBRARY()

OWNER(
    crossby
    g:factordev
)

PEERDIR(
    kernel/prs_log/serializer/uniform_bound/proto
    kernel/prs_log/serializer/common
    kernel/dssm_applier/utils
    library/cpp/protobuf/util
)

SRCS(
    serializer.cpp
)

END()
