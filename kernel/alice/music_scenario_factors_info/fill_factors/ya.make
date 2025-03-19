LIBRARY()

OWNER(
    movb
    olegator
    g:alice_quality
)

PEERDIR(
    alice/megamind/protos/scenarios/features
    kernel/alice/music_scenario_factors_info
    kernel/factor_storage
)

SRCS(
    fill_music_scenario_factors.cpp
)

BASE_CODEGEN(
    kernel/fill_factors_codegen
    fill_factors
)

END()
