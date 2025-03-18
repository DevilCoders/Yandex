LIBRARY()

OWNER(zador)

SRCS(
    iterators.h
    minhash_func.cpp
    minhash_helpers.cpp
    minhash_builder.cpp
    prime.cpp
    table.h
)

PEERDIR(
    library/cpp/streams/factory
    library/cpp/succinct_arrays
    util/draft
)

END()
