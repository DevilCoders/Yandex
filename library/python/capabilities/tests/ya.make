OWNER(torkve)

PY23_TEST()

IF(OS_LINUX)
TEST_SRCS(test_capabilities.py)
ENDIF()

PEERDIR(library/python/capabilities)

FORK_TESTS()

REQUIREMENTS(ram:9)

END()
