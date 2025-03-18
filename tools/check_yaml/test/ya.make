PY2TEST()

OWNER(g:yatool dmitko)

TEST_SRCS(
    test_check_yaml.py
)

PEERDIR(
    tools/check_yaml/lib
)

DEPENDS(
    tools/check_yaml/bin
)

END()
