LIBRARY()

OWNER(yurakura)

PEERDIR(
    kernel/remorph/tokenizer
    library/cpp/containers/comptrie
    library/cpp/getopt/small
    library/cpp/token
    library/cpp/tokenizer
    library/cpp/json/writer
)

SRCS(
    tokenizer.cpp
    token_types.cpp
    filter.cpp
    detector.cpp
)

END()
