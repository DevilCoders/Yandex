LIBRARY()

OWNER(
    ilnurkh
    timuratshin
    g:factordev
)

PEERDIR(
    kernel/qtree/request
    kernel/search_query/proto
    library/cpp/unicode/normalization
)

SRCS(
    constants.cpp
    search_query.cpp
    cnorm.cpp
)

GENERATE_ENUM_SERIALIZATION(cnorm.h)

END()
