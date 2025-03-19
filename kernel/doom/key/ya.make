LIBRARY()

OWNER(
    elric
    g:base
)

SRCS(
    decoded_key.cpp
    key_encoder.cpp
    key_decoder.cpp
    old_key_decoder.cpp
    old_key_encoder.cpp
)

PEERDIR(
    kernel/doom/enums
    kernel/keyinv/invkeypos
    kernel/search_types
    library/cpp/charset
    library/cpp/langs
)

END()
