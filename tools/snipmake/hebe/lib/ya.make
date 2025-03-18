LIBRARY()

OWNER(
    divankov
    g:snippets
)

SRCS(
    config.cpp
    run.cpp
    tstab.cpp
)

PEERDIR(
    tools/snipmake/argv
    mapreduce/interface
    yweb/robot/gefest/libkiwiexport
)

END()
