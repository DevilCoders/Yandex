OWNER(
    g:geoapps_infra
)

LIBRARY()

PEERDIR(
    tools/idl/common
)

ADDINCL(GLOBAL tools/idl/flex_bison)

SET(OUTPUT_INCLUDES
    tools/idl/flex_bison/flex.h
    util/system/compiler.h
    stdlib.h
    fstream
)

IF (NOT OS_WINDOWS)
    SET(EXTRA_OUTPUT_INCLUDES
        unistd.h
    )
ENDIF()

RUN_PROGRAM(contrib/tools/flex-old -o flex_l.cpp flex.l
    OUTPUT_INCLUDES ${OUTPUT_INCLUDES} ${EXTRA_OUTPUT_INCLUDES} IN flex.l OUT flex_l.cpp)

SRCS(
    bison.y
    internal/bison_helpers.cpp
)

END()
