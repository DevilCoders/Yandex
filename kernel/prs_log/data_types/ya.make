LIBRARY()

OWNER(
    crossby
    g:factordev
)

PEERDIR(
    kernel/web_factors_info
    kernel/web_meta_factors_info
    kernel/factor_storage
    search/meta/generic
    search/rapid_clicks
)

SRCS(
    web.cpp
    log.proto
)

END()
