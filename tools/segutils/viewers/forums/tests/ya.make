OWNER(
    grechnik
    g:snippets
)

PY2TEST()

TEST_SRCS(forumstests.py)

DATA(
    arcadia_tests_data/forums_tests_data
    arcadia_tests_data/recognize/dict.dict
)

DEPENDS(tools/segutils/viewers/forums)

END()
