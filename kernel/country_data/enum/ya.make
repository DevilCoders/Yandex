OWNER(yazevnul)

LIBRARY()

SRCS(
    country_cast.cpp
    country_to_alpha2.cpp
    country_to_alpha3.cpp
    country_to_region_id.cpp
)

PEERDIR(
    kernel/country_data/enum/protos
    kernel/search_types
)

END()
