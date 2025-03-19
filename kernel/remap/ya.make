LIBRARY()

OWNER(
    g:base
    mvel
)

PEERDIR(
    kernel/factor_storage
    library/cpp/deprecated/split
    library/cpp/string_utils/ascii_encode
)

SRCS(
    remap_adultness.cpp
    remap_table.cpp
    remaps.cpp
)

END()
