PROGRAM(cpp_program)

OWNER(
    g:blender
)

PEERDIR(
    kernel/blender/factor_storage
    kernel/blender/factor_storage/protos
    library/cpp/getopt
    library/cpp/scheme
    library/cpp/streams/factory
)

SRCS(
    GLOBAL main.cpp
)

END()
