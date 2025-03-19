LIBRARY()

OWNER(
    iddqd
    nsofya
    g:saas
    g:base # since multipart archives are used in basesearch runtime
)

SRCS(
    multipart.cpp
    owner.cpp
    archive_fat.cpp
    test_utils.cpp
)

PEERDIR(
    kernel/multipart_archive/abstract
    kernel/multipart_archive/statistic
    kernel/multipart_archive/compressor
    kernel/multipart_archive/common
    kernel/multipart_archive/protos
    kernel/multipart_archive/data_accessor
    kernel/multipart_archive/part_impl
    kernel/multipart_archive/archive_impl
    kernel/multipart_archive/iterators
    kernel/index_mapping
    library/cpp/logger/global
    library/cpp/object_factory
    library/cpp/protobuf/protofile
    library/cpp/threading/hot_swap
    library/cpp/threading/named_lock
)

END()
