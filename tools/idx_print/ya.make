PROGRAM()

OWNER(
    g:base
    elric
)
SUBSCRIBER(
    g:saas
)

SRCS(
    idx_print.cpp
)

PEERDIR(
    library/cpp/pop_count
    library/cpp/dbg_output
    library/cpp/getopt
    tools/idx_print/plain_printers
    tools/idx_print/utils
    tools/idx_print/wad_printers
    kernel/doom/array4d
)

END()
