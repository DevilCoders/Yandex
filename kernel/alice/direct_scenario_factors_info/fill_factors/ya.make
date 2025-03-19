LIBRARY()

OWNER(
    makatunkin
    g:alice_quality
)

PEERDIR(
    alice/megamind/protos/scenarios/features
    kernel/alice/direct_scenario_factors_info
    kernel/factor_storage
    kernel/generated_factors_info
)

SRCS(
    fill_direct_scenario_factors.cpp
)

BASE_CODEGEN(
    kernel/fill_factors_codegen
    fill_factors
)

END()
