LIBRARY()

OWNER(
    gotmanov
    g:factordev
    g:base
)

PEERDIR(
    kernel/country_data
    library/cpp/wordpos
)

SRCS(
    constants.cpp
    error_handler.cpp
    freq.cpp
)

GENERATE_ENUM_SERIALIZATION(constants.h)

END()
