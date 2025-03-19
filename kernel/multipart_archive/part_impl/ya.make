LIBRARY()

OWNER(
    iddqd
    nsofya
    alexbykov
    g:saas
    g:base # since multipart archives are used in basesearch runtime
)

SRCS(
    GLOBAL part_compressed.cpp
    GLOBAL part_compressed_ext.cpp
    GLOBAL part_dataless.cpp
    GLOBAL part_raw.cpp
)

PEERDIR(
    kernel/multipart_archive/abstract
    kernel/multipart_archive/compressor
    library/cpp/codecs
    library/cpp/logger/global
    library/cpp/object_factory
    library/cpp/streams/special
)

END()
