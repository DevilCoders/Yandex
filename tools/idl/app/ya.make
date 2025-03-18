OWNER(
    g:geoapps_infra
)

LIBRARY()

ADDINCL(GLOBAL tools/idl/include)

# This condition is intended to cure the problem
# of configuring a tool in the context of target graph
IF (NOT MAPSMOBI_BUILD_TARGET)

PEERDIR(
    contrib/restricted/boost/libs/program_options
    tools/idl/utils
    tools/idl/common
    tools/idl/parser
    tools/idl/generator
)

SRCS(
    app.cpp
    utils.cpp
)

ELSE()

NO_UTIL()

ENDIF()

END()
