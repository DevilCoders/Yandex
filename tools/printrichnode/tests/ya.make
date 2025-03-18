OWNER(
    g:base
    g:wizard
    onpopov
)

PY2TEST()

TEST_SRCS(printrichnode.py)

DEPENDS(
    tools/printrichnode
    search/wizard/data/wizard/language
)



SIZE(MEDIUM)

END()
