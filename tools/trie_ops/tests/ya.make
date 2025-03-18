OWNER(yazevnul)

PY2TEST()

TEST_SRCS(test.py)

DATA(
    sbr://85234144  # az77-100-20000.tsv
)

DEPENDS(
    tools/trie_ops
)

FORK_TESTS()
FORK_SUBTESTS()

END()

