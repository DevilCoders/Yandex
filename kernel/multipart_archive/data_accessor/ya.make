LIBRARY()

OWNER(
    iddqd
    nsofya
    g:saas
    g:base # since multipart archives are used in basesearch runtime
)

SRCS(
    GLOBAL data_accessor_file.cpp
    GLOBAL data_accessor_mem.cpp
    GLOBAL data_accessor_mem_map.cpp
    GLOBAL data_accessor_mem_from_file.cpp
)

PEERDIR(
    kernel/multipart_archive/abstract
    library/cpp/logger/global
    library/cpp/object_factory
    library/cpp/streams/special
)

END()
