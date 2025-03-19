OWNER(
    g:neural-search
    e-shalnov
    lexeyo
)

PY2TEST()

TEST_SRCS(batch-processor-test.py)

SIZE(MEDIUM)

DATA(
    sbr://1380797892 # input.txt
    sbr://1646675893 # bert.npz with removed embedding biases
    sbr://1380927295 # vocab.txt
    sbr://1380930618 # start.trie
    sbr://1380937858 # cont.trie
)

PEERDIR(
)

DEPENDS(
    kernel/bert/tests/batch_runner
    kernel/bert/tests/batch_runner/diff_tool
)

END()
