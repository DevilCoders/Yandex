LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    library/cpp/tvmauth/client
    kernel/common_server/library/geometry
    maps/doc/proto/yandex/maps/proto/common2
    maps/doc/proto/yandex/maps/proto/search
    kernel/common_server/library/json
    kernel/common_server/library/tvm_services/abstract
    kernel/common_server/obfuscator/obfuscators
)

SRCS(
    client.cpp
    request.cpp
    GLOBAL customizer.cpp
)

END()
