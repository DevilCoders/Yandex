LIBRARY()

OWNER(
    g:base
    g:factordev
    gotmanov
)

PEERDIR(
    kernel/lingboost
    kernel/text_machine/metadata
    library/cpp/json
    library/cpp/vec4
)

SRCS(
    module.cpp
)

END()
