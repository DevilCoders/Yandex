PY23_TEST()

SIZE(MEDIUM)

OWNER(
    g:yacoll
)

PEERDIR(
    contrib/python/pymongo
    library/python/pytest-mongodb
)

BUILD_ONLY_IF(OS_LINUX OS_DARWIN)
IF (OS_LINUX)
    DATA(sbr://320653966)
ELSEIF (OS_DARWIN)
    DATA(sbr://769724493)
ENDIF()

TEST_SRCS(test.py)

END()
