LIBRARY()

OWNER(
    elric
    g:base
)

SRCS(
    test_data.h
    test_index.h
    test_key_data.h
    dummy.cpp
)

PEERDIR(
    library/cpp/offroad/custom
    library/cpp/offroad/offset
)

END()
