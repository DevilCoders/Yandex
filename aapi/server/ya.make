LIBRARY()

OWNER(g:aapi)

PEERDIR(
    aapi/lib/common
    aapi/lib/proto
    aapi/lib/sensors
    aapi/lib/store
    aapi/lib/trace
    aapi/lib/yt
    library/cpp/blockcodecs
    library/cpp/json
    mapreduce/yt/client
    mapreduce/yt/interface
    yweb/robot/js/lib/monitor
)

SRCS(
    config.cpp
    server.cpp
    svn_head.cpp
    tracers.cpp
    warmup.cpp
    ping.cpp
    hg_id.cpp
)

END()
