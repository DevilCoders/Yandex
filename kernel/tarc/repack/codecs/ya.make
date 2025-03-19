LIBRARY()

OWNER(
    bazarinm
    g:base
)

SRCS(
    codecs.cpp
    offroad_zones_codec.cpp
    offroad_sentinfo_codec.cpp
)

RESOURCE(
    ./pretrained/zstd-7-text-dict       text_model
    ./pretrained/zstd-7-mkup-dict       mkup_model
    ./pretrained/offroad-sentinfo-model sentinfo_model
    ./pretrained/offroad-zone-model     zone_model
)

PEERDIR(
    kernel/tarc/repack/codecs/flatbuffers
    kernel/tarc/iface
    kernel/tarc/enums
    library/cpp/offroad/codec
    library/cpp/codecs
    library/cpp/codecs/static
    contrib/libs/openssl
)

END()
