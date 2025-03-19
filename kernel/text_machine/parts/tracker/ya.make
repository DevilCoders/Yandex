LIBRARY()

OWNER(
    g:base
    g:factordev
    gotmanov
    edik
)

PEERDIR(
    kernel/text_machine/interface
    kernel/text_machine/module
    kernel/text_machine/parts/common
    kernel/text_machine/parts/accumulators
    library/cpp/json
)

SRCS(
    all.cpp
    parts_field_set.cpp
)

END()
