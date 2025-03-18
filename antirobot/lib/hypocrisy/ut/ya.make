OWNER(g:antirobot)

UNITTEST()

SRCDIR(antirobot/lib/hypocrisy)

SRCS(
    hypocrisy_ut.cpp
)

PEERDIR(
    antirobot/lib/hypocrisy
    library/cpp/string_utils/base64
)

RESOURCE(
    valid_fingerprint.json valid_fingerprint.json
)

END()
