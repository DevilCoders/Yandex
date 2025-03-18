OWNER(g:antirobot)

PROGRAM(antirobot_evlogdump)

PEERDIR(
    antirobot/idl
    antirobot/tools/evlogdump/lib
    library/cpp/getopt
    library/cpp/string_utils/quote
    library/cpp/framing
    library/cpp/protobuf/json
)

SRCS(
    evlogdump.cpp
)

END()

RECURSE(
    lib
)
