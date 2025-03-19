LIBRARY()

OWNER(
    hygge
    g:webfresh
)

PEERDIR(
    kernel/begemot_model_factors_info
    kernel/factor_storage
    kernel/generated_factors_info
    search/begemot/rules/webfresh/fresh_query_detector/proto
)

SRCS(
    fill_begemot_model_factors.cpp
)

BASE_CODEGEN(
    kernel/fill_factors_codegen
    fill_factors
)

END()
