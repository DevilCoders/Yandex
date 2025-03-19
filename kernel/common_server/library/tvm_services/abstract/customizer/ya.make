LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/library/metasearch/simple
    kernel/common_server/util/network
    library/cpp/yconf
    library/cpp/object_factory
    kernel/common_server/library/interfaces
    kernel/common_server/library/logging
    library/cpp/tvmauth/client
    kernel/common_server/library/interfaces
    kernel/common_server/library/tvm_services/abstract/request
)

SRCS(
    abstract.cpp
    GLOBAL composite.cpp
    GLOBAL fake.cpp
    GLOBAL zora.cpp
    GLOBAL tvm.cpp
    GLOBAL corp_client_oauth.cpp
)

END()
