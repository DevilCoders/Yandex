LIBRARY()

OWNER(tobo)

SRCS(
    binarized_word_hash_vector.h
    binarized_word_hash_vector.cpp
    query.h
    query.cpp
    word_vector.h
    word_vector.cpp
)

PEERDIR(
    library/cpp/digest/md5
    library/cpp/packedtypes
    library/cpp/string_utils/base64
    library/cpp/vowpalwabbit
)

END()
