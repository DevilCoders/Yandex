PY3TEST()

OWNER(g:mstand-online)

TEST_SRCS(
    tests.py
)

SIZE(MEDIUM)

PEERDIR(
    yt/python/client
    mapreduce/yt/python
    tools/mstand/session_squeezer
    tools/mstand/session_squeezer/tests/yt/test_runner
)

DEPENDS(
    yt/packages/latest
)

DATA(
    arcadia/tools/mstand/session_squeezer/tests/yt/alice/data/tests.json
    #local_cypress_tree
    sbr://1119186786
    #libra_files
    sbr://2047237921
)

END()
