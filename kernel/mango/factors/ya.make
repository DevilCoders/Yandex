LIBRARY()

OWNER(sashateh)

PEERDIR(
    kernel/keyinv/invkeypos
    kernel/mango/common
    kernel/mango/proto
    kernel/mango/url
    kernel/qtree/richrequest
    kernel/web_factors_info
    library/cpp/binsaver
    library/cpp/prob_counter
    library/cpp/shingles
    util/draft
)

SRCS(
    doc_hit_aggregator.cpp
    factors.cpp
    hits_based_tr.cpp
    link.cpp
    prel.cpp
    quorum.cpp
)

END()
