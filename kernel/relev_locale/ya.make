OWNER(
    dmitryno
    mbusel
    volverine
)

LIBRARY()

SRCS(
    cast.cpp
    get_relev_locale.cpp
    relev_locale.cpp
    serptld.cpp
)

PEERDIR(
    kernel/country_data
    kernel/relev_locale/protos
    kernel/search_types
    library/cpp/langs
)

END()
