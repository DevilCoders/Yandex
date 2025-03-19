LIBRARY()

OWNER(g:facts)

PEERDIR(
    kernel/factor_storage
    kernel/generated_factors_info
)

SPLIT_CODEGEN(kernel/generated_factors_info/factors_codegen factors_gen NUnstructuredFeatures)

GENERATE_ENUM_SERIALIZATION(factors_gen.h)

END()
