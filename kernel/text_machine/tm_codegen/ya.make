PROGRAM(tm_codegen)

OWNER(
    g:base
    g:factordev
    edik
    gotmanov
)

SRCS(
    main.cpp
)

PEERDIR(
    kernel/proto_codegen
    kernel/text_machine/metadata
    kernel/text_machine/parts
    kernel/text_machine/parts/core/static
    kernel/text_machine/parts/tracker/static
    library/cpp/getopt/small
)

END()
