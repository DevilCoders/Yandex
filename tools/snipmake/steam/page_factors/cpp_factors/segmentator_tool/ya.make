PROGRAM(segmentator_tool)

OWNER(
    a-bocharov
    g:snippets
)

SRCS(
    main.cpp
)

PEERDIR(
    kernel/matrixnet
    kernel/recshell
    kernel/snippets/strhl
    library/cpp/charset
    library/cpp/domtree
    library/cpp/getopt
    library/cpp/json
    tools/snipmake/steam/page_factors/cpp_factors
    yweb/rca/lib/textbuilder
)

END()

RECURSE(
    tests
)
