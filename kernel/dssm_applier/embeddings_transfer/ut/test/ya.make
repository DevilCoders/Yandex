OWNER(
    filmih
    g:neural-search
    e-shalnov
)

PY2TEST()

TEST_SRCS(embedding-transfer-test.py)

SIZE(SMALL)

DATA(
    sbr://1608179706 # input.txt
)

PEERDIR(
)

DEPENDS(
    kernel/dssm_applier/embeddings_transfer/ut/applier
    kernel/dssm_applier/embeddings_transfer/ut/diff_tool
)

END()
