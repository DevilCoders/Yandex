OWNER(alzobnin)

PROGRAM(tools_queryrectest)

SRCS(
    queryrectest.cpp
)

PEERDIR(
    dict/recognize/queryrec
    kernel/geodb
    library/cpp/getopt
    library/cpp/langs
    library/cpp/streams/factory
)

END()

RECURSE(
    tests
)
