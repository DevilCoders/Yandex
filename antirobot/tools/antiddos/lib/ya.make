OWNER(
    g:antirobot
)

LIBRARY()

SRCS(
    antiddos_tool.cpp
    request_iterator.cpp
)

PEERDIR(
    antirobot/daemon_lib
    antirobot/idl
    library/cpp/eventlog
    library/cpp/json
    library/cpp/string_utils/quote
)

END()
