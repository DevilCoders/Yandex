PROGRAM()

OWNER(velavokr)

SRCS(
    segmentator_test.cpp
)

PEERDIR(
    tools/segutils/tests/tests_common
    kernel/segutils
)

END()

RECURSE(tests)
