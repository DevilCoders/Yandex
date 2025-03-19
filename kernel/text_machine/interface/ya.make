LIBRARY()

OWNER(
    g:base
    g:factordev
    edik
    gotmanov
)

PEERDIR(
    kernel/lingboost
    kernel/text_machine/module
    kernel/text_machine/proto
    kernel/text_machine/structured_id
    library/cpp/binsaver
    library/cpp/langs
    library/cpp/json
    library/cpp/wordpos
)

SRCS(
    cores.cpp
    factory.cpp
    feature.cpp
    feature_part.cpp
    hit.cpp
    query.cpp
    query_set.cpp
    types.cpp
    stream_constants.cpp
)

GENERATE_ENUM_SERIALIZATION(feature_part.h)
GENERATE_ENUM_SERIALIZATION(factory.h)
GENERATE_ENUM_SERIALIZATION(stream_constants.h)

END()
