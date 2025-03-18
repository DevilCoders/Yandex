UNITTEST()

OWNER(
    yanddmi
    kulikov
    g:iss
    g:balancer
)

PEERDIR(
    library/cpp/proto_config
    library/cpp/proto_config/ut/lib
)

SRCS(
    config.cfgproto
    config2.cfgproto
    config3.cfgproto
    test_config.proto
    config_ut.cpp
    plugin_ut.cpp
    custom_options.cpp
)

DATA(
    arcadia/library/cpp/proto_config/ut/config.txt
    arcadia/library/cpp/proto_config/ut/config.lua
)

END()
