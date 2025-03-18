PROGRAM()

OWNER(
    g:geotargeting
)

NO_COMPILER_WARNINGS()

SRCS(
    checker.cpp
)

PEERDIR(
    ipreg/utils

    library/cpp/deprecated/ipreg1
    library/cpp/getopt/small
    library/cpp/ipreg
    library/cpp/json
)

END()
