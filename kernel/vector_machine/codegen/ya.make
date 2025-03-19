PROGRAM()

OWNER(
    g:base
    g:neural-search
    carzil
)

SRCS(
    config.proto
    booster_gen.cpp
    calcer_gen.cpp
    aggregator_gen.cpp
    main.cpp
)

INDUCED_DEPS(h
    ${ARCADIA_ROOT}/kernel/dssm_applier/begemot/production_data.h
    ${ARCADIA_ROOT}/kernel/vector_machine/common.h
    ${ARCADIA_ROOT}/kernel/vector_machine/transforms.h
    ${ARCADIA_ROOT}/kernel/vector_machine/similarities.h
    ${ARCADIA_ROOT}/kernel/vector_machine/aggregators.h
    ${ARCADIA_ROOT}/kernel/vector_machine/slicers.h
    ${ARCADIA_ROOT}/util/generic/array_ref.h
)

PEERDIR(
    library/cpp/protobuf/util
    kernel/vector_machine
)

END()
