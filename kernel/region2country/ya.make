LIBRARY()

# Extracted from kernel/relevfml as separate dependency

OWNER(
    g:base
    yazevnul
    mvel
)

SRCS(
    countries.cpp
)

PEERDIR(
    kernel/country_data
    kernel/groupattrs
)

END()
