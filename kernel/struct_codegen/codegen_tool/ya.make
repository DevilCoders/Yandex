PROGRAM()

OWNER(
    g:base
    mvel
)

PEERDIR(
    contrib/libs/protobuf
    kernel/proto_codegen
    kernel/struct_codegen/metadata
    kernel/struct_codegen/reflection
)

SRCS(
    main.cpp
)

END()
