PY2TEST()

OWNER(torkve)

BUILD_ONLY_IF(LINUX)

PEERDIR(library/python/sendmsg)

TEST_SRCS(test_sendmsg.py)

END()
