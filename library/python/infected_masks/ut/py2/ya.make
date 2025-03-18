OWNER(
    g:antimalware
    g:antiwebspam
)

PY2TEST()

TEST_SRCS(infected_masks_test.py)

PEERDIR(
    library/python/infected_masks
)

DATA(arcadia_tests_data/infected_masks)

INCLUDE(../lib/ya.make)

END()
