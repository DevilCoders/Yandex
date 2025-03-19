OWNER(gotmanov)

PY2TEST()

TEST_SRCS(
    test_parse_for_search.py
)

SIZE(SMALL)
TIMEOUT(30)

DEPENDS(
    kernel/reqbundle/tools/parse_for_search
)

DATA(
    sbr://340926413 ## factor_requests_qbundles_100.tsv
)

END()
