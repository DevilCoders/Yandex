PY2TEST()

OWNER(g:antiadblock)

TEST_SRCS(
    test_encrypter.py
)

PEERDIR(
    antiadblock/encrypter
)

DATA (
    arcadia/antiadblock/encrypter/tests/test_keys.txt
)

END()
