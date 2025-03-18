LIBRARY()

OWNER(
    svshevtsov
    g:geosaas
)

PEERDIR(
    library/cpp/logger/global
    library/cpp/histogram/rt/proto
    library/cpp/unistat
    library/cpp/deprecated/atomic
)

SRCS(
    events_by_enum.cpp
    histogram.cpp
    fix_interval.cpp
    time_slide.cpp
    histogram.proto
)

END()
