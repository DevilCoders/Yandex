UNITTEST_FOR(library/cpp/offroad/codec)

OWNER(
    elric
    g:base
)

SRCS(
    codec_ut.cpp
    serializable_model_ut.cpp
)

PEERDIR(
    library/cpp/digest/md5
    library/cpp/offroad/test
)

END()
