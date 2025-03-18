OWNER(stanly)

PY2TEST()

TEST_SRCS(gumbo_tokenizer.py)

DATA(arcadia_tests_data/indexer_tests_data/tokenizer)

DEPENDS(library/cpp/html/html5/tests/tokenizer)

END()
