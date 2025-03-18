OWNER(g:antirobot)

PROGRAM(antirobot_daemon)

ALLOCATOR(LF)

PEERDIR(
    antirobot/daemon_lib
    antirobot/lib
    library/cpp/getopt
    library/cpp/svnversion
    library/cpp/terminate_handler
)

SRCS(
    daemon.cpp
    factors_check.cpp
)

RUN_PROGRAM(
    antirobot/tools/fnames -H 0
    STDOUT ${BINDIR}/factors_hash.h
)

END()
