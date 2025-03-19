OWNER(
    petrk
    yurakura
)

UNITTEST()

FORK_TESTS()

FORK_SUBTESTS()

DATA(sbr://244316737)

SRCS(
    geodb_ut.cpp
    bestgeo_ut.cpp
)

PEERDIR(
    kernel/geodb
    kernel/search_types
)

END()
