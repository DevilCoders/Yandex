PROGRAM(yc-token-agent)

OWNER(g:cloud-iam)

PEERDIR(
    ADDINCL cloud/iam/token_agent/daemon/lib

    library/cpp/getopt
)

SRCS(
    main.cpp
)

END()

RECURSE(
    test-helper
    lib
)

IF (FUZZING)
    RECURSE(fuzzing)
ENDIF()

RECURSE_FOR_TESTS(ut)
