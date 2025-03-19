LIBRARY()

OWNER(
    g:base
    g:middle
    gotmanov
)

NO_WSHADOW()

SRCS(
    factor_slices.cpp
    factor_borders.cpp
    factor_domain.cpp
    factor_map.cpp
    meta_info.cpp
    slices_info.cpp
    slice_iterator.cpp
)

IF (GCC)
    CFLAGS(-fno-var-tracking-assignments)
ENDIF()

PEERDIR(
    kernel/factors_info
    kernel/feature_pool
)

BASE_CODEGEN(kernel/factor_slices/slices_codegen factor_slices_gen NFactorSlices)

END()
