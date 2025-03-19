LIBRARY()

OWNER(vvp)

SRCS(
    countries.cpp
)

PEERDIR(
    kernel/relev_locale/protos
    kernel/search_types
    library/cpp/deprecated/split
    library/cpp/langmask
    library/cpp/string_utils/url
)

END()
