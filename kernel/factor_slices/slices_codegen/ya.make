PROGRAM(slices_codegen)

OWNER(
    gotmanov
)

SRCS(
    main.cpp
)

PEERDIR(
    kernel/proto_codegen
    kernel/factor_slices/metadata
)

INDUCED_DEPS(cpp
    ${ARCADIA_ROOT}/util/system/guard.h
    ${ARCADIA_ROOT}/util/system/mutex.h
    ${ARCADIA_ROOT}/util/string/cast.h
    ${ARCADIA_ROOT}/util/generic/string.h
    ${ARCADIA_ROOT}/util/generic/hash_set.h
    ${ARCADIA_ROOT}/util/generic/hash.h
    ${ARCADIA_ROOT}/util/generic/xrange.h
    ${ARCADIA_ROOT}/util/generic/typetraits.h
    ${ARCADIA_ROOT}/util/ysaveload.h
)

INDUCED_DEPS(h
    ${ARCADIA_ROOT}/kernel/factor_slices/factor_borders.h
    ${ARCADIA_ROOT}/kernel/factors_info/factors_info.h
    ${ARCADIA_ROOT}/kernel/factor_slices/slice_map.inc
    ${ARCADIA_ROOT}/kernel/factor_slices/slices_codegen/stdlib_deps.h
    ${ARCADIA_ROOT}/util/generic/singleton.h
)

END()
