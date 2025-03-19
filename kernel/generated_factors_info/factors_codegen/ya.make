PROGRAM()

OWNER(
    alsafr
    g:base
    g:factordev
)

SRCS(
    main.cpp
)

INDUCED_DEPS(h+cpp
    ${ARCADIA_BUILD_ROOT}/kernel/generated_factors_info/metadata/factors_metadata.pb.h
    ${ARCADIA_BUILD_ROOT}/kernel/factor_slices/factor_slices_gen.h
    ${ARCADIA_ROOT}/kernel/factors_codegen/stdlib_deps.h
    ${ARCADIA_ROOT}/kernel/u_tracker/u_tracker.h
    ${ARCADIA_ROOT}/util/charset/unidata.h
)

INDUCED_DEPS(cpp
    ${ARCADIA_ROOT}/kernel/factor_storage/factor_storage.h
    ${ARCADIA_ROOT}/kernel/factors_info/factors_info.h
    ${ARCADIA_ROOT}/util/generic/array_ref.h
    ${ARCADIA_ROOT}/util/generic/bitmap.h
    ${ARCADIA_ROOT}/util/generic/hash.h
    ${ARCADIA_ROOT}/util/generic/noncopyable.h
    ${ARCADIA_ROOT}/util/generic/string.h
    ${ARCADIA_ROOT}/util/generic/utility.h
    ${ARCADIA_ROOT}/util/generic/vector.h
    ${ARCADIA_ROOT}/util/stream/output.h
    ${ARCADIA_ROOT}/util/system/defaults.h
)

PEERDIR(
    kernel/factors_codegen
    kernel/generated_factors_info/metadata
)

END()
