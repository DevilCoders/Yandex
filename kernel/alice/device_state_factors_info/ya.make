LIBRARY()

OWNER(
    tolyandex
    g:alice_quality
)

PEERDIR(
    kernel/generated_factors_info
)

SRCS(
    GLOBAL factor_names.cpp
)

SPLIT_CODEGEN(
    kernel/generated_factors_info/factors_codegen
    factors_gen
    NAliceDeviceState
)

END()
