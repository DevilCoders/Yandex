OWNER(akhropov)

LIBRARY()

PEERDIR(
    kernel/country_data
    kernel/search_types
    library/cpp/deprecated/split
    library/cpp/langmask
    library/cpp/langs
)

SRCS(
    geotree.cpp
    geotree.h
    region_name.cpp
    region_name.h
)

END()
