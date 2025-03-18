OWNER(g:morphology)

PY2TEST()

TEST_SRCS(
    diff.py
    document.py
    query.py
)

DEPENDS(
    tools/tokenize
)



END()
