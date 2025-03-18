PY23_LIBRARY()

OWNER(
    rmcf
    g:yabs-rt
)

TEST_SRCS(
    test_protoseq.py
)

PEERDIR(
    library/python/framing
    library/python/framing/ut/proto_example
)

END()
