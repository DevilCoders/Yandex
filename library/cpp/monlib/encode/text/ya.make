LIBRARY()

OWNER(
    g:solomon
    jamel
)

SRCS(
    text_encoder.cpp
)

PEERDIR(
    library/cpp/monlib/encode
)

END()

RECURSE_FOR_TESTS(
    ut
)
