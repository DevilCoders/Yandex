LIBRARY()

OWNER(olegator)

SRCS(
    decompression.cpp
    fast_distance.cpp
)

PEERDIR(
    kernel/dssm_applier/utils
)

GENERATE_ENUM_SERIALIZATION_WITH_HEADER(dssm_model_decompression.h)

END()
