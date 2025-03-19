PROTO_LIBRARY()

OWNER(
    g:itditp
)

SRCS(
    images4pages.proto
    site_recommendation_settings.proto
)

PEERDIR(
    mapreduce/yt/interface/protos
)

EXCLUDE_TAGS(GO_PROTO)

END()

