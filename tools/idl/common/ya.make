OWNER(
    g:geoapps_infra
)

LIBRARY()

PEERDIR(
    tools/idl/utils
    contrib/restricted/boost
)

ADDINCL(
    GLOBAL tools/idl/common/include
)

SRCS(
    validators/validators.cpp
    validators/variable_kind.cpp
    env.cpp
    full_type_ref.cpp
    functions.cpp
    scope.cpp
    nodes/type_ref.cpp
    nodes/nodes.cpp
    utils.cpp
)

END()

RECURSE_FOR_TESTS(
    tests
)
