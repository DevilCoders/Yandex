PROGRAM(factors_streams_codegen)

OWNER(
    smikler
    yustuken
)

SRCS(
    main.cpp
)

PEERDIR(
    kernel/proto_codegen
    kernel/streams/metadata
)

INDUCED_DEPS(h+cpp
    ${ARCADIA_BUILD_ROOT}/kernel/generated_factors_info/metadata/factors_metadata.pb.h
    ${ARCADIA_ROOT}/kernel/u_tracker/u_tracker.h
    ${ARCADIA_ROOT}/util/charset/unidata.h
)

END()
