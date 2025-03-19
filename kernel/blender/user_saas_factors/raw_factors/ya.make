LIBRARY()

OWNER(
    kitesh
    g:blender
)

PEERDIR(
    kernel/blender/user_saas_factors/protos
    kernel/dssm_applier/nn_applier/lib
    library/cpp/string_utils/base64
)

SRCS(
    common.cpp
    factors_generator.cpp
)

END()

