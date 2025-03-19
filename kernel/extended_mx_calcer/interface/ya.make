LIBRARY()

OWNER(
    epar
    g:blender
)

SRCS(
    calcer_exception.cpp
    common.cpp
    context.cpp
    extended_relev_calcer.cpp
)

PEERDIR(
    kernel/extended_mx_calcer/proto
    kernel/matrixnet
)

GENERATE_ENUM_SERIALIZATION(common.h)

END()
