LIBRARY()

OWNER(
    iddqd
    nsofya
    g:saas
    g:base # since multipart archives are used in basesearch runtime
)

SRCS(
    queue_storage.cpp
    multipart_storage.cpp
)

PEERDIR(
    kernel/multipart_archive/abstract
    kernel/multipart_archive/config
    kernel/multipart_archive/iterators
    kernel/multipart_archive/compressor
    kernel/multipart_archive/data_accessor
    kernel/multipart_archive/part_impl
    library/cpp/logger/global
    library/cpp/object_factory
)

END()
