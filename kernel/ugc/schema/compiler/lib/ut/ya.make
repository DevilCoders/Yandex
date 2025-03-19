OWNER(
    g:ugc
    stakanviski
)

PY3TEST()

TEST_SRCS(
    test_generator.py
    test_parser.py
)

PEERDIR(
    kernel/ugc/schema/compiler/lib
)

END()
