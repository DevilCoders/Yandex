PROGRAM()

OWNER(
    g:base
    gotmanov
)

SRCS(
    main.cpp
)

PEERDIR(
    kernel/factor_slices
    kernel/factors_info/all_slices_infos
    kernel/feature_pool/feature_filter
    kernel/ranking_feature_pool
    library/cpp/getopt
    search/l4_features # should it be in all_slices_infos?
)

GENERATE_ENUM_SERIALIZATION(main.h)

END()
