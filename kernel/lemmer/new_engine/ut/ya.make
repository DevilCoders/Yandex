UNITTEST()

OWNER(g:morphology)

PEERDIR(
    kernel/lemmer/new_engine/build_lib/protos
    kernel/lemmer/new_engine/binary_dict
    kernel/lemmer/new_engine/proto_dict
    library/cpp/streams/factory
)

ARCHIVE_ASM(
    NAME ExBinaryDict
    DONTCOMPRESS
    ExEngDict.bin
)

ARCHIVE_ASM(
    NAME ExProtoDict
    DONTCOMPRESS
    ExEngDict.msg
)

SRCS(
    kernel/lemmer/new_engine/dict_iface_ut.cpp
)

END()
