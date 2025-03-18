OWNER(alzobnin)

PY2TEST()

TEST_SRCS(
    test_lemmer.py
    test_lemmer_small.py
    test_disamb.py
)

DATA(
    sbr://439662726
    sbr://334135735
)

DEPENDS(
    tools/lemmer-test
    search/wizard/data/wizard/Automorphology/est
)

SIZE(MEDIUM)



END()
