OWNER(
    g:geoapps_infra
)

LIBRARY()

PEERDIR(
    contrib/restricted/boost/libs/regex
    tools/idl/flex_bison
    tools/idl/utils
    tools/idl/common
)

SRCS(
    parse_idl.cpp
    parse_framework.cpp
)

END()

RECURSE_FOR_TESTS(tests)
