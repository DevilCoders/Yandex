PROTO_LIBRARY()

OWNER(g:jupiter)

SRCS(
    searchzone.proto
)

PEERDIR(
    mapreduce/yt/interface/protos
)

IF (NOT PY_PROTOS_FOR)
    EXCLUDE_TAGS(GO_PROTO)
ENDIF()

END()
