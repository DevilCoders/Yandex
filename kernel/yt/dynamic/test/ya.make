PY2TEST()

OWNER(
    g:fastcrawl
    g:jupiter
)

SIZE(MEDIUM)

REQUIREMENTS (
    ram_disk:2
)

TEST_SRCS(
    test.py
)

PEERDIR(
    mapreduce/yt/python
)

DEPENDS(
    yt/packages/latest
    kernel/yt/dynamic/ut
)

END()
