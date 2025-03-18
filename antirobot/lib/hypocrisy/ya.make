OWNER(g:antirobot)

LIBRARY()

SRCS(
    chacha.cpp
    hypocrisy.cpp
)

PEERDIR(
    antirobot/lib
    library/cpp/json
    library/cpp/string_utils/base64
)

END()
