OWNER(akhropov)

LIBRARY()

SRCS(
    geo.h
    reg_data.h
    reg_data.cpp
    utils.cpp
)

PEERDIR(
    kernel/country_data
    kernel/geo/tree
    kernel/geo/types
    library/cpp/binsaver
    library/cpp/deprecated/small_array
    library/cpp/deprecated/split
    library/cpp/geobase
    util/draft
)

END()
