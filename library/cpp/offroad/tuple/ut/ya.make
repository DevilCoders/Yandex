UNITTEST_FOR(library/cpp/offroad/tuple)

OWNER(
    elric
    g:base
)

SRCS(
    limited_tuple_reader_ut.cpp
    tuple_ut.cpp
)

PEERDIR(
    library/cpp/digest/md5
    library/cpp/offroad/test
    library/cpp/offroad/custom
)

SIZE(LARGE)

TAG(ya:fat)

END()
