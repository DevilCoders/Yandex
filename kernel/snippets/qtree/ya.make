LIBRARY()

OWNER(g:snippets)

PEERDIR(
    kernel/lemmer/alpha
    kernel/lemmer/core
    kernel/qtree/richrequest
    kernel/snippets/config
    kernel/snippets/telephone
    library/cpp/stopwords
    library/cpp/telfinder
)

SRCS(
    query.cpp
    regionquery.cpp
    squeezer.cpp
    wordnormalizer.cpp
)

GENERATE_ENUM_SERIALIZATION(query.h)

END()

