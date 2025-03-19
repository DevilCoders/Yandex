LIBRARY()

OWNER(
    druxa
    sameg
    mvel
)

PEERDIR(
    kernel/feature_pool/proto
    library/cpp/expression
    library/cpp/regex/pcre
)

SRCS(
    feature_filter.cpp
)

END()
