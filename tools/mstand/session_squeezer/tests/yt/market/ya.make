PY3TEST()

OWNER(g:mstand-squeeze)

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
    arcadia/tools/mstand/session_squeezer/tests/yt/market/data/pool.json
    arcadia/tools/mstand/session_squeezer/tests/yt/market/data/old_pool.json
    arcadia/tools/mstand/session_squeezer/tests/yt/market/data/unicode_decode_error_pool.json
    #local_cypress_tree
    sbr://2137071513
    #libra_files
    sbr://2047237921
)

REQUIREMENTS(ram:14)

END()
