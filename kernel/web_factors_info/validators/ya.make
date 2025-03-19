LIBRARY()

OWNER(
    gotmanov
    grechnik
    ulanovgeorgiy
    g:factordev
)

PEERDIR(
    kernel/factor_slices
    kernel/matrixnet
    kernel/generated_factors_info/metadata
    library/cpp/archive
    library/cpp/json
    library/cpp/regex/pire
    quality/relev_tools/dependency_graph/lib
    search/formula_chooser
    search/web/rearr_formulas_bundle/reader_lib
)

SRCS(
    impltime_ut_helper.cpp
    dependency_ut_helper.cpp
    models_archive_ut_helper.cpp
    borders_ut_helper.cpp
)

END()
