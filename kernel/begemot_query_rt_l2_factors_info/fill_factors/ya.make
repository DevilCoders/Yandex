LIBRARY()

OWNER(
    hygge
    g:webfresh
)

PEERDIR(
    kernel/begemot_query_rt_l2_factors_info
    kernel/factor_storage
    kernel/generated_factors_info
    search/begemot/rules/webfresh/query_rt_factors/proto
)

SRCS(
    fill_begemot_query_rt_l2_factors.cpp
)

BASE_CODEGEN(
    kernel/fill_factors_codegen
    fill_factors
)

END()
