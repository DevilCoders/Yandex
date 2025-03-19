LIBRARY()

OWNER(
    alejes
    vmakeev
    sankear
)

PEERDIR(
    kernel/begemot_query_factors_info
    kernel/factor_storage
    kernel/generated_factors_info
    search/begemot/rules/query_factors/proto
)

SRCS(
    fill_begemot_query_factors.cpp
)

BASE_CODEGEN(
    kernel/fill_factors_codegen
    fill_factors
)

END()
