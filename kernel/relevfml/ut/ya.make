UNITTEST()

OWNER(
    apos
    bochkarev
)

PEERDIR(
    ADDINCL kernel/relevfml
    kernel/relevfml/testmodels
)

SRCDIR(kernel/relevfml)

SRCS(
    rank_models_factory_ut.cpp
    relev_fml_ut.cpp
    split_by_specified_ut.cpp
)

END()
