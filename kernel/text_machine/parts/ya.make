LIBRARY()

OWNER(
    g:base
    g:factordev
    edik
    gotmanov
)

PEERDIR(
    kernel/text_machine/parts/accumulators
    kernel/text_machine/parts/common
    kernel/text_machine/parts/core
    kernel/text_machine/parts/tracker
)

SRCS(
    parts.cpp
)

END()
