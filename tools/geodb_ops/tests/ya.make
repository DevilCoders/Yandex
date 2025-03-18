OWNER(yurakura)

PY2TEST()

SIZE(MEDIUM)

FORK_TESTS()

FORK_SUBTESTS()

TEST_SRCS(test.py)

DATA(
    sbr://107977627 # geobase_dump.json
    sbr://107977683 # geodb.bin
)

DEPENDS(tools/geodb_ops)

PEERDIR(
    contrib/python/simplejson
)

END()
