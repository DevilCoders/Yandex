OWNER(
    g:adult
    tobo
    vdf
)

LIBRARY()

PEERDIR(
    kernel/porno/proto
    library/cpp/protobuf/util
)

SRCS(
    ad_cat.cpp
    metasearch_fixlist_labels.cpp
)

GENERATE_ENUM_SERIALIZATION(metasearch_fixlist_labels.h)

END()
