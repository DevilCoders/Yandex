PY3TEST()

OWNER(
    o-gulyaev
)

TEST_SRCS(
    test_expressions.py
    test_view.py
    test_sanitize.py
)

PEERDIR(
    cloud/ai/speechkit/stt/lib/data/ops
)



END()
