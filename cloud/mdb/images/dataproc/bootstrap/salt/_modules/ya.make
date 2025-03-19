PY3TEST()

STYLE_PYTHON()

OWNER(g:mdb-dataproc)

SIZE(SMALL)

PY_SRCS(
    dataproc-agent.py
    ydputils.py
)

TEST_SRCS(
    test_dataproc-agent.py
    test_ydputils.py
)

END()
