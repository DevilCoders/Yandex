OWNER(g:mstand)

LIBRARY()

SRCS(
    common.cpp
    common.sc
    squeeze_impl.cpp
    squeeze_yt.cpp
    squeezer.cpp
    squeezer_common.cpp
    squeezer_market_search_sessions.cpp
    squeezer_web.cpp
    squeezer_web_extended.cpp
    squeezer_web_surveys.cpp
    squeezer_yuid_reqid_testid_filter.cpp
    squeezer_video.cpp
)

PEERDIR(
    library/cpp/scheme
    mapreduce/yt/interface
    quality/user_sessions/createlib/qb3/parser
    quality/user_sessions/request_aggregate_lib
    tools/mstand/squeeze_lib/requests/common
    tools/mstand/squeeze_lib/requests/market
    tools/mstand/squeeze_lib/requests/web
)

YQL_LAST_ABI_VERSION()

END()
