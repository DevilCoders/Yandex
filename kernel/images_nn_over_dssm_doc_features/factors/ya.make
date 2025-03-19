LIBRARY()

LICENSE(YandexNDA)

OWNER(
    temnajab
    g:images-followers g:images-robot g:images-search-quality g:images-nonsearch-quality
)

SRCS(
    GLOBAL factor_names.cpp
)

PEERDIR(
    kernel/generated_factors_info
)

SPLIT_CODEGEN(
    kernel/generated_factors_info/factors_codegen
    factors_gen
    NImagesNnOverDssmDocFeatures
)

END()
