LIBRARY()

OWNER(g:begemot)

PEERDIR(
    library/cpp/containers/ring_buffer
    library/cpp/stat-handle/proto
    library/cpp/protobuf/json
    library/cpp/protobuf/util
    library/cpp/scheme
)

SRCS(
    histogram.cpp
    mem.h
    record.cpp
    reqstat.cpp
    rps.cpp
    stat.cpp
    threadsafe_reqstat.cpp
)

END()
