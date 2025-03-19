LIBRARY()

OWNER(
    iddqd
    nsofya
    g:saas
)

SRCS(
    part_header.cpp
    part_optimization.cpp
    part_thread_safe.cpp
    parts_utilizator.cpp
    archive_manager.cpp
    multipart_base.cpp
)

PEERDIR(
    kernel/multipart_archive/abstract
    kernel/multipart_archive/protos
    kernel/multipart_archive/part_impl
    library/cpp/balloc/optional
    library/cpp/logger/global
    library/cpp/malloc/system
    library/cpp/object_factory
    library/cpp/protobuf/protofile
    library/cpp/deprecated/atomic
)

END()
