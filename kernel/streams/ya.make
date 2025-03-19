LIBRARY()

OWNER(
    smikler
    yustuken
)

SRCS(
    streams_factors.cpp
)

IF (GCC)
    CFLAGS(-fno-var-tracking-assignments)
ENDIF()

PEERDIR(
    kernel/streams/metadata
    kernel/generated_factors_info/metadata
    kernel/u_tracker
)

BASE_CODEGEN(kernel/streams/factors_codegen factors_gen)

END()
