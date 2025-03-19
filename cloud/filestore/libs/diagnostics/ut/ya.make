UNITTEST_FOR(cloud/filestore/libs/diagnostics)

OWNER(g:cloud-nbs)

PEERDIR(
    library/cpp/eventlog/dumper
)

SRCS(
    filesystem_counters_ut.cpp
    profile_log_ut.cpp
    request_stats_ut.cpp
)

END()
