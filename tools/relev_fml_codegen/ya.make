PROGRAM()

INDUCED_DEPS(
    cpp
    ${ARCADIA_ROOT}/library/cpp/sse/sse.h
    ${ARCADIA_ROOT}/kernel/relevfml/relev_fml.h
)

SRCS(
    relev_fml_codegen.cpp
)

PEERDIR(
    kernel/relevfml/code_gen
    kernel/web_factors_info
)

SET(IDE_FOLDER "_Builders")

END()
