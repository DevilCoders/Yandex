LIBRARY()

OWNER(
    udovichenko-r
    nslus
)

SRCS(
    rank.cpp
    solve_ambig.cpp
)

GENERATE_ENUM_SERIALIZATION(rank.h)

END()
