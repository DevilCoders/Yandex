PROGRAM()

OWNER(
    g:snippets
)

SRCS(
    zones_test.cpp
)

PEERDIR(
    tools/segutils/tests/tests_common
    kernel/segutils
)

END()

RECURSE(tests)
