LIBRARY()

OWNER(
    gotmanov
)

PEERDIR(
    contrib/libs/protobuf
    kernel/lemmer
    kernel/lemmer/dictlib
    kernel/search_types
    library/cpp/charset
    library/cpp/langmask
)

SRCS(
    gramfeatures.cpp
    ylemma.cpp
    complexword.cpp
    numeral.cpp
)

END()

RECURSE(
    simple
)
