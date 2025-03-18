OWNER(g:antirobot)

PROTO_LIBRARY()

PEERDIR(
    library/cpp/eventlog/proto
    mapreduce/yt/interface/protos
)

SRCS(
    antirobot.ev
    antirobot_cookie.proto
    cache_sync.proto
    captcha_response.proto
    cbb_cache.proto
    daemon_log.proto
    factors.proto
    ip2backend.proto
    spravka_data.proto
)

IF (NOT PY_PROTOS_FOR)
    EXCLUDE_TAGS(GO_PROTO)
ENDIF()

END()
