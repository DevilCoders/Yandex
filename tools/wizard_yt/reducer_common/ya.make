LIBRARY()

OWNER(
    g:begemot
)

PEERDIR(
    library/cpp/oauth
    contrib/libs/re2
    mapreduce/yt/common
    mapreduce/yt/interface
    mapreduce/yt/client
    tools/wizard_yt/shard_packer
    library/cpp/getopt
    library/cpp/threading/serial_postprocess_queue
    library/cpp/json/fast_sax
    library/cpp/streams/factory
    library/cpp/string_utils/base64
    apphost/lib/json
    apphost/lib/service_testing
    search/begemot/core
    library/cpp/string_utils/quote
)

SRCS(
    rule_updater.cpp
    cmd_line_options.cpp
)

END()
