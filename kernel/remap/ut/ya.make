UNITTEST()

OWNER(
    g:base
    mvel
)

PEERDIR(
    ADDINCL kernel/remap
)

SRCDIR(
    kernel/remap
)

SRCS(
    remaps_ut.cpp
    remap_table_ut.cpp
)

END()
