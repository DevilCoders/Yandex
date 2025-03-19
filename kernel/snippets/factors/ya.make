LIBRARY()

LICENSE(YandexNDA)

OWNER(g:snippets)

PEERDIR(
    kernel/generated_factors_info
)

SRCS(
    GLOBAL factors.cpp
)

SPLIT_CODEGEN(
    kernel/generated_factors_info/factors_codegen
    factors_gen
    NSnippets
)

SPLIT_CODEGEN(
    kernel/generated_factors_info/factors_codegen
    factors_gen_web
    NSnippetsWeb
)

SPLIT_CODEGEN(
    kernel/generated_factors_info/factors_codegen
    factors_gen_web_v1
    NSnippetsWebV1
)

SPLIT_CODEGEN(
    kernel/generated_factors_info/factors_codegen
    factors_gen_web_noclick
    NSnippetsWebNoclick
)

GENERATE_ENUM_SERIALIZATION(factors_gen.h)

END()
