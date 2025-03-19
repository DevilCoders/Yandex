LIBRARY()

OWNER(
    agafonov-v
)

SRCS(
    factor_names.cpp
    GLOBAL ${BINDIR}/factors_gen.cpp
)

PEERDIR(
    kernel/web_factors_info
)

SPLIT_CODEGEN(
    OUT_NUM 6
    kernel/generated_factors_info/factors_codegen factors_gen
    NWebL3Mx
)

END()
