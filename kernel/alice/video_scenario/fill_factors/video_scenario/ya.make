LIBRARY()

OWNER(
    tolyandex
    g:alice_quality
)

PEERDIR(
    alice/library/video_common/protos
    kernel/alice/video_scenario/video_scenario_factors_info
    kernel/factor_storage
    kernel/generated_factors_info
)

SRCS(
    fill_video_scenario_factors.cpp
)

BASE_CODEGEN(
    kernel/fill_factors_codegen
    fill_factors
)

END()
