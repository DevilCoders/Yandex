LIBRARY()

OWNER(
    crossby
    mbusel
    g:neural-search
)

PEERDIR(
    kernel/dssm_applier/nn_applier/lib
    kernel/dssm_applier/utils
    library/cpp/packedtypes
)

SRCS(
    production_data.cpp
)

GENERATE_ENUM_SERIALIZATION(production_data.h)

END()
