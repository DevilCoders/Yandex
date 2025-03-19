LIBRARY()

OWNER(
    tolyandex
    g:alice_quality
)

PEERDIR(
    alice/megamind/protos/scenarios/features
    kernel/alice/gc_scenario_factors_info
    kernel/factor_storage
    kernel/generated_factors_info
)

SRCS(
    fill_gc_scenario_factors.cpp
)

BASE_CODEGEN(
    kernel/fill_factors_codegen
    fill_factors
)

END()
