LIBRARY()

OWNER(
    g:facts
)

PEERDIR(
    search/session
    library/cpp/resource
    library/cpp/scheme
)

SRCS(
    offline_factors.cpp
)

RESOURCE(
    offline_factors_info.json offline_factors_info
)

END()
