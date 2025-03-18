PROGRAM()

OWNER(
    g:balancer
    kulikov
)

PEERDIR(
    contrib/libs/protoc
    library/cpp/eventlog/proto
    library/cpp/proto_config/protos
    library/cpp/proto_config/codegen
)

SRCS(
    main.cpp
)

INDUCED_DEPS(h+cpp
    ${ARCADIA_ROOT}/util/generic/vector.h
    ${ARCADIA_ROOT}/util/generic/hash.h
    ${ARCADIA_ROOT}/util/datetime/base.h
    ${ARCADIA_ROOT}/library/cpp/proto_config/codegen/parse_value.h
)

END()
