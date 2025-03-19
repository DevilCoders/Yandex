PY2TEST()

OWNER(
    g:facts
)

TEST_SRCS(
    test_normalize.py
)

SIZE(SMALL)
TIMEOUT(60)

PEERDIR(kernel/normalize_by_lemmas)

DEPENDS(
    quality/functionality/entity_search/factqueries/tools/normalize_queries
    dict/gazetteer/compiler
)

DATA(
    arcadia/kernel/normalize_by_lemmas/special_words.gztproto
)

END()
