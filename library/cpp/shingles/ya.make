OWNER(yurap)

LIBRARY()

PEERDIR(
    library/cpp/charset
    library/cpp/digest/old_crc
)

SRCS(
    crossing.cpp
    hash_func.h
    ngram.h
    readable.h
    shingler.h
    shingles.cpp
    word_hash.h
)

END()
