LIBRARY()

OWNER(
    vl-trifonov
    g:alice_quality
)

PEERDIR(
    alice/protos/api/nlu
    alice/protos/api/nlu/generated
    kernel/alice/begemot_nlu_factors_info
    kernel/alice/begemot_nlu_features
    kernel/factor_storage
    kernel/generated_factors_info
)

BASE_CODEGEN(
    kernel/fill_factors_codegen
    fill_factors
)

SRCS(
    fill_nlu_factors.cpp
)

END()
