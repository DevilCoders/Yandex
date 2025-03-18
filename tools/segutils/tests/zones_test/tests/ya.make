OWNER(g:snippets)

PY2TEST()

TEST_SRCS(zones_test.py)

DATA(
    arcadia/yweb/common/roboconf
    arcadia/yweb/urlrules
    arcadia_tests_data/recognize
    arcadia_tests_data/segmentator_tests_data
)

DEPENDS(tools/segutils/tests/zones_test)



END()
