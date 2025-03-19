LIBRARY()

OWNER(
    alexbykov
    mvel
    velavokr
)

SRCS(
    qd_constants.cpp
    qd_printer.cpp
    qd_registry.cpp
    qd_reporters.cpp
    qd_search.cpp
    qd_server.cpp
    qd_source.cpp
    qd_source_updater.cpp
)

PEERDIR(
    contrib/libs/protobuf
    kernel/querydata/client
    kernel/querydata/common
    kernel/querydata/idl
    kernel/querydata/request
    kernel/querydata/scheme
    kernel/querydata/tries
    kernel/searchlog
    library/cpp/file_checker
    library/cpp/scheme
    library/cpp/string_utils/base64
    library/cpp/string_utils/relaxed_escaper
)

END()
